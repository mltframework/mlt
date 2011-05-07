/*
 * producer_libdv.c -- simple libdv test case
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt_producer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_deque.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_profile.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libdv/dv.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FRAME_SIZE_525_60 	10 * 150 * 80
#define FRAME_SIZE_625_50 	12 * 150 * 80

/** To conserve resources, we maintain a stack of dv decoders.
*/

static pthread_mutex_t decoder_lock = PTHREAD_MUTEX_INITIALIZER;
static mlt_properties dv_decoders = NULL;

dv_decoder_t *dv_decoder_alloc( )
{
	// We'll return a dv_decoder
	dv_decoder_t *this = NULL;

	// Lock the mutex
	pthread_mutex_lock( &decoder_lock );

	// Create the properties if necessary
	if ( dv_decoders == NULL )
	{
		// Create the properties
		dv_decoders = mlt_properties_new( );

		// Create the stack
		mlt_properties_set_data( dv_decoders, "stack", mlt_deque_init( ), 0, ( mlt_destructor )mlt_deque_close, NULL );

		// Register the properties for clean up
		mlt_factory_register_for_clean_up( dv_decoders, ( mlt_destructor )mlt_properties_close );
	}

	// Now try to obtain a decoder
	if ( dv_decoders != NULL )
	{
		// Obtain the stack
		mlt_deque stack = mlt_properties_get_data( dv_decoders, "stack", NULL );

		// Pop the top of the stack
		this = mlt_deque_pop_back( stack );

		// Create a new decoder if none available
		if ( this == NULL )
		{
			// We'll need a unique property ID for this
			char label[ 256 ];

			// Configure the decoder
			this = dv_decoder_new( FALSE, FALSE, FALSE );
			this->quality = DV_QUALITY_COLOR | DV_QUALITY_AC_2;
			this->audio->arg_audio_emphasis = 2;
			dv_set_audio_correction( this, DV_AUDIO_CORRECT_AVERAGE );
			dv_set_error_log( this, NULL );

			// Register it with the properties to ensure clean up
			sprintf( label, "%p", this );
			mlt_properties_set_data( dv_decoders, label, this, 0, ( mlt_destructor )dv_decoder_free, NULL );
		}
	}

	// Unlock the mutex
	pthread_mutex_unlock( &decoder_lock );

	return this;
}

void dv_decoder_return( dv_decoder_t *this )
{
	// Lock the mutex
	pthread_mutex_lock( &decoder_lock );

	// Now try to return the decoder
	if ( dv_decoders != NULL )
	{
		// Obtain the stack
		mlt_deque stack = mlt_properties_get_data( dv_decoders, "stack", NULL );

		// Push it back
		mlt_deque_push_back( stack, this );
	}

	// Unlock the mutex
	pthread_mutex_unlock( &decoder_lock );
}


typedef struct producer_libdv_s *producer_libdv;

struct producer_libdv_s
{
	struct mlt_producer_s parent;
	int fd;
	int is_pal;
	uint64_t file_size;
	int frame_size;
	long frames_in_file;
	mlt_producer alternative;
};

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

static int producer_collect_info( producer_libdv this, mlt_profile profile );

