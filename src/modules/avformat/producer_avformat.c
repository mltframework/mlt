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
#include <avformat.h>

// System header files
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

void avformat_lock( );
void avformat_unlock( );

// Forward references.
static int producer_open( mlt_producer this, char *file );
static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index );

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
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

			// Set the resource property (required for all producers)
			mlt_properties_set( properties, "resource", file );

			// Register our get_frame implementation
			this->get_frame = producer_get_frame;

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
   		AVCodecContext *codec_context = context->streams[ i ]->codec;

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
		avformat_lock( );

		// Close the file
		av_close_input_file( context );

		// Unlock the mutex now
		avformat_unlock( );
	}
}

/** Producer file destructor.
*/

static void producer_codec_close( void *codec )
{
	if ( codec != NULL )
	{
		// Lock the mutex now
		avformat_lock( );

		// Close the file
		avcodec_close( codec );

		// Unlock the mutex now
		avformat_unlock( );
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
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

	// We will treat everything with the producer fps
	double fps = mlt_properties_get_double( properties, "fps" );

	// Lock the mutex now
	avformat_lock( );
	
	// If "MRL", then create AVInputFormat
	AVInputFormat *format = NULL;
	AVFormatParameters *params = NULL;
	char *standard = NULL;
	char *mrl = strchr( file, ':' );

	// AV option (0 = both, 1 = video, 2 = audio)
	int av = 0;
	
	// Setting lowest log level
	av_log_set_level( -1 );

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
			params->time_base= (AVRational){1,25};
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
					params->time_base.den = atoi( value );
				else if ( !strcmp( name, "frame_rate_base" ) )
					params->time_base.num = atoi( value );
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
				else if ( !strcmp( name, "av" ) )
					av = atoi( value );
			}
			free( name );
			mrl = strchr( mrl, '&' );
		}
	}

	// Now attempt to open the file
	error = av_open_input_file( &context, file, format, 0, params ) < 0;
	
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
			int av_bypass = 0;

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

            if ( context->start_time != AV_NOPTS_VALUE )
                mlt_properties_set_double( properties, "_start_time", context->start_time );
			
			// Check if we're seekable (something funny about mpeg here :-/)
			if ( strcmp( file, "pipe:" ) && strncmp( file, "http://", 6 ) )
				mlt_properties_set_int( properties, "seekable", av_seek_frame( context, -1, mlt_properties_get_double( properties, "_start_time" ), AVSEEK_FLAG_BACKWARD ) >= 0 );
			else
				av_bypass = 1;

			// Store selected audio and video indexes on properties
			mlt_properties_set_int( properties, "audio_index", audio_index );
			mlt_properties_set_int( properties, "video_index", video_index );

			// Fetch the width, height and aspect ratio
			if ( video_index != -1 )
			{
				AVCodecContext *codec_context = context->streams[ video_index ]->codec;
				mlt_properties_set_int( properties, "width", codec_context->width );
				mlt_properties_set_int( properties, "height", codec_context->height );
				mlt_properties_set_double( properties, "aspect_ratio", av_q2d( codec_context->sample_aspect_ratio ) );
			}
			
			// We're going to cheat here - for a/v files, we will have two contexts (reasoning will be clear later)
			if ( av == 0 && !av_bypass && audio_index != -1 && video_index != -1 )
			{
				// We'll use the open one as our video_context
				mlt_properties_set_data( properties, "video_context", context, 0, producer_file_close, NULL );

				// And open again for our audio context
				av_open_input_file( &context, file, NULL, 0, NULL );
				av_find_stream_info( context );

				// Audio context
				mlt_properties_set_data( properties, "audio_context", context, 0, producer_file_close, NULL );
			}
			else if ( av != 2 && video_index != -1 )
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

			mlt_properties_set_int( properties, "av_bypass", av_bypass );
		}
	}

	// Unlock the mutex now
	avformat_unlock( );

	return error;
}

/** Convert a frame position to a time code.
*/

static double producer_time_of_frame( mlt_producer this, mlt_position position )
{
	// Get the properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

	// Obtain the fps
	double fps = mlt_properties_get_double( properties, "fps" );

	// Do the calc
	return ( double )position / fps;
}

