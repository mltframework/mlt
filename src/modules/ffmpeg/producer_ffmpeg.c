/*
 * producer_ffmpeg.c -- simple ffmpeg test case
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

#include "producer_ffmpeg.h"
#include <framework/mlt_frame.h>
#include <stdlib.h>
#include <string.h>

typedef struct producer_ffmpeg_s *producer_ffmpeg;

struct producer_ffmpeg_s
{
	struct mlt_producer_s parent;
	char *command;
	FILE *video;
	FILE *audio;
	uint64_t expected;
	uint8_t *buffer;
	int open;
	int width;
	int height;
};

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

mlt_producer producer_ffmpeg_init( char *command )
{
	producer_ffmpeg this = calloc( sizeof( struct producer_ffmpeg_s ), 1 );
	if ( this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		mlt_producer producer = &this->parent;

		producer->get_frame = producer_get_frame;
		producer->close = producer_close;

		if ( command != NULL )
			this->command = strdup( command );

		this->buffer = malloc( 1024 * 1024 );

		return producer;
	}
	free( this );
	return NULL;
}

static int producer_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the frames properties
	mlt_properties properties = mlt_frame_properties( this );

	if ( mlt_properties_get_int( properties, "has_image" ) )
	{
		// Get width and height
		*format = mlt_image_yuv422;
		*width = mlt_properties_get_int( properties, "width" );
		*height = mlt_properties_get_int( properties, "height" );

		// Specify format and image
		*buffer = mlt_properties_get_data( properties, "image", NULL );
	}
	else
	{
		mlt_frame_get_image( this, buffer, format, width, height, writable );
	}

	return 0;
}

FILE *producer_ffmpeg_run_video( producer_ffmpeg this )
{
	if ( this->video == NULL && !this->open )
	{
		if ( this->command != NULL )
		{
			char command[ 1024 ];
			float fps = mlt_producer_get_fps( &this->parent );
			float position = mlt_producer_position( &this->parent );
			sprintf( command, "ffmpeg -i \"%s\" -ss %f -f imagepipe -r %f -f yuv4mpegpipe - 2>/dev/null", this->command, position, fps );
			this->video = popen( command, "r" );
		}
	}
	return this->video;
}

FILE *producer_ffmpeg_run_audio( producer_ffmpeg this )
{
	if ( this->audio == NULL && !this->open )
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

static void producer_ffmpeg_position( producer_ffmpeg this, uint64_t requested )
{
	if ( requested != this->expected )
	{
		if ( this->video != NULL )
			pclose( this->video );
		this->video = NULL;
		if ( this->audio != NULL )
			pclose( this->audio );
		this->audio = NULL;
	
		// We should not be open now
		this->open = 0;
	}

	// This is the next frame we expect
	this->expected = mlt_producer_frame( &this->parent ) + 1;

	// Open the pipe
	this->video = producer_ffmpeg_run_video( this );

	// Open the audio pipe
	this->audio = producer_ffmpeg_run_audio( this );

	// We should be open now
	this->open = 1;
}

static int producer_get_audio( mlt_frame this, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the frames properties
	mlt_properties properties = mlt_frame_properties( this );

	producer_ffmpeg producer = mlt_properties_get_data( properties, "producer_ffmpeg", NULL );

	*frequency = 48000;
	*channels = 2;
	*samples = 1920;

	// Size
	int size = *samples * *channels * 2;

	// Allocate an image
	*buffer = malloc( size );
		
	// Read it
	if ( producer->audio != NULL )
	{
		if ( fread( *buffer, size, 1, producer->audio ) != 1 )
		{
			pclose( producer->audio );
			producer->audio = NULL;
		}
	}
	else
	{
		memset( *buffer, 0, size );
	}

	// Pass the data on the frame properties
	mlt_properties_set_data( properties, "audio", *buffer, size, free, NULL );

	return 0;
}

static int read_ffmpeg_header( producer_ffmpeg this, int *width, int *height )
{
	int count = 0;
	char temp[ 132 ];
	FILE *video = this->video;

	if ( fgets( temp, 132, video ) )
	{
		if ( strncmp( temp, "FRAME", 5 ) )
		{
			if ( strstr( temp, " W" ) != NULL )
				*width = atoi( strstr( temp, " W" ) + 2 );
			if ( strstr( temp, " H" ) != NULL )
				*height = atoi( strstr( temp, " H" ) + 2 );
			count = 2;
			fgets( temp, 132, video );
			this->width = *width;
			this->height = *height;
		}
		else
		{
			*width = this->width;
			*height = this->height;
			count = 2;
		}
	}
	return count;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	producer_ffmpeg this = producer->child;
	int width;
	int height;

	// Construct a test frame
	*frame = mlt_frame_init( );

	// Are we at the position expected?
	producer_ffmpeg_position( this, mlt_producer_frame( producer ) );

	// Get the frames properties
	mlt_properties properties = mlt_frame_properties( *frame );

	FILE *video = this->video;

	// Read the video
	if ( video != NULL && read_ffmpeg_header( this, &width, &height ) == 2 )
	{
		// Allocate an image
		uint8_t *image = malloc( width * height * 2 );
		
		// Read it
		fread( this->buffer, width * height * 3 / 2, 1, video );

		// Convert it
		mlt_convert_yuv420p_to_yuv422( this->buffer, width, height, width, image );

		// Pass the data on the frame properties
		mlt_properties_set_data( properties, "image", image, width * height * 2, free, NULL );
		mlt_properties_set_int( properties, "width", width );
		mlt_properties_set_int( properties, "height", height );
		mlt_properties_set_int( properties, "has_image", 1 );

		// Push the image callback
		mlt_frame_push_get_image( *frame, producer_get_image );
	}
	else
	{
		// Clean up
		if ( this->video != NULL )
		{
			pclose( this->video );
			this->video = NULL;
		}

		// Push the image callback
		mlt_frame_push_get_image( *frame, producer_get_image );
	}

	// Set the audio pipe
	mlt_properties_set_data( properties, "producer_ffmpeg", this, 0, NULL, NULL );

	// Hmm - register audio callback
	( *frame )->get_audio = producer_get_audio;

	// Update timecode on the frame we're creating
	mlt_frame_set_timecode( *frame, mlt_producer_position( producer ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	producer_ffmpeg this = parent->child;
	if ( this->video )
		pclose( this->video );
	if ( this->audio )
		pclose( this->audio );
	free( this->command );
	parent->close = NULL;
	mlt_producer_close( parent );
	free( this );
}

