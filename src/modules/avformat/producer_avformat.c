/*
 * producer_avformat.c -- avformat producer
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

// Local header files
#include "producer_avformat.h"

// MLT Header files
#include <framework/mlt_frame.h>

// ffmpeg Header files
#include <ffmpeg/avformat.h>

// System header files
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

// Forward references.
static int producer_open( mlt_producer this, char *file );
static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index );

// A static flag used to determine if avformat has been initialised
static int avformat_initialised = 0;
static pthread_mutex_t avformat_mutex;

#if 0
void *av_malloc( unsigned int size )
{
	return mlt_pool_alloc( size );
}

void *av_realloc( void *ptr, unsigned int size )
{
	return mlt_pool_realloc( ptr, size );
}

void av_free( void *ptr )
{
	return mlt_pool_release( ptr );
}
#endif

/** Constructor for libavformat.
*/

mlt_producer producer_avformat_init( char *file )
{
	mlt_producer this = NULL;

	// Check that we have a non-NULL argument
	if ( file != NULL )
	{
		// Construct the producer
		this = calloc( 1, sizeof( struct mlt_producer_s ) );

		// Initialise it
		if ( mlt_producer_init( this, NULL ) == 0 )
		{
			// Get the properties
			mlt_properties properties = mlt_producer_properties( this );

			// Set the resource property (required for all producers)
			mlt_properties_set( properties, "resource", file );

			// TEST: audio sync tweaking
			mlt_properties_set_double( properties, "discrepancy", 1 );

			// Register our get_frame implementation
			this->get_frame = producer_get_frame;

			// Initialise avformat if necessary
			if ( avformat_initialised == 0 )
			{
				avformat_initialised = 1;
				pthread_mutex_init( &avformat_mutex, NULL );
				av_register_all( );
			}

			// Open the file
			if ( producer_open( this, file ) != 0 )
			{
				// Clean up
				mlt_producer_close( this );
				this = NULL;
			}
		}
	}

	return this;
}

/** Find the default streams.
*/

static void find_default_streams( AVFormatContext *context, int *audio_index, int *video_index )
{
	int i;

	// Allow for multiple audio and video streams in the file and select first of each (if available)
	for( i = 0; i < context->nb_streams; i++ ) 
	{
		// Get the codec context
   		AVCodecContext *codec_context = &context->streams[ i ]->codec;

		// Determine the type and obtain the first index of each type
   		switch( codec_context->codec_type ) 
		{
			case CODEC_TYPE_VIDEO:
				if ( *video_index < 0 )
					*video_index = i;
				break;
			case CODEC_TYPE_AUDIO:
				if ( *audio_index < 0 )
					*audio_index = i;
		   		break;
			default:
		   		break;
		}
	}
}

/** Producer file destructor.
*/

static void producer_file_close( void *context )
{
	if ( context != NULL )
	{
		// Lock the mutex now
		pthread_mutex_lock( &avformat_mutex );

		// Close the file
		av_close_input_file( context );

		// Unlock the mutex now
		pthread_mutex_unlock( &avformat_mutex );
	}
}

/** Producer file destructor.
*/

static void producer_codec_close( void *codec )
{
	if ( codec != NULL )
	{
		// Lock the mutex now
		pthread_mutex_lock( &avformat_mutex );

		// Close the file
		avcodec_close( codec );

		// Unlock the mutex now
		pthread_mutex_unlock( &avformat_mutex );
	}
}

/** Open the file.
*/