mlt_producer producer_libdv_init( mlt_profile profile, mlt_service_type type, const char *id, char *filename )
{
	producer_libdv this = calloc( sizeof( struct producer_libdv_s ), 1 );

	if ( filename != NULL && this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		int destroy = 0;
		mlt_producer producer = &this->parent;
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

		// Set the resource property (required for all producers)
		mlt_properties_set( properties, "resource", filename );

		// Register transport implementation with the producer
		producer->close = ( mlt_destructor )producer_close;

		// Register our get_frame implementation with the producer
		producer->get_frame = producer_get_frame;

		// If we have mov or dv, then we'll use an alternative producer
		if ( strchr( filename, '.' ) != NULL && (
			 strncasecmp( strrchr( filename, '.' ), ".avi", 4 ) == 0 ||
			 strncasecmp( strrchr( filename, '.' ), ".mov", 4 ) == 0 ) )
		{
			// Load via an alternative mechanism
			mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );
			this->alternative = mlt_factory_producer( profile, "kino", filename );

			// If it's unavailable, then clean up
			if ( this->alternative == NULL )
				destroy = 1;
			else
				mlt_properties_pass( properties, MLT_PRODUCER_PROPERTIES( this->alternative ), "" );
			this->is_pal = ( ( int ) mlt_producer_get_fps( producer ) ) == 25;
		}
		else
		{
			// Open the file if specified
			this->fd = open( filename, O_RDONLY );

			// Collect info
			if ( this->fd == -1 || !producer_collect_info( this, profile ) )
				destroy = 1;
		}

		// If we couldn't open the file, then destroy it now
		if ( destroy )
		{
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
	int result = read( fd, frame_buf, FRAME_SIZE_525_60 ) == FRAME_SIZE_525_60;
	if ( result )
	{
		*isPAL = ( frame_buf[3] & 0x80 );
		if ( *isPAL )
		{
			int diff = FRAME_SIZE_625_50 - FRAME_SIZE_525_60;
			result = read( fd, frame_buf + FRAME_SIZE_525_60, diff ) == diff;
		}
	}
	
	return result;
}

static int producer_collect_info( producer_libdv this, mlt_profile profile )
{
	int valid = 0;

	uint8_t *dv_data = mlt_pool_alloc( FRAME_SIZE_625_50 );

	if ( dv_data != NULL )
	{
		// Read the first frame
		valid = read_frame( this->fd, dv_data, &this->is_pal );

		// If it looks like a valid frame, the get stats
		if ( valid )
		{
			// Get the properties
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( &this->parent );

			// Get a dv_decoder
			dv_decoder_t *dv_decoder = dv_decoder_alloc( );

			// Determine the file size
			struct stat buf;
			fstat( this->fd, &buf );

			// Store the file size
			this->file_size = buf.st_size;

			// Determine the frame size
			this->frame_size = this->is_pal ? FRAME_SIZE_625_50 : FRAME_SIZE_525_60;

			// Determine the number of frames in the file
			this->frames_in_file = this->file_size / this->frame_size;

			// Calculate default in/out points
			int fps = 1000 * ( this->is_pal ? 25 : ( 30000.0 / 1001.0 ) );
			if ( ( int )( mlt_profile_fps( profile ) * 1000 ) == fps )
			{
				if ( this->frames_in_file > 0 )
				{
					mlt_properties_set_position( properties, "length", this->frames_in_file );
					mlt_properties_set_position( properties, "in", 0 );
					mlt_properties_set_position( properties, "out", this->frames_in_file - 1 );
				}
			}
			else
			{
				valid = 0;
			}

			// Parse the header for meta info
			dv_parse_header( dv_decoder, dv_data );
			mlt_properties_set_double( properties, "aspect_ratio", 
				dv_format_wide( dv_decoder ) ? ( this->is_pal ? 118.0/81.0 : 40.0/33.0 ) : ( this->is_pal ? 59.0/54.0 : 10.0/11.0 ) );
			mlt_properties_set_double( properties, "source_fps", this->is_pal ? 25 : ( 30000.0 / 1001.0 ) );
			mlt_properties_set_int( properties, "meta.media.nb_streams", 2 );
			mlt_properties_set_int( properties, "video_index", 0 );
			mlt_properties_set( properties, "meta.media.0.stream.type", "video" );
			mlt_properties_set( properties, "meta.media.0.codec.name", "dvvideo" );
			mlt_properties_set( properties, "meta.media.0.codec.long_name", "DV (Digital Video)" );
			mlt_properties_set_int( properties, "audio_index", 1 );
			mlt_properties_set( properties, "meta.media.1.stream.type", "audio" );
			mlt_properties_set( properties, "meta.media.1.codec.name", "pcm_s16le" );
			mlt_properties_set( properties, "meta.media.1.codec.long_name", "signed 16-bit little-endian PCM" );

			// Return the decoder
			dv_decoder_return( dv_decoder );
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
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	// Get a dv_decoder
	dv_decoder_t *decoder = dv_decoder_alloc( );

	// Get the dv data
	uint8_t *dv_data = mlt_properties_get_data( properties, "dv_data", NULL );

	// Get and set the quality request
	char *quality = mlt_frame_pop_service( this );

	if ( quality != NULL )
	{
		if ( strncmp( quality, "fast", 4 ) == 0 )
			decoder->quality = ( DV_QUALITY_COLOR | DV_QUALITY_DC );
		else if ( strncmp( quality, "best", 4 ) == 0 )
			decoder->quality = ( DV_QUALITY_COLOR | DV_QUALITY_AC_2 );
		else
			decoder->quality = ( DV_QUALITY_COLOR | DV_QUALITY_AC_1 );
	}

	// Parse the header for meta info
	dv_parse_header( decoder, dv_data );
	
	// Assign width and height according to the frame
	*width = 720;
	*height = dv_data[ 3 ] & 0x80 ? 576 : 480;

	// Extract an image of the format requested
	if ( *format == mlt_image_yuv422 || *format == mlt_image_yuv420p )
	{
		// Allocate an image
		uint8_t *image = mlt_pool_alloc( *width * ( *height + 1 ) * 2 );

		// Pass to properties for clean up
		mlt_frame_set_image( this, image, *width * ( *height + 1 ) * 2, mlt_pool_release );

		// Decode the image
		pitches[ 0 ] = *width * 2;
		pixels[ 0 ] = image;
		dv_decode_full_frame( decoder, dv_data, e_dv_color_yuv, pixels, pitches );

		// Assign result
		*buffer = image;
		*format = mlt_image_yuv422;
	}
	else
	{
		// Allocate an image
		uint8_t *image = mlt_pool_alloc( *width * ( *height + 1 ) * 3 );

		// Pass to properties for clean up
		mlt_frame_set_image( this, image, *width * ( *height + 1 ) * 3, mlt_pool_release );

		// Decode the frame
		pitches[ 0 ] = 720 * 3;
		pixels[ 0 ] = image;
		dv_decode_full_frame( decoder, dv_data, e_dv_color_rgb, pixels, pitches );

		// Assign result
		*buffer = image;
		*format = mlt_image_rgb24;
	}

	// Return the decoder
	dv_decoder_return( decoder );

	return 0;
}

static int producer_get_audio( mlt_frame this, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	int16_t *p;
	int i, j;
	int16_t *audio_channels[ 4 ];
	
	// Get the frames properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	// Get a dv_decoder
	dv_decoder_t *decoder = dv_decoder_alloc( );

	// Get the dv data
	uint8_t *dv_data = mlt_properties_get_data( properties, "dv_data", NULL );

	// Parse the header for meta info
	dv_parse_header( decoder, dv_data );

	// Check that we have audio
	if ( decoder->audio->num_channels > 0 )
	{
		int size = *channels * DV_AUDIO_MAX_SAMPLES * sizeof( int16_t );

		// Obtain required values
		*frequency = decoder->audio->frequency;
		*samples = decoder->audio->samples_this_frame;
		*channels = decoder->audio->num_channels;
		*format = mlt_audio_s16;

		// Create a temporary workspace
		for ( i = 0; i < 4; i++ )
			audio_channels[ i ] = mlt_pool_alloc( DV_AUDIO_MAX_SAMPLES * sizeof( int16_t ) );
	
		// Create a workspace for the result
		*buffer = mlt_pool_alloc( size );
	
		// Pass the allocated audio buffer as a property
		mlt_frame_set_audio( this, *buffer, *format, size, mlt_pool_release );
	
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
	}
	else
	{
		// No audio available on the frame, so get test audio (silence)
		mlt_frame_get_audio( this, buffer, format, frequency, channels, samples );
	}

	// Return the decoder
	dv_decoder_return( decoder );

	return 0;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Access the private data
	producer_libdv this = producer->child;

	// Will carry the frame data
	uint8_t *data = NULL;

	// Obtain the current frame number
	uint64_t position = mlt_producer_frame( producer );
	
	if ( this->alternative == NULL )
	{
		// Convert timecode to a file position (ensuring that we're on a frame boundary)
		uint64_t offset = position * this->frame_size;

		// Allocate space
		data = mlt_pool_alloc( FRAME_SIZE_625_50 );

		// Create an empty frame
		*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

		// Seek and fetch
		if ( this->fd != 0 && 
		 	 lseek( this->fd, offset, SEEK_SET ) == offset &&
		 	 read_frame( this->fd, data, &this->is_pal ) )
		{
			// Pass the dv data
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( *frame ), "dv_data", data, FRAME_SIZE_625_50, ( mlt_destructor )mlt_pool_release, NULL );
		}
		else
		{
			mlt_pool_release( data );
			data = NULL;
		}
	}
	else
	{
		// Seek
		mlt_producer_seek( this->alternative, position );
		
		// Fetch
		mlt_service_get_frame( MLT_PRODUCER_SERVICE( this->alternative ), frame, 0 );

		// Verify
		if ( *frame != NULL )
			data = mlt_properties_get_data( MLT_FRAME_PROPERTIES( *frame ), "dv_data", NULL );
	}

	if ( data != NULL )
	{
		// Get the frames properties
		mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );
	
		// Get a dv_decoder
		dv_decoder_t *dv_decoder = dv_decoder_alloc( );

		mlt_properties_set_int( properties, "test_image", 0 );
		mlt_properties_set_int( properties, "test_audio", 0 );

		// Update other info on the frame
		mlt_properties_set_int( properties, "width", 720 );
		mlt_properties_set_int( properties, "height", this->is_pal ? 576 : 480 );
		mlt_properties_set_int( properties, "real_width", 720 );
		mlt_properties_set_int( properties, "real_height", this->is_pal ? 576 : 480 );
		mlt_properties_set_int( properties, "top_field_first", !this->is_pal ? 0 : ( data[ 5 ] & 0x07 ) == 0 ? 0 : 1 );
		mlt_properties_set_int( properties, "colorspace", 601 );
	
		// Parse the header for meta info
		dv_parse_header( dv_decoder, data );
		//mlt_properties_set_int( properties, "progressive", dv_is_progressive( dv_decoder ) );
		mlt_properties_set_double( properties, "aspect_ratio", 
				dv_format_wide( dv_decoder ) ? ( this->is_pal ? 118.0/81.0 : 40.0/33.0 ) : ( this->is_pal ? 59.0/54.0 : 10.0/11.0 ) );
	

		mlt_properties_set_int( properties, "audio_frequency", dv_decoder->audio->frequency );
		mlt_properties_set_int( properties, "audio_channels", dv_decoder->audio->num_channels );

		// Register audio callback
		if ( mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( producer ), "audio_index" ) > 0 )
			mlt_frame_push_audio( *frame, producer_get_audio );
	
		if ( mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( producer ), "video_index" ) > -1 )
		{
			// Push the quality string
			mlt_frame_push_service( *frame, mlt_properties_get( MLT_PRODUCER_PROPERTIES( producer ), "quality" ) );

			// Push the get_image method on to the stack
			mlt_frame_push_get_image( *frame, producer_get_image );
		}
	
		// Return the decoder
		dv_decoder_return( dv_decoder );
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

	// Close the file
	if ( this->fd > 0 )
		close( this->fd );

	if ( this->alternative )
		mlt_producer_close( this->alternative );

	// Close the parent
	parent->close = NULL;
	mlt_producer_close( parent );

	// Free the memory
	free( this );
}
