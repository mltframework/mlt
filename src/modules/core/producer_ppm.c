/*
 * producer_ppm.c -- simple ppm test case
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

#include <stdlib.h>
#include <string.h>

typedef struct producer_ppm_s *producer_ppm;

struct producer_ppm_s
{
	struct mlt_producer_s parent;
	char *command;
	FILE *video;
	FILE *audio;
	uint64_t expected;
};

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

mlt_producer producer_ppm_init( mlt_profile profile, mlt_service_type type, const char *id, char *command )
{
	producer_ppm this = calloc( sizeof( struct producer_ppm_s ), 1 );
	if ( this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		mlt_producer producer = &this->parent;
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;

		if ( command != NULL )
		{
			mlt_properties_set( properties, "resource", command );
			this->command = strdup( command );
		}
		else
		{
			mlt_properties_set( properties, "resource", "ppm test" );
		}

		return producer;
	}
	free( this );
	return NULL;
}

static int producer_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the frames properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	if ( mlt_properties_get_int( properties, "has_image" ) )
	{
		// Get the RGB image
		*buffer = mlt_properties_get_data( properties, "image", NULL );
		*width = mlt_properties_get_int( properties, "width" );
		*height = mlt_properties_get_int( properties, "height" );
		*format = mlt_image_rgb24;
	}
	else
	{
		mlt_frame_get_image( this, buffer, format, width, height, writable );
	}

	return 0;
}

FILE *producer_ppm_run_video( producer_ppm this )
{
	if ( this->video == NULL )
	{
		if ( this->command == NULL )
		{
			this->video = popen( "image2raw -k -r 25 -ppm /usr/share/pixmaps/*.png", "r" );
		}
		else
		{
			char command[ 1024 ];
			float fps = mlt_producer_get_fps( &this->parent );
			float position = mlt_producer_position( &this->parent );
			sprintf( command, "ffmpeg -i \"%s\" -ss %f -f image2pipe -r %f -vcodec ppm - 2>/dev/null", this->command, position, fps );
			this->video = popen( command, "r" );
		}
	}
	return this->video;
}

FILE *producer_ppm_run_audio( producer_ppm this )
{
	if ( this->audio == NULL )
	{
		if ( this->command != NULL )
		{
			char command[ 1024 ];
			float position = mlt_producer_position( &this->parent );
			sprintf( command, "ffmpeg -i \"%s\" -ss %f -f s16le -ar 48000 -ac 2 - 2>/dev/null", this->command, position );
			this->audio = popen( command, "r" );
		}
	}
	return this->audio;
}

static void producer_ppm_position( producer_ppm this, uint64_t requested )
{
	if ( requested != this->expected )
	{
		if ( this->video != NULL )
			pclose( this->video );
		this->video = NULL;
		if ( this->audio != NULL )
			pclose( this->audio );
		this->audio = NULL;
	}

	// This is the next frame we expect
	this->expected = mlt_producer_frame( &this->parent ) + 1;

	// Open the pipe
	this->video = producer_ppm_run_video( this );

	// Open the audio pipe
	this->audio = producer_ppm_run_audio( this );

}

static int producer_get_audio( mlt_frame this, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the frames properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );

	FILE *pipe = mlt_properties_get_data( properties, "audio.pipe", NULL );

	*frequency = 48000;
	*channels = 2;
	*samples = 1920;

	// Size
	int size = *samples * *channels * 2;

	// Allocate an image
	*buffer = malloc( size );
		
	// Read it
	if ( pipe != NULL )
		size = fread( *buffer, size, 1, pipe );
	else
		memset( *buffer, 0, size );

	// Pass the data on the frame properties
	mlt_frame_set_audio( this, *buffer, *format, size, free );

	return 0;
}

static int read_ppm_header( FILE *video, int *width, int *height )
{
	int count = 0;
	{
		char temp[ 132 ];
		char *ignore = fgets( temp, 132, video );
		ignore = fgets( temp, 132, video );
		count += sscanf( temp, "%d %d", width, height );
		ignore = fgets( temp, 132, video );
	}
	return count;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	producer_ppm this = producer->child;
	int width;
	int height;

	// Construct a test frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	// Are we at the position expected?
	producer_ppm_position( this, mlt_producer_frame( producer ) );

	// Get the frames properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

	FILE *video = this->video;
	FILE *audio = this->audio;

	// Read the video
	if ( video != NULL && read_ppm_header( video, &width, &height ) == 2 )
	{
		// Allocate an image
		uint8_t *image = mlt_pool_alloc( width * ( height + 1 ) * 3 );
		
		// Read it
		size_t ignore;
		ignore = fread( image, width * height * 3, 1, video );

		// Pass the data on the frame properties
		mlt_frame_set_image( *frame, image, width * ( height + 1 ) * 3, mlt_pool_release );
		mlt_properties_set_int( properties, "width", width );
		mlt_properties_set_int( properties, "height", height );
		mlt_properties_set_int( properties, "has_image", 1 );
		mlt_properties_set_int( properties, "progressive", 1 );
		mlt_properties_set_double( properties, "aspect_ratio", 1 );

		// Push the image callback
		mlt_frame_push_get_image( *frame, producer_get_image );
	}
	else
	{
		// Push the image callback
		mlt_frame_push_get_image( *frame, producer_get_image );
	}

	// Set the audio pipe
	mlt_properties_set_data( properties, "audio.pipe", audio, 0, NULL, NULL );

	// Hmm - register audio callback
	mlt_frame_push_audio( *frame, producer_get_audio );

	// Update timecode on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	producer_ppm this = parent->child;
	if ( this->video )
		pclose( this->video );
	if ( this->audio )
		pclose( this->audio );
	free( this->command );
	parent->close = NULL;
	mlt_producer_close( parent );
	free( this );
}