static int producer_open( mlt_producer this, char *file )
{
	// Return an error code (0 == no error)
	int error = 0;

	// Context for avformat
	AVFormatContext *context = NULL;

	// Get the properties
	mlt_properties properties = mlt_producer_properties( this );

	// We will treat everything with the producer fps
	double fps = mlt_properties_get_double( properties, "fps" );

	// Lock the mutex now
	pthread_mutex_lock( &avformat_mutex );
	
	// If "MRL", then create AVInputFormat
	AVInputFormat *format = NULL;
	AVFormatParameters *params = NULL;
	char *standard = NULL;
	char *mrl = strchr( file, ':' );
	
	// Only if there is not a protocol specification that avformat can handle
	if ( mrl && !url_exist( file ) )
	{
		// 'file' becomes format abbreviation
		mrl[0] = 0;
	
		// Lookup the format
		format = av_find_input_format( file );
		
		// Eat the format designator
		file = ++mrl;
		
		if ( format )
		{
			// Allocate params
			params = calloc( sizeof( AVFormatParameters ), 1 );
			
			// These are required by video4linux (defaults)
			params->width = 640;
			params->height = 480;
			params->frame_rate = 25;
			params->frame_rate_base = 1;
			params->device = file;
			params->channels = 2;
			params->sample_rate = 48000;
		}
		
		// Parse out params
		mrl = strchr( file, '?' );
		while ( mrl )
		{
			mrl[0] = 0;
			char *name = strdup( ++mrl );
			char *value = strchr( name, ':' );
			if ( value )
			{
				value[0] = 0;
				value++;
				char *t = strchr( value, '&' );
				if ( t )
					t[0] = 0;
				if ( !strcmp( name, "frame_rate" ) )
					params->frame_rate = atoi( value );
				else if ( !strcmp( name, "frame_rate_base" ) )
					params->frame_rate_base = atoi( value );
				else if ( !strcmp( name, "sample_rate" ) )
					params->sample_rate = atoi( value );
				else if ( !strcmp( name, "channels" ) )
					params->channels = atoi( value );
				else if ( !strcmp( name, "width" ) )
					params->width = atoi( value );
				else if ( !strcmp( name, "height" ) )
					params->height = atoi( value );
				else if ( !strcmp( name, "standard" ) )
				{
					standard = strdup( value );
					params->standard = standard;
				}
			}
			free( name );
			mrl = strchr( mrl, '&' );
		}
	}

	// Now attempt to open the file
	error = av_open_input_file( &context, file, format, 0, params );
	error = error < 0;
	
	// Cleanup AVFormatParameters
	free( standard );
	free( params );

	// If successful, then try to get additional info
	if ( error == 0 )
	{
		// Get the stream info
		error = av_find_stream_info( context ) < 0;

		// Continue if no error
		if ( error == 0 )
		{
			// We will default to the first audio and video streams found
			int audio_index = -1;
			int video_index = -1;

			// Now set properties where we can (use default unknowns if required)
			if ( context->duration != AV_NOPTS_VALUE ) 
			{
				// This isn't going to be accurate for all formats
				mlt_position frames = ( mlt_position )( ( ( double )context->duration / ( double )AV_TIME_BASE ) * fps );
				mlt_properties_set_position( properties, "out", frames - 2 );
				mlt_properties_set_position( properties, "length", frames - 1 );
			}

			// Find default audio and video streams
			find_default_streams( context, &audio_index, &video_index );

			// Store selected audio and video indexes on properties
			mlt_properties_set_int( properties, "audio_index", audio_index );
			mlt_properties_set_int( properties, "video_index", video_index );
			
			// We're going to cheat here - for a/v files, we will have two contexts (reasoning will be clear later)
			if ( audio_index != -1 && video_index != -1 )
			{
				// We'll use the open one as our video_context
				mlt_properties_set_data( properties, "video_context", context, 0, producer_file_close, NULL );

				// And open again for our audio context
				av_open_input_file( &context, file, NULL, 0, NULL );
				av_find_stream_info( context );

				// Audio context
				mlt_properties_set_data( properties, "audio_context", context, 0, producer_file_close, NULL );
			}
			else if ( video_index != -1 )
			{
				// We only have a video context
				mlt_properties_set_data( properties, "video_context", context, 0, producer_file_close, NULL );
			}
			else if ( audio_index != -1 )
			{
				// We only have an audio context
				mlt_properties_set_data( properties, "audio_context", context, 0, producer_file_close, NULL );
			}
			else
			{
				// Something has gone wrong
				error = -1;
			}
		}
	}

	// Unlock the mutex now
	pthread_mutex_unlock( &avformat_mutex );

	return error;
}

/** Convert a frame position to a time code.
*/

static double producer_time_of_frame( mlt_producer this, mlt_position position )
{
	// Get the properties
	mlt_properties properties = mlt_producer_properties( this );

	// Obtain the fps
	double fps = mlt_properties_get_double( properties, "fps" );

	// Do the calc
	return ( double )position / fps;
}

