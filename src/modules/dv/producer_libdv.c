/*
 * producer_libdv.c -- simple libdv test case
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "producer_libdv.h"
#include <framework/mlt_frame.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libdv/dv.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct producer_libdv_s *producer_libdv;

struct producer_libdv_s
{
	struct mlt_producer_s parent;
	int fd;
	dv_decoder_t *dv_decoder;
	int is_pal;
	uint64_t file_size;
	int frame_size;
	long frames_in_file;
};

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

static int producer_collect_info( producer_libdv this );

mlt_producer producer_libdv_init( char *filename )
{
	producer_libdv this = calloc( sizeof( struct producer_libdv_s ), 1 );

	if ( filename != NULL && this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		mlt_producer producer = &this->parent;
		mlt_properties properties = mlt_producer_properties( producer );

		// Register transport implementation with the producer
		producer->close = producer_close;

		// Register our get_frame implementation with the producer
		producer->get_frame = producer_get_frame;

		// Create the dv_decoder
		this->dv_decoder = dv_decoder_new( FALSE, FALSE, FALSE );
		this->dv_decoder->quality = DV_QUALITY_BEST;
		this->dv_decoder->audio->arg_audio_emphasis = 2;
		dv_set_audio_correction( this->dv_decoder, DV_AUDIO_CORRECT_AVERAGE );

		// Open the file if specified
		this->fd = open( filename, O_RDONLY );

		// Collect info
		if ( this->fd != -1 && producer_collect_info( this ) )
		{
			// Set the resource property (required for all producers)
			mlt_properties_set( properties, "resource", filename );
		}
		else
		{
			// Reject this file
			mlt_producer_close( producer );
			producer = NULL;
		}

		// Return the producer
		return producer;
	}
	free( this );
	return NULL;
}

static int read_frame( int fd, uint8_t* frame_buf, int *isPAL )
{
	int result = read( fd, frame_buf, frame_size_525_60 ) == frame_size_525_60;
	if ( result )
	{
		*isPAL = ( frame_buf[3] & 0x80 );

		if ( *isPAL )
		{
			int diff = frame_size_625_50 - frame_size_525_60;
			result = read( fd, frame_buf + frame_size_525_60, diff ) == diff;
		}
	}
	
	return result;
}

static int producer_collect_info( producer_libdv this )
{
	int valid = 0;

	uint8_t *dv_data = mlt_pool_alloc( frame_size_625_50 );

	if ( dv_data != NULL )
	{
		// Read the first frame
		valid = read_frame( this->fd, dv_data, &this->is_pal );

		// If it looks like a valid frame, the get stats
		if ( valid )
		{
			// Get the properties
			mlt_properties properties = mlt_producer_properties( &this->parent );

			// Determine the file size
			struct stat buf;
			fstat( this->fd, &buf );

			// Store the file size
			this->file_size = buf.st_size;

			// Determine the frame size
			this->frame_size = this->is_pal ? frame_size_625_50 : frame_size_525_60;

			// Determine the number of frames in the file
			this->frames_in_file = this->file_size / this->frame_size;

			// Calculate default in/out points
			double fps = this->is_pal ? 25 : 30000.0 / 1001.0;
			if ( mlt_properties_get_double( properties, "fps" ) == fps )
			{
				mlt_properties_set_position( properties, "length", this->frames_in_file );
				mlt_properties_set_position( properties, "in", 0 );
				mlt_properties_set_position( properties, "out", this->frames_in_file - 1 );
			}
			else
			{
				valid = 0;
			}

			// Parse the header for meta info
			dv_parse_header( this->dv_decoder, dv_data );
			mlt_properties_set_double( properties, "aspect_ratio", dv_format_wide( this->dv_decoder ) ? 16.0/9.0 : 4.0/3.0 );
		}

		mlt_pool_release( dv_data );
	}

	return valid;
}

static int producer_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	int pitches[3] = { 0, 0, 0 };
	uint8_t *pixels[3] = { NULL, NULL, NULL };
	
	// Get the frames properties
	mlt_properties properties = mlt_frame_properties( this );

	// Get the dv decoder
	dv_decoder_t *decoder = mlt_properties_get_data( properties, "dv_decoder", NULL );

	// Get the dv data
	uint8_t *dv_data = mlt_properties_get_data( properties, "dv_data", NULL );

	// Parse the header for meta info
	dv_parse_header( decoder, dv_data );
	
	// Assign width and height from properties
	*width = mlt_properties_get_int( properties, "width" );
	*height = mlt_properties_get_int( properties, "height" );

	// Extract an image of the format requested
	if ( *format == mlt_image_yuv422 )
	{
		// Allocate an image
		uint8_t *image = mlt_pool_alloc( *width * ( *height + 1 ) * 2 );

		// Pass to properties for clean up
		mlt_properties_set_data( properties, "image", image, *width * ( *height + 1 ) * 2, ( mlt_destructor )mlt_pool_release, NULL );

		// Decode the image
		pitches[ 0 ] = *width * 2;
		pixels[ 0 ] = image;
		dv_decode_full_frame( decoder, dv_data, e_dv_color_yuv, pixels, pitches );

		// Assign result
		*buffer = image;
	}
	else if ( *format == mlt_image_rgb24 )
	{
		// Allocate an image
		uint8_t *image = mlt_pool_alloc( *width * ( *height + 1 ) * 3 );

		// Pass to properties for clean up
		mlt_properties_set_data( properties, "image", image, *width * ( *height + 1 ) * 3, ( mlt_destructor )mlt_pool_release, NULL );

		// Decode the frame
		pitches[ 0 ] = 720 * 3;
		pixels[ 0 ] = image;
		dv_decode_full_frame( decoder, dv_data, e_dv_color_rgb, pixels, pitches );

		// Assign result
		*buffer = image;
	}

	return 0;
}

static int producer_get_audio( mlt_frame this, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	int16_t *p;
	int i, j;
	int16_t *audio_channels[ 4 ];
	
	// Get the frames properties
	mlt_properties properties = mlt_frame_properties( this );

	// Get the dv decoder
	dv_decoder_t *decoder = mlt_properties_get_data( properties, "dv_decoder", NULL );

	// Get the dv data
	uint8_t *dv_data = mlt_properties_get_data( properties, "dv_data", NULL );

	// Parse the header for meta info
	dv_parse_header( decoder, dv_data );

	// Obtain required values
	//fprintf( stderr, "libdv: frequency %d\n", decoder->audio->frequency );
	*frequency = decoder->audio->frequency;
	*samples = decoder->audio->samples_this_frame;
	*channels = decoder->audio->num_channels;

	// Create a temporary workspace
	for ( i = 0; i < 4; i++ )
		audio_channels[ i ] = mlt_pool_alloc( DV_AUDIO_MAX_SAMPLES * sizeof( int16_t ) );

	// Create a workspace for the result
	*buffer = mlt_pool_alloc( *channels * DV_AUDIO_MAX_SAMPLES * sizeof( int16_t ) );

	// Pass the allocated audio buffer as a property
	mlt_properties_set_data( properties, "audio", *buffer, *channels * DV_AUDIO_MAX_SAMPLES * sizeof( int16_t ), ( mlt_destructor )mlt_pool_release, NULL );

	// Decode the audio
	dv_decode_full_audio( decoder, dv_data, audio_channels );
	
	// Interleave the audio
	p = *buffer;
	for ( i = 0; i < *samples; i++ )
		for ( j = 0; j < *channels; j++ )
			*p++ = audio_channels[ j ][ i ];

	// Free the temporary work space
	for ( i = 0; i < 4; i++ )
		mlt_pool_release( audio_channels[ i ] );

	return 0;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	producer_libdv this = producer->child;
	uint8_t *data = mlt_pool_alloc( frame_size_625_50 );
	
	// Obtain the current frame number
	uint64_t position = mlt_producer_frame( producer );
	
	// Convert timecode to a file position (ensuring that we're on a frame boundary)
	uint64_t offset = position * this->frame_size;

	// Create an empty frame
	*frame = mlt_frame_init( );

	// Seek and fetch
	if ( this->fd != 0 && 
		 lseek( this->fd, offset, SEEK_SET ) == offset &&
		 read_frame( this->fd, data, &this->is_pal ) )
	{
		// Get the frames properties
		mlt_properties properties = mlt_frame_properties( *frame );

		// Pass the dv decoder
		mlt_properties_set_data( properties, "dv_decoder", this->dv_decoder, 0, NULL, NULL );

		// Pass the dv data
		mlt_properties_set_data( properties, "dv_data", data, frame_size_625_50, ( mlt_destructor )mlt_pool_release, NULL );

		// Update other info on the frame
		mlt_properties_set_int( properties, "width", 720 );
		mlt_properties_set_int( properties, "height", this->is_pal ? 576 : 480 );
		mlt_properties_set_int( properties, "top_field_first", 0 );
		
		// Parse the header for meta info
		dv_parse_header( this->dv_decoder, data );
		mlt_properties_set_int( properties, "progressive", dv_is_progressive( this->dv_decoder ) );
		mlt_properties_set_double( properties, "aspect_ratio", dv_format_wide( this->dv_decoder ) ? 16.0/9.0 : 4.0/3.0 );

		// Hmm - register audio callback
		( *frame )->get_audio = producer_get_audio;
		
		// Push the get_image method on to the stack
		mlt_frame_push_get_image( *frame, producer_get_image );
	}
	else
	{
		mlt_pool_release( data );
	}

	// Update timecode on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	// Obtain this
	producer_libdv this = parent->child;

	// Free the dv deconder
	//dv_decoder_free( this->dv_decoder );

	// Close the file
	if ( this->fd > 0 )
		close( this->fd );

	// Close the parent
	parent->close = NULL;
	mlt_producer_close( parent );

	// Free the memory
	free( this );
}