static inline void convert_image( AVFrame *frame, uint8_t *buffer, int pix_fmt, mlt_image_format format, int width, int height )
{
	if ( format == mlt_image_yuv420p )
	{
		AVPicture pict;
		pict.data[0] = buffer;
		pict.data[1] = buffer + width * height;
		pict.data[2] = buffer + ( 3 * width * height ) / 2;
		pict.linesize[0] = width;
		pict.linesize[1] = width >> 1;
		pict.linesize[2] = width >> 1;
		img_convert( &pict, PIX_FMT_YUV420P, (AVPicture *)frame, pix_fmt, width, height );
	}
	else if ( format == mlt_image_rgb24 )
	{
		AVPicture output;
		avpicture_fill( &output, buffer, PIX_FMT_RGB24, width, height );
		img_convert( &output, PIX_FMT_RGB24, (AVPicture *)frame, pix_fmt, width, height );
	}
	else
	{
		AVPicture output;
		avpicture_fill( &output, buffer, PIX_FMT_YUV422, width, height );
		img_convert( &output, PIX_FMT_YUV422, (AVPicture *)frame, pix_fmt, width, height );
	}
}

/** Get an image from a frame.
*/

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the properties from the frame
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the frame number of this frame
	mlt_position position = mlt_properties_get_position( frame_properties, "avformat_position" );

	// Get the producer 
	mlt_producer this = mlt_properties_get_data( frame_properties, "avformat_producer", NULL );

	// Get the producer properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

	// Fetch the video_context
	AVFormatContext *context = mlt_properties_get_data( properties, "video_context", NULL );

	// Get the video_index
	int index = mlt_properties_get_int( properties, "video_index" );

	// Obtain the expected frame numer
	mlt_position expected = mlt_properties_get_position( properties, "_video_expected" );

	// Calculate the real time code
	double real_timecode = producer_time_of_frame( this, position );

	// Get the video stream
	AVStream *stream = context->streams[ index ];

	// Get codec context
	AVCodecContext *codec_context = stream->codec;

	// Packet
	AVPacket pkt;

	// Get the conversion frame
	AVFrame *av_frame = mlt_properties_get_data( properties, "av_frame", NULL );

	// Special case pause handling flag
	int paused = 0;

	// Special case ffwd handling
	int ignore = 0;

	// Current time calcs
	double current_time = mlt_properties_get_double( properties, "_current_time" );

	// We may want to use the source fps if available
	double source_fps = mlt_properties_get_double( properties, "source_fps" );

	// Get the seekable status
	int seekable = mlt_properties_get_int( properties, "seekable" );

	// Generate the size in bytes
	int size = 0; 

	// Hopefully provide better support for streams...
	int av_bypass = mlt_properties_get_int( properties, "av_bypass" );

	// Set the result arguments that we know here (only *buffer is now required)
	*width = codec_context->width;
	*height = codec_context->height;

	switch ( *format )
	{
		case mlt_image_yuv420p:
			size = *width * 3 * ( *height + 1 ) / 2;
			break;
		case mlt_image_rgb24:
			size = *width * ( *height + 1 ) * 3;
			break;
		default:
			*format = mlt_image_yuv422;
			size = *width * ( *height + 1 ) * 2;
			break;
	}

	// Set this on the frame properties
	mlt_properties_set_int( frame_properties, "width", *width );
	mlt_properties_set_int( frame_properties, "height", *height );

	// Construct the output image
	*buffer = mlt_pool_alloc( size );

	// Seek if necessary
	if ( position != expected )
	{
		if ( av_frame != NULL && position + 1 == expected )
		{
			// We're paused - use last image
			paused = 1;
		}
		else if ( !seekable && position > expected && ( position - expected ) < 250 )
		{
			// Fast forward - seeking is inefficient for small distances - just ignore following frames
			ignore = position - expected;
		}
		else if ( seekable && ( position < expected || position - expected >= 12 ) )
		{
			// Set to the real timecode
			av_seek_frame( context, -1, mlt_properties_get_double( properties, "_start_time" ) + real_timecode * 1000000.0, AVSEEK_FLAG_BACKWARD );
	
			// Remove the cached info relating to the previous position
			mlt_properties_set_double( properties, "_current_time", real_timecode );

			mlt_properties_set_data( properties, "av_frame", NULL, 0, NULL, NULL );
			av_frame = NULL;
		}
	}
	
	// Duplicate the last image if necessary (see comment on rawvideo below)
	if ( av_frame != NULL && ( paused || mlt_properties_get_double( properties, "_current_time" ) >= real_timecode ) && av_bypass == 0 )
	{
		// Duplicate it
		convert_image( av_frame, *buffer, codec_context->pix_fmt, *format, *width, *height );

		// Set this on the frame properties
		mlt_properties_set_data( frame_properties, "image", *buffer, size, ( mlt_destructor )mlt_pool_release, NULL );
	}
	else
	{
		int ret = 0;
		int got_picture = 0;
		int must_decode = 1;

		// Temporary hack to improve intra frame only
		if ( !strcmp( codec_context->codec->name, "mjpeg" ) || !strcmp( codec_context->codec->name, "rawvideo" ) )
			must_decode = 0;

		av_init_packet( &pkt );

		// Construct an AVFrame for YUV422 conversion
		if ( av_frame == NULL )
		{
			av_frame = avcodec_alloc_frame( );
			mlt_properties_set_data( properties, "av_frame", av_frame, 0, av_free, NULL );
		}

		while( ret >= 0 && !got_picture )
		{
			// Read a packet
			ret = av_read_frame( context, &pkt );

			// We only deal with video from the selected video_index
			if ( ret >= 0 && pkt.stream_index == index && pkt.size > 0 )
			{
				// Determine time code of the packet
				if ( pkt.dts != AV_NOPTS_VALUE )
					current_time = av_q2d( stream->time_base ) * pkt.dts;
				else
					current_time = real_timecode;

				// Decode the image
				if ( must_decode || current_time >= real_timecode )
					ret = avcodec_decode_video( codec_context, av_frame, &got_picture, pkt.data, pkt.size );

				if ( got_picture )
				{
					// Handle ignore
					if ( ( int )( current_time * 100 ) < ( int )( real_timecode * 100 ) - 7 )
					{
						ignore = 0;
						got_picture = 0;
					}
					else if ( current_time >= real_timecode )
					{
						ignore = 0;
					}
					else if ( ignore -- )
					{
						got_picture = 0;
					}
				}
			}

			// Now handle the picture if we have one
			if ( got_picture )
			{
				mlt_properties_set_int( frame_properties, "progressive", !av_frame->interlaced_frame );
				mlt_properties_set_int( frame_properties, "top_field_first", av_frame->top_field_first );

				convert_image( av_frame, *buffer, codec_context->pix_fmt, *format, *width, *height );

				mlt_properties_set_data( frame_properties, "image", *buffer, size, (mlt_destructor)mlt_pool_release, NULL );

				if ( current_time == 0 && source_fps != 0 )
				{
					double fps = mlt_properties_get_double( properties, "fps" );
					current_time = ceil( source_fps * ( double )position / fps ) * ( 1 / source_fps );
					mlt_properties_set_double( properties, "_current_time", current_time );
				}
				else
				{
					mlt_properties_set_double( properties, "_current_time", current_time );
				}
			}

			// We're finished with this packet regardless
			av_free_packet( &pkt );
		}
	}

	// Very untidy - for rawvideo, the packet contains the frame, hence the free packet
	// above will break the pause behaviour - so we wipe the frame now
	if ( !strcmp( codec_context->codec->name, "rawvideo" ) )
		mlt_properties_set_data( properties, "av_frame", NULL, 0, NULL, NULL );

	// Set the field order property for this frame
	mlt_properties_set_int( frame_properties, "top_field_first", mlt_properties_get_int( properties, "top_field_first" ) );

	// Regardless of speed, we expect to get the next frame (cos we ain't too bright)
	mlt_properties_set_position( properties, "_video_expected", position + 1 );

	return 0;
}