/** Get an image from a frame.
*/

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the properties from the frame
	mlt_properties frame_properties = mlt_frame_properties( frame );

	// Obtain the frame number of this frame
	mlt_position position = mlt_properties_get_position( frame_properties, "avformat_position" );

	// Get the producer 
	mlt_producer this = mlt_properties_get_data( frame_properties, "avformat_producer", NULL );

	// Get the producer properties
	mlt_properties properties = mlt_producer_properties( this );

	// Fetch the video_context
	AVFormatContext *context = mlt_properties_get_data( properties, "video_context", NULL );

	// Get the video_index
	int index = mlt_properties_get_int( properties, "video_index" );

	// Obtain the expected frame numer
	mlt_position expected = mlt_properties_get_position( properties, "video_expected" );

	// Calculate the real time code
	double real_timecode = producer_time_of_frame( this, position );

	// Get the video stream
	AVStream *stream = context->streams[ index ];

	// Get codec context
	AVCodecContext *codec_context = &stream->codec;

	// Packet
	AVPacket pkt;

	// Get the conversion frame
	AVPicture *output = mlt_properties_get_data( properties, "video_output_frame", NULL );

	// Special case pause handling flag
	int paused = 0;

	// Special case ffwd handling
	int ignore = 0;

	// Current time calcs
	double current_time = mlt_properties_get_double( properties, "current_time" );

	// We may want to use the source fps if available
	double source_fps = mlt_properties_get_double( properties, "source_fps" );

	// Set the result arguments that we know here (only *buffer is now required)
	*format = mlt_image_yuv422;
	*width = codec_context->width;
	*height = codec_context->height;

	// Set this on the frame properties
	mlt_properties_set_int( frame_properties, "width", *width );
	mlt_properties_set_int( frame_properties, "height", *height );

	// Lock the mutex now
	pthread_mutex_lock( &avformat_mutex );

	// Construct an AVFrame for YUV422 conversion
	if ( output == NULL )
	{
		int size = avpicture_get_size( PIX_FMT_YUV422, *width, *height );
		size += *width * 2;
		uint8_t *buf = mlt_pool_alloc( size );
		output = mlt_pool_alloc( sizeof( AVPicture ) );
		//memset( output, 0, sizeof( AVPicture ) );
		avpicture_fill( output, buf, PIX_FMT_YUV422, *width, *height );
		mlt_properties_set_data( properties, "video_output_frame", output, 0, ( mlt_destructor )mlt_pool_release, NULL );
		mlt_properties_set_data( properties, "video_output_buffer", buf, 0, ( mlt_destructor )mlt_pool_release, NULL );
	}

	// Seek if necessary
	if ( position != expected )
	{
		if ( position + 1 == expected )
		{
			// We're paused - use last image
			paused = 1;
		}
		else if ( position > expected && ( position - expected ) < 250 )
		{
			// Fast forward - seeking is inefficient for small distances - just ignore following frames
			ignore = position - expected;
		}
		else
		{
			// Set to the real timecode
			av_seek_frame( context, -1, real_timecode * 1000000.0 );
	
			// Remove the cached info relating to the previous position
			mlt_properties_set_double( properties, "current_time", real_timecode );
			mlt_properties_set_data( properties, "current_image", NULL, 0, NULL, NULL );
		}
	}
	
	// Duplicate the last image if necessary
	if ( mlt_properties_get_data( properties, "current_image", NULL ) != NULL &&
		 ( paused || mlt_properties_get_double( properties, "current_time" ) >= real_timecode ) )
	{
		// Get current image and size
		int size = 0;
		uint8_t *image = mlt_properties_get_data( properties, "current_image", &size );

		// Duplicate it
		*buffer = mlt_pool_alloc( size );
		memcpy( *buffer, image, size );

		// Set this on the frame properties
		mlt_properties_set_data( frame_properties, "image", *buffer, size, ( mlt_destructor )mlt_pool_release, NULL );
	}
	else
	{
		int ret = 0;
		int got_picture = 0;
		AVFrame frame;

		memset( &pkt, 0, sizeof( pkt ) );
		memset( &frame, 0, sizeof( frame ) );

		while( ret >= 0 && !got_picture )
		{
			// Read a packet
			ret = av_read_frame( context, &pkt );

			// We only deal with video from the selected video_index
			if ( ret >= 0 && pkt.stream_index == index && pkt.size > 0 )
			{
				// Decode the image
				ret = avcodec_decode_video( codec_context, &frame, &got_picture, pkt.data, pkt.size );

				if ( got_picture )
				{
					if ( pkt.pts != AV_NOPTS_VALUE && pkt.pts != 0  )
						current_time = ( double )pkt.pts / 1000000.0;
					else
						current_time = real_timecode;

					// Handle ignore
					if ( current_time < real_timecode )
					{
						ignore = 0;
						got_picture = 0;
					}
					else if ( current_time >= real_timecode )
					{
						//current_time = real_timecode;
						ignore = 0;
					}
					else if ( ignore -- )
					{
						got_picture = 0;
					}
				}
			}

			// We're finished with this packet regardless
			av_free_packet( &pkt );
		}

		// Now handle the picture if we have one
		if ( got_picture )
		{
			// Get current image and size
			int size = 0;
			uint8_t *image = mlt_properties_get_data( properties, "current_image", &size );

			if ( image == NULL || size != *width * *height * 2 )
			{
				size = *width * ( *height + 1 ) * 2;
				image = mlt_pool_alloc( size );
				mlt_properties_set_data( properties, "current_image", image, size, ( mlt_destructor )mlt_pool_release, NULL );
			}

			*buffer = mlt_pool_alloc( size );

			// EXPERIMENTAL IMAGE NORMALISATIONS
			if ( codec_context->pix_fmt == PIX_FMT_YUV420P )
			{
				register int i, j;
				register int half = *width >> 1;
				register uint8_t *Y = ( ( AVPicture * )&frame )->data[ 0 ];
				register uint8_t *U = ( ( AVPicture * )&frame )->data[ 1 ];
				register uint8_t *V = ( ( AVPicture * )&frame )->data[ 2 ];
				register uint8_t *d = *buffer;
				register uint8_t *y, *u, *v;

				i = *height >> 1;
				while ( i -- )
				{
					y = Y;
					u = U;
					v = V;
					j = half;
					while ( j -- )
					{
						*d ++ = *y ++;
						*d ++ = *u ++;
						*d ++ = *y ++;
						*d ++ = *v ++;
					}

					Y += ( ( AVPicture * )&frame )->linesize[ 0 ];
					y = Y;
					u = U;
					v = V;
					j = half;
					while ( j -- )
					{
						*d ++ = *y ++;
						*d ++ = *u ++;
						*d ++ = *y ++;
						*d ++ = *v ++;
					}

					Y += ( ( AVPicture * )&frame )->linesize[ 0 ];
					U += ( ( AVPicture * )&frame )->linesize[ 1 ];
					V += ( ( AVPicture * )&frame )->linesize[ 2 ];
				}
			}
			else
			{
				img_convert( output, PIX_FMT_YUV422, (AVPicture *)&frame, codec_context->pix_fmt, *width, *height );
				memcpy( *buffer, output->data[ 0 ], size );
			}

			memcpy( image, *buffer, size );
			mlt_properties_set_data( frame_properties, "image", *buffer, size, ( mlt_destructor )mlt_pool_release, NULL );

			if ( current_time == 0 && source_fps != 0 )
			{
				double fps = mlt_properties_get_double( properties, "fps" );
				current_time = ceil( source_fps * ( double )position / fps ) * ( 1 / source_fps );
				mlt_properties_set_double( properties, "current_time", current_time );
			}
			else
			{
				mlt_properties_set_double( properties, "current_time", current_time );
			}
		}
	}

	// Regardless of speed, we expect to get the next frame (cos we ain't too bright)
	mlt_properties_set_position( properties, "video_expected", position + 1 );

	// Unlock the mutex now
	pthread_mutex_unlock( &avformat_mutex );

	return 0;
}

