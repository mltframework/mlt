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

// Forward references.
static int producer_open( mlt_producer this, char *file );
static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index );

// A static flag used to determine if avformat has been initialised
static int avformat_initialised = 0;

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

void find_default_streams( AVFormatContext *context, int *audio_index, int *video_index )
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

/** Open the file.

  	NOTE: We need to have a valid [PAL or NTSC] frame rate before we can determine the 
	number of frames in the file. However, this is at odds with the way things work - the 
	constructor needs to provide in/out points before the user of the producer is able
	to specify properties :-/. However, the PAL/NTSC distinction applies to all producers
	and while we currently accept whatever the producer provides, this will not work in
	the more general case. Plans are afoot... and this one will work without modification
	(in theory anyway ;-)).
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

	// Now attempt to open the file
	error = av_open_input_file( &context, file, NULL, 0, NULL ) < 0;

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
				mlt_properties_set_position( properties, "out", frames - 1 );
				mlt_properties_set_position( properties, "length", frames );
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
				mlt_properties_set_data( properties, "video_context", context, 0, ( mlt_destructor )av_close_input_file, NULL );

				// And open again for our audio context
				av_open_input_file( &context, file, NULL, 0, NULL );
				av_find_stream_info( context );

				// Audio context
				mlt_properties_set_data( properties, "audio_context", context, 0, ( mlt_destructor )av_close_input_file, NULL );
			}
			else if ( video_index != -1 )
			{
				// We only have a video context
				mlt_properties_set_data( properties, "video_context", context, 0, ( mlt_destructor )av_close_input_file, NULL );
			}
			else if ( audio_index != -1 )
			{
				// We only have an audio context
				mlt_properties_set_data( properties, "audio_context", context, 0, ( mlt_destructor )av_close_input_file, NULL );
			}
			else
			{
				// Something has gone wrong
				error = -1;
			}
		}
	}

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
	double current_time = 0;

	// Set the result arguments that we know here (only *buffer is now required)
	*format = mlt_image_yuv422;
	*width = codec_context->width;
	*height = codec_context->height;

	// Set this on the frame properties
	mlt_properties_set_int( frame_properties, "width", *width );
	mlt_properties_set_int( frame_properties, "height", *height );

	// Construct an AVFrame for YUV422 conversion
	if ( output == NULL )
	{
		int size = avpicture_get_size( PIX_FMT_YUV422, *width, *height );
		uint8_t *buf = malloc( size );
		output = malloc( sizeof( AVPicture ) );
		avpicture_fill( output, buf, PIX_FMT_YUV422, *width, *height );
		mlt_properties_set_data( properties, "video_output_frame", output, 0, av_free, NULL );
		mlt_properties_set_data( properties, "video_output_buffer", buf, 0, free, NULL );
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
			mlt_properties_set_double( properties, "current_time", 0 );
			mlt_properties_set_data( properties, "current_image", NULL, 0, NULL, NULL );
		}
	}
	
	// Duplicate the last image if necessary
	if ( mlt_properties_get_data( properties, "current_image", NULL ) != NULL &&
		 ( paused || mlt_properties_get_double( properties, "current_time" ) > real_timecode ) )
	{
		// Get current image and size
		int size = 0;
		uint8_t *image = mlt_properties_get_data( properties, "current_image", &size );

		// Duplicate it
		*buffer = malloc( size );
		memcpy( *buffer, image, size );

		// Set this on the frame properties
		mlt_properties_set_data( frame_properties, "image", *buffer, size, free, NULL );
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
				// Wouldn't it be great if I could use this...
				//if ( (float)pkt.pts / 1000000.0 >= real_timecode )
				ret = avcodec_decode_video( codec_context, &frame, &got_picture, pkt.data, pkt.size );

				// Handle ignore
				if ( (float)pkt.pts / 1000000.0 < real_timecode )
				{
					ignore = 0;
					got_picture = 0;
				}
				else if ( (float)pkt.pts / 1000000.0 >= real_timecode )
				{
					ignore = 0;
				}
				else if ( got_picture && ignore -- )
				{
					got_picture = 0;
				}

				current_time = ( double )pkt.pts / 1000000.0;
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
				size = *width * *height * 2;
				image = malloc( size );
				mlt_properties_set_data( properties, "current_image", image, size, free, NULL );
			}

			*buffer = malloc( size );
			img_convert( output, PIX_FMT_YUV422, (AVPicture *)&frame, codec_context->pix_fmt, *width, *height );
			memcpy( image, output->data[ 0 ], size );
			memcpy( *buffer, output->data[ 0 ], size );
			mlt_properties_set_data( frame_properties, "image", *buffer, size, free, NULL );
			mlt_properties_set_double( properties, "current_time", current_time );
		}
	}

	// Regardless of speed, we expect to get the next frame (cos we ain't too bright)
	mlt_properties_set_position( properties, "video_expected", position + 1 );

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

	if ( context != NULL && index != -1 )
	{
		// Get the frame properties
		mlt_properties frame_properties = mlt_frame_properties( frame );

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
				double aspect_ratio = 0;

				// Set aspect ratio
				if ( codec_context->sample_aspect_ratio.num == 0) 
					aspect_ratio = 0;
				else
					aspect_ratio = av_q2d( codec_context->sample_aspect_ratio ) * codec_context->width / codec_context->height;

        		if (aspect_ratio <= 0.0)
					aspect_ratio = ( double )codec_context->width / ( double )codec_context->height;

				mlt_properties_set_double( properties, "aspect_ratio", aspect_ratio );

				// Now store the codec with its destructor
				mlt_properties_set_data( properties, "video_codec", codec, 0, ( mlt_destructor )avcodec_close, NULL );

				// Set to the real timecode
				av_seek_frame( context, -1, 0 );
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
			mlt_frame_push_get_image( frame, producer_get_image );
			mlt_properties_set_data( frame_properties, "avformat_producer", this, 0, NULL, NULL );
		}
	}
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

	// Check for resample and create if necessary
	if ( resample == NULL )
	{
		// Create the resampler
		resample = audio_resample_init( *channels, codec_context->channels, *frequency, codec_context->sample_rate );

		// And store it on properties
		mlt_properties_set_data( properties, "audio_resample", resample, 0, ( mlt_destructor ) audio_resample_close, NULL );
	}

	// Check for audio buffer and create if necessary
	if ( audio_buffer == NULL )
	{
		// Allocate the audio buffer
		audio_buffer = malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE * sizeof( int16_t ) );

		// And store it on properties for reuse
		mlt_properties_set_data( properties, "audio_buffer", audio_buffer, 0, free, NULL );
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
		}
	}

	// Get the audio if required
	if ( !paused )
	{
		int ret = 0;
		int got_audio = 0;
		int16_t temp[ AVCODEC_MAX_AUDIO_FRAME_SIZE / 2 ];

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

			if ( ptr == NULL || len == 0 )
				break;

			// We only deal with video from the selected video_index
			while ( ret >= 0 && pkt.stream_index == index && len > 0 )
			{
				// Decode the audio
				ret = avcodec_decode_audio( codec_context, temp, &data_size, ptr, len );

				if ( ret < 0 )
					break;

				len -= ret;
				ptr += ret;

				if ( data_size > 0 )
				{
					int size_out = audio_resample( resample, &audio_buffer[ audio_used * *channels ], temp, data_size / ( codec_context->channels * sizeof( int16_t ) ) );

					audio_used += size_out;

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

				fprintf( stderr, "%f < %f\n", discrepancy * current_pts, (float) real_timecode );
				if ( discrepancy * current_pts < real_timecode )
					ignore = 1;
			}

			// We're finished with this packet regardless
			av_free_packet( &pkt );
		}

		// Now handle the audio if we have enough
		if ( audio_used >= *samples )
		{
			*buffer = malloc( *samples * *channels * sizeof( int16_t ) );
			memcpy( *buffer, audio_buffer, *samples * *channels * sizeof( int16_t ) );
			audio_used -= *samples;
			memmove( audio_buffer, &audio_buffer[ *samples * *channels ], audio_used * *channels * sizeof( int16_t ) );
			mlt_properties_set_data( frame_properties, "audio", *buffer, 0, free, NULL );
		}
		else
		{
			frame->get_audio = NULL;
			mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
			audio_used = 0;
		}
		
		// Store the number of audio samples still available
		mlt_properties_set_int( properties, "audio_used", audio_used );
	}
	else
	{
		// Get silence and don't touch the context
		frame->get_audio = NULL;
		mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	}

	// Regardless of speed, we expect to get the next frame (cos we ain't too bright)
	mlt_properties_set_position( properties, "audio_expected", position + 1 );

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
				mlt_properties_set_data( properties, "audio_codec", codec, 0, ( mlt_destructor )avcodec_close, NULL );
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