/** Set up video handling.
*/

static void producer_set_up_video( mlt_producer this, mlt_frame frame )
{
	// Get the properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

	// Fetch the video_context
	AVFormatContext *context = mlt_properties_get_data( properties, "video_context", NULL );

	// Get the video_index
	int index = mlt_properties_get_int( properties, "video_index" );

	// Get the frame properties
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	if ( context != NULL && index != -1 )
	{
		// Get the video stream
		AVStream *stream = context->streams[ index ];

		// Get codec context
		AVCodecContext *codec_context = stream->codec;

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
			double source_fps = 0;
			int norm_aspect_ratio = mlt_properties_get_int( properties, "norm_aspect_ratio" );
			double force_aspect_ratio = mlt_properties_get_double( properties, "force_aspect_ratio" );
			double aspect_ratio;

			// XXX: We won't know the real aspect ratio until an image is decoded
			// but we do need it now (to satisfy filter_resize) - take a guess based
			// on pal/ntsc
			if ( force_aspect_ratio > 0.0 )
			{
				aspect_ratio = force_aspect_ratio;
			}
			else if ( !norm_aspect_ratio && codec_context->sample_aspect_ratio.num > 0 )
			{
				aspect_ratio = av_q2d( codec_context->sample_aspect_ratio );
			}
			else
			{
				int is_pal = mlt_properties_get_double( properties, "fps" ) == 25.0;
				aspect_ratio = is_pal ? 59.0/54.0 : 10.0/11.0;
			}

			// Determine the fps
			source_fps = ( double )codec_context->time_base.den / ( codec_context->time_base.num == 0 ? 1 : codec_context->time_base.num );

			// We'll use fps if it's available
			if ( source_fps > 0 && source_fps < 30 )
				mlt_properties_set_double( properties, "source_fps", source_fps );
			mlt_properties_set_double( properties, "aspect_ratio", aspect_ratio );
			
			// Set the width and height
			mlt_properties_set_int( frame_properties, "width", codec_context->width );
			mlt_properties_set_int( frame_properties, "height", codec_context->height );
			mlt_properties_set_double( frame_properties, "aspect_ratio", aspect_ratio );

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
}

/** Get the audio from a frame.
*/

static int producer_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the properties from the frame
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the frame number of this frame
	mlt_position position = mlt_properties_get_position( frame_properties, "avformat_position" );

	// Get the producer 
	mlt_producer this = mlt_properties_get_data( frame_properties, "avformat_producer", NULL );

	// Get the producer properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

	// Fetch the audio_context
	AVFormatContext *context = mlt_properties_get_data( properties, "audio_context", NULL );

	// Get the audio_index
	int index = mlt_properties_get_int( properties, "audio_index" );

	// Get the seekable status
	int seekable = mlt_properties_get_int( properties, "seekable" );

	// Obtain the expected frame numer
	mlt_position expected = mlt_properties_get_position( properties, "_audio_expected" );

	// Obtain the resample context if it exists (not always needed)
	ReSampleContext *resample = mlt_properties_get_data( properties, "audio_resample", NULL );

	// Obtain the audio buffer
	int16_t *audio_buffer = mlt_properties_get_data( properties, "audio_buffer", NULL );

	// Get amount of audio used
	int audio_used =  mlt_properties_get_int( properties, "_audio_used" );

	// Calculate the real time code
	double real_timecode = producer_time_of_frame( this, position );

	// Get the audio stream
	AVStream *stream = context->streams[ index ];

	// Get codec context
	AVCodecContext *codec_context = stream->codec;

	// Packet
	AVPacket pkt;

	// Number of frames to ignore (for ffwd)
	int ignore = 0;

	// Flag for paused (silence) 
	int paused = 0;

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
		else if ( !seekable && position > expected && ( position - expected ) < 250 )
		{
			// Fast forward - seeking is inefficient for small distances - just ignore following frames
			ignore = position - expected;
		}
		else if ( position < expected || position - expected >= 12 )
		{
			// Set to the real timecode
			if ( av_seek_frame( context, -1, mlt_properties_get_double( properties, "_start_time" ) + real_timecode * 1000000.0, AVSEEK_FLAG_BACKWARD ) != 0 )
				paused = 1;

			// Clear the usage in the audio buffer
			audio_used = 0;
		}
	}

	// Get the audio if required
	if ( !paused )
	{
		int ret = 0;
		int got_audio = 0;
		int16_t *temp = mlt_pool_alloc( sizeof( int16_t ) * AVCODEC_MAX_AUDIO_FRAME_SIZE );

		av_init_packet( &pkt );

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
				float current_pts = av_q2d( stream->time_base ) * pkt.pts;
				if ( seekable && ( !ignore && current_pts <= ( real_timecode - 0.02 ) ) )
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
		mlt_properties_set_int( properties, "_audio_used", audio_used );

		// Release the temporary audio
		mlt_pool_release( temp );
	}
	else
	{
		// Get silence and don't touch the context
		mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	}

	// Regardless of speed (other than paused), we expect to get the next frame
	if ( !paused )
		mlt_properties_set_position( properties, "_audio_expected", position + 1 );

	return 0;
}