/** Set up video handling.
*/

static void producer_set_up_video( mlt_producer this, mlt_frame frame )
{
	// Get the properties
	mlt_properties properties = mlt_producer_properties( this );

	// Fetch the video_context
	AVFormatContext *context = mlt_properties_get_data( properties, "video_context", NULL );

	// Get the video_index
	int index = mlt_properties_get_int( properties, "video_index" );

	// Get the frame properties
	mlt_properties frame_properties = mlt_frame_properties( frame );

	// Lock the mutex now
	pthread_mutex_lock( &avformat_mutex );

	if ( context != NULL && index != -1 )
	{
		// Get the video stream
		AVStream *stream = context->streams[ index ];

		// Get codec context
		AVCodecContext *codec_context = &stream->codec;

		// Get the codec
		AVCodec *codec = mlt_properties_get_data( properties, "video_codec", NULL );

		// Initialise the codec if necessary
		if ( codec == NULL )
		{
			// Find the codec
			codec = avcodec_find_decoder( codec_context->codec_id );

			// If we don't have a codec and we can't initialise it, we can't do much more...
			if ( codec != NULL && avcodec_open( codec_context, codec ) >= 0 )
			{
				// Now store the codec with its destructor
				mlt_properties_set_data( properties, "video_codec", codec_context, 0, producer_codec_close, NULL );
			}
			else
			{
				// Remember that we can't use this later
				mlt_properties_set_int( properties, "video_index", -1 );
			}
		}

		// No codec, no show...
		if ( codec != NULL )
		{
			double aspect_ratio = 1;
			double source_fps = 0;

			// Set aspect ratio
			if ( codec_context->sample_aspect_ratio.num > 0 )
				aspect_ratio = av_q2d( codec_context->sample_aspect_ratio );

			mlt_properties_set_double( properties, "aspect_ratio", aspect_ratio );
			//fprintf( stderr, "AVFORMAT: sample aspect %f %dx%d\n", av_q2d( codec_context->sample_aspect_ratio ), codec_context->width, codec_context->height );

			// Determine the fps
			source_fps = ( double )codec_context->frame_rate / ( codec_context->frame_rate_base == 0 ? 1 : codec_context->frame_rate_base );

			// We'll use fps if it's available
			if ( source_fps > 0 && source_fps < 30 )
				mlt_properties_set_double( properties, "source_fps", source_fps );
			
			// Set the width and height
			mlt_properties_set_int( frame_properties, "width", codec_context->width );
			mlt_properties_set_int( frame_properties, "height", codec_context->height );

			mlt_frame_push_get_image( frame, producer_get_image );
			mlt_properties_set_data( frame_properties, "avformat_producer", this, 0, NULL, NULL );
		}
		else
		{
			mlt_properties_set_int( frame_properties, "test_image", 1 );
		}
	}
	else
	{
		mlt_properties_set_int( frame_properties, "test_image", 1 );
	}

	// Unlock the mutex now
	pthread_mutex_unlock( &avformat_mutex );
}

/** Get the audio from a frame.
*/

static int producer_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the properties from the frame
	mlt_properties frame_properties = mlt_frame_properties( frame );

	// Obtain the frame number of this frame
	mlt_position position = mlt_properties_get_position( frame_properties, "avformat_position" );

	// Get the producer 
	mlt_producer this = mlt_properties_get_data( frame_properties, "avformat_producer", NULL );

	// Get the producer properties
	mlt_properties properties = mlt_producer_properties( this );

	// Fetch the audio_context
	AVFormatContext *context = mlt_properties_get_data( properties, "audio_context", NULL );

	// Get the audio_index
	int index = mlt_properties_get_int( properties, "audio_index" );

	// Obtain the expected frame numer
	mlt_position expected = mlt_properties_get_position( properties, "audio_expected" );

	// Obtain the resample context if it exists (not always needed)
	ReSampleContext *resample = mlt_properties_get_data( properties, "audio_resample", NULL );

	// Obtain the audio buffer
	int16_t *audio_buffer = mlt_properties_get_data( properties, "audio_buffer", NULL );

	// Get amount of audio used
	int audio_used =  mlt_properties_get_int( properties, "audio_used" );

	// Calculate the real time code
	double real_timecode = producer_time_of_frame( this, position );

	// Get the audio stream
	AVStream *stream = context->streams[ index ];

	// Get codec context
	AVCodecContext *codec_context = &stream->codec;

	// Packet
	AVPacket pkt;

	// Number of frames to ignore (for ffwd)
	int ignore = 0;

	// Flag for paused (silence) 
	int paused = 0;
	int locked = 0;

	// Lock the mutex now
	pthread_mutex_lock( &avformat_mutex );

	// Check for resample and create if necessary
	if ( resample == NULL && codec_context->channels <= 2 )
	{
		// Create the resampler
		resample = audio_resample_init( *channels, codec_context->channels, *frequency, codec_context->sample_rate );

		// And store it on properties
		mlt_properties_set_data( properties, "audio_resample", resample, 0, ( mlt_destructor )audio_resample_close, NULL );
	}
	else if ( resample == NULL )
	{
		*channels = codec_context->channels;
		*frequency = codec_context->sample_rate;
	}

	// Check for audio buffer and create if necessary
	if ( audio_buffer == NULL )
	{
		// Allocate the audio buffer
		audio_buffer = mlt_pool_alloc( AVCODEC_MAX_AUDIO_FRAME_SIZE * sizeof( int16_t ) );

		// And store it on properties for reuse
		mlt_properties_set_data( properties, "audio_buffer", audio_buffer, 0, ( mlt_destructor )mlt_pool_release, NULL );
	}

	// Seek if necessary
	if ( position != expected )
	{
		if ( position + 1 == expected )
		{
			// We're paused - silence required
			paused = 1;
		}
		else if ( position > expected && ( position - expected ) < 250 )
		{
			// Fast forward - seeking is inefficient for small distances - just ignore following frames
			ignore = position - expected;
		}
		else
		{
			// Set to the real timecode
			av_seek_frame( context, -1, real_timecode * 1000000.0 );

			// Clear the usage in the audio buffer
			audio_used = 0;

			locked = 1;
		}
	}

	// Get the audio if required
	if ( !paused )
	{
		int ret = 0;
		int got_audio = 0;
		int16_t *temp = mlt_pool_alloc( sizeof( int16_t ) * AVCODEC_MAX_AUDIO_FRAME_SIZE );

		memset( &pkt, 0, sizeof( pkt ) );

		while( ret >= 0 && !got_audio )
		{
			// Check if the buffer already contains the samples required
			if ( audio_used >= *samples && ignore == 0 )
			{
				got_audio = 1;
				break;
			}

			// Read a packet
			ret = av_read_frame( context, &pkt );

    		int len = pkt.size;
    		uint8_t *ptr = pkt.data;
			int data_size;

			// We only deal with audio from the selected audio_index
			while ( ptr != NULL && ret >= 0 && pkt.stream_index == index && len > 0 )
			{
				// Decode the audio
				ret = avcodec_decode_audio( codec_context, temp, &data_size, ptr, len );

				if ( ret < 0 )
				{
					ret = 0;
					break;
				}

				len -= ret;
				ptr += ret;

				if ( data_size > 0 )
				{
					if ( resample != NULL )
					{
						audio_used += audio_resample( resample, &audio_buffer[ audio_used * *channels ], temp, data_size / ( codec_context->channels * sizeof( int16_t ) ) );
					}
					else
					{
						memcpy( &audio_buffer[ audio_used * *channels ], temp, data_size );
						audio_used += data_size / ( codec_context->channels * sizeof( int16_t ) );
					}

					// Handle ignore
					while ( ignore && audio_used > *samples )
					{
						ignore --;
						audio_used -= *samples;
						memmove( audio_buffer, &audio_buffer[ *samples * *channels ], audio_used * sizeof( int16_t ) );
					}
				}

				// If we're behind, ignore this packet
				float current_pts = (float)pkt.pts / 1000000.0;
				double discrepancy = mlt_properties_get_double( properties, "discrepancy" );
				if ( current_pts != 0 && real_timecode != 0 )
				{
					if ( discrepancy != 1 )
						discrepancy = ( discrepancy + ( real_timecode / current_pts ) ) / 2;
					else
						discrepancy = real_timecode / current_pts;
					if ( discrepancy > 0.9 && discrepancy < 1.1 )
						discrepancy = 1.0;
					else
						discrepancy = floor( discrepancy + 0.5 );

					if ( discrepancy == 0 )
						discrepancy = 1.0;

					mlt_properties_set_double( properties, "discrepancy", discrepancy );
				}

				if ( !ignore && discrepancy * current_pts <= ( real_timecode - 0.02 ) )
					ignore = 1;
			}

			// We're finished with this packet regardless
			av_free_packet( &pkt );
		}

		*buffer = mlt_pool_alloc( *samples * *channels * sizeof( int16_t ) );
		mlt_properties_set_data( frame_properties, "audio", *buffer, 0, ( mlt_destructor )mlt_pool_release, NULL );

		// Now handle the audio if we have enough
		if ( audio_used >= *samples )
		{
			memcpy( *buffer, audio_buffer, *samples * *channels * sizeof( int16_t ) );
			audio_used -= *samples;
			memmove( audio_buffer, &audio_buffer[ *samples * *channels ], audio_used * *channels * sizeof( int16_t ) );
		}
		else
		{
			memset( *buffer, 0, *samples * *channels * sizeof( int16_t ) );
		}
		
		// Store the number of audio samples still available
		mlt_properties_set_int( properties, "audio_used", audio_used );

		// Release the temporary audio
		mlt_pool_release( temp );
	}
	else
	{
		// Get silence and don't touch the context
		frame->get_audio = NULL;
		mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	}

	// Regardless of speed, we expect to get the next frame (cos we ain't too bright)
	mlt_properties_set_position( properties, "audio_expected", position + 1 );

	// Unlock the mutex now
	pthread_mutex_unlock( &avformat_mutex );

	return 0;
}