/** Set up audio handling.
*/

static void producer_set_up_audio( mlt_producer this, mlt_frame frame )
{
	// Get the properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

	// Fetch the audio_context
	AVFormatContext *context = mlt_properties_get_data( properties, "audio_context", NULL );

	// Get the audio_index
	int index = mlt_properties_get_int( properties, "audio_index" );

	// Deal with audio context
	if ( context != NULL && index != -1 )
	{
		// Get the frame properties
		mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

		// Get the audio stream
		AVStream *stream = context->streams[ index ];

		// Get codec context
		AVCodecContext *codec_context = stream->codec;

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
			mlt_frame_push_audio( frame, producer_get_audio );
			mlt_properties_set_data( frame_properties, "avformat_producer", this, 0, NULL, NULL );
			mlt_properties_set_int( frame_properties, "frequency", codec_context->sample_rate );
			mlt_properties_set_int( frame_properties, "channels", codec_context->channels );
		}
	}
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
	mlt_properties_set_position( MLT_FRAME_PROPERTIES( *frame ), "avformat_position", mlt_producer_frame( this ) );

	// Set up the video
	producer_set_up_video( this, *frame );

	// Set up the audio
	producer_set_up_audio( this, *frame );

	// Set the aspect_ratio
	mlt_properties_set_double( MLT_FRAME_PROPERTIES( *frame ), "aspect_ratio", mlt_properties_get_double( MLT_PRODUCER_PROPERTIES( this ), "aspect_ratio" ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( this );

	return 0;
}