/** Set up audio handling.
*/

static void producer_set_up_audio( mlt_producer this, mlt_frame frame )
{
	// Get the properties
	mlt_properties properties = mlt_producer_properties( this );

	// Fetch the audio_context
	AVFormatContext *context = mlt_properties_get_data( properties, "audio_context", NULL );

	// Get the audio_index
	int index = mlt_properties_get_int( properties, "audio_index" );

	// Lock the mutex now
	pthread_mutex_lock( &avformat_mutex );

	// Deal with audio context
	if ( context != NULL && index != -1 )
	{
		// Get the frame properties
		mlt_properties frame_properties = mlt_frame_properties( frame );

		// Get the audio stream
		AVStream *stream = context->streams[ index ];

		// Get codec context
		AVCodecContext *codec_context = &stream->codec;

		// Get the codec
		AVCodec *codec = mlt_properties_get_data( properties, "audio_codec", NULL );

		// Initialise the codec if necessary
		if ( codec == NULL )
		{
			// Find the codec
			codec = avcodec_find_decoder( codec_context->codec_id );

			// If we don't have a codec and we can't initialise it, we can't do much more...
			if ( codec != NULL && avcodec_open( codec_context, codec ) >= 0 )
			{
				// Now store the codec with its destructor
				mlt_properties_set_data( properties, "audio_codec", codec_context, 0, producer_codec_close, NULL );

			}
			else
			{
				// Remember that we can't use this later
				mlt_properties_set_int( properties, "audio_index", -1 );
			}
		}

		// No codec, no show...
		if ( codec != NULL )
		{
			frame->get_audio = producer_get_audio;
			mlt_properties_set_data( frame_properties, "avformat_producer", this, 0, NULL, NULL );
		}
	}

	// Unlock the mutex now
	pthread_mutex_unlock( &avformat_mutex );
}

/** Our get frame implementation.
*/

static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index )
{
	// Create an empty frame
	*frame = mlt_frame_init( );

	// Update timecode on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_position( this ) );

	// Set the position of this producer
	mlt_properties_set_position( mlt_frame_properties( *frame ), "avformat_position", mlt_producer_get_in( this ) + mlt_producer_position( this ) );

	// Set up the video
	producer_set_up_video( this, *frame );

	// Set up the audio
	producer_set_up_audio( this, *frame );

	// Set the aspect_ratio
	mlt_properties_set_double( mlt_frame_properties( *frame ), "aspect_ratio", mlt_properties_get_double( mlt_producer_properties( this ), "aspect_ratio" ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( this );

	return 0;
}
