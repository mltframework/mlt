/*
 * producer_avformat.c -- avformat producer
 * Copyright (C) 2003-2009 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 * Author: Dan Dennedy <dan@dennedy.org>
 * Much code borrowed from ffmpeg.c: Copyright (c) 2000-2003 Fabrice Bellard
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

// MLT Header files
#include <framework/mlt_producer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_profile.h>
#include <framework/mlt_log.h>
#include <framework/mlt_deque.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_cache.h>

// ffmpeg Header files
#include <avformat.h>
#include <opt.h>
#ifdef SWSCALE
#  include <swscale.h>
#endif
#if (LIBAVCODEC_VERSION_INT >= ((51<<16)+(71<<8)+0))
#  include "audioconvert.h"
#endif
#ifdef VDPAU
#include <vdpau.h>
#endif

// System header files
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>

#if LIBAVUTIL_VERSION_INT < (50<<16)
#define PIX_FMT_RGB32 PIX_FMT_RGBA32
#define PIX_FMT_YUYV422 PIX_FMT_YUV422
#endif

#define POSITION_INITIAL (-2)
#define POSITION_INVALID (-1)

#define MAX_AUDIO_STREAMS (10)
#define MAX_VDPAU_SURFACES (10)

void avformat_lock( );
void avformat_unlock( );

struct producer_avformat_s
{
	mlt_producer parent;
	AVFormatContext *dummy_context;
	AVFormatContext *audio_format;
	AVFormatContext *video_format;
	AVCodecContext *audio_codec[ MAX_AUDIO_STREAMS ];
	AVCodecContext *video_codec;
	AVFrame *av_frame;
	ReSampleContext *audio_resample[ MAX_AUDIO_STREAMS ];
	mlt_position audio_expected;
	mlt_position video_expected;
	int audio_index;
	int video_index;
	double start_time;
	int first_pts;
	int last_position;
	int seekable;
	int current_position;
	int got_picture;
	int top_field_first;
	int16_t *audio_buffer[ MAX_AUDIO_STREAMS ];
	size_t audio_buffer_size[ MAX_AUDIO_STREAMS ];
	int16_t *decode_buffer[ MAX_AUDIO_STREAMS ];
	int audio_used[ MAX_AUDIO_STREAMS ];
	int audio_streams;
	int audio_max_stream;
	int total_channels;
	int max_channel;
	int max_frequency;
	unsigned int invalid_pts_counter;
	double resample_factor;
	mlt_cache image_cache;
#ifdef VDPAU
	struct
	{
		// from FFmpeg
		struct vdpau_render_state render_states[MAX_VDPAU_SURFACES];
		
		// internal
		mlt_deque deque;
		int b_age;
		int ip_age[2];
		int is_decoded;
		uint8_t *buffer;
	} *vdpau;
#endif
};
typedef struct producer_avformat_s *producer_avformat;

// Forward references.
static int producer_open( producer_avformat this, mlt_profile profile, char *file );
static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index );
static void producer_avformat_close( producer_avformat );
static void producer_close( mlt_producer parent );

#ifdef VDPAU
#include "vdpau.c"
#endif

/** Constructor for libavformat.
*/

mlt_producer producer_avformat_init( mlt_profile profile, const char *service, char *file )
{
	int skip = 0;

	// Report information about available demuxers and codecs as YAML Tiny
	if ( file && strstr( file, "f-list" ) )
	{
		fprintf( stderr, "---\nformats:\n" );
		AVInputFormat *format = NULL;
		while ( ( format = av_iformat_next( format ) ) )
			fprintf( stderr, "  - %s\n", format->name );
		fprintf( stderr, "...\n" );
		skip = 1;
	}
	if ( file && strstr( file, "acodec-list" ) )
	{
		fprintf( stderr, "---\naudio_codecs:\n" );
		AVCodec *codec = NULL;
		while ( ( codec = av_codec_next( codec ) ) )
			if ( codec->decode && codec->type == CODEC_TYPE_AUDIO )
				fprintf( stderr, "  - %s\n", codec->name );
		fprintf( stderr, "...\n" );
		skip = 1;
	}
	if ( file && strstr( file, "vcodec-list" ) )
	{
		fprintf( stderr, "---\nvideo_codecs:\n" );
		AVCodec *codec = NULL;
		while ( ( codec = av_codec_next( codec ) ) )
			if ( codec->decode && codec->type == CODEC_TYPE_VIDEO )
				fprintf( stderr, "  - %s\n", codec->name );
		fprintf( stderr, "...\n" );
		skip = 1;
	}

	// Check that we have a non-NULL argument
	if ( !skip && file )
	{
		// Construct the producer
		mlt_producer producer = calloc( 1, sizeof( struct mlt_producer_s ) );
		producer_avformat this = calloc( 1, sizeof( struct producer_avformat_s ) );

		// Initialise it
		if ( mlt_producer_init( producer, this ) == 0 )
		{
			this->parent = producer;

			// Get the properties
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

			// Set the resource property (required for all producers)
			mlt_properties_set( properties, "resource", file );

			// Register transport implementation with the producer
			producer->close = (mlt_destructor) producer_close;

			// Register our get_frame implementation
			producer->get_frame = producer_get_frame;
			
			if ( strcmp( service, "avformat-novalidate" ) )
			{
				// Open the file
				if ( producer_open( this, profile, file ) != 0 )
				{
					// Clean up
					mlt_producer_close( producer );
					producer = NULL;
				}
				else
				{
					// Close the file to release resources for large playlists - reopen later as needed
					avformat_lock();
					if ( this->dummy_context )
						av_close_input_file( this->dummy_context );
					this->dummy_context = NULL;
					if ( this->audio_format )
						av_close_input_file( this->audio_format );
					this->audio_format = NULL;
					if ( this->video_format )
						av_close_input_file( this->video_format );
					this->video_format = NULL;
					avformat_unlock();
	
					// Default the user-selectable indices from the auto-detected indices
					mlt_properties_set_int( properties, "audio_index",  this->audio_index );
					mlt_properties_set_int( properties, "video_index",  this->video_index );
					
#ifdef VDPAU
					mlt_service_cache_set_size( MLT_PRODUCER_SERVICE(producer), "producer_avformat", 5 );
#endif
					mlt_service_cache_put( MLT_PRODUCER_SERVICE(producer), "producer_avformat", this, 0, (mlt_destructor) producer_avformat_close );
				}
			}
			else
			{
#ifdef VDPAU
				mlt_service_cache_set_size( MLT_PRODUCER_SERVICE(producer), "producer_avformat", 5 );
#endif
				mlt_service_cache_put( MLT_PRODUCER_SERVICE(producer), "producer_avformat", this, 0, (mlt_destructor) producer_avformat_close );
			}
			return producer;
		}
	}
	return NULL;
}

/** Find the default streams.
*/

static mlt_properties find_default_streams( mlt_properties meta_media, AVFormatContext *context, int *audio_index, int *video_index )
{
	int i;
	char key[200];

	mlt_properties_set_int( meta_media, "meta.media.nb_streams", context->nb_streams );

	// Allow for multiple audio and video streams in the file and select first of each (if available)
	for( i = 0; i < context->nb_streams; i++ )
	{
		// Get the codec context
		AVStream *stream = context->streams[ i ];
		if ( ! stream ) continue;
		AVCodecContext *codec_context = stream->codec;
		if ( ! codec_context ) continue;
		AVCodec *codec = avcodec_find_decoder( codec_context->codec_id );
		if ( ! codec ) continue;

		snprintf( key, sizeof(key), "meta.media.%d.stream.type", i );

		// Determine the type and obtain the first index of each type
		switch( codec_context->codec_type )
		{
			case CODEC_TYPE_VIDEO:
				if ( *video_index < 0 )
					*video_index = i;
				mlt_properties_set( meta_media, key, "video" );
				snprintf( key, sizeof(key), "meta.media.%d.stream.frame_rate", i );
				mlt_properties_set_double( meta_media, key, av_q2d( context->streams[ i ]->r_frame_rate ) );
#if LIBAVFORMAT_VERSION_INT >= ((52<<16)+(21<<8)+0)
				snprintf( key, sizeof(key), "meta.media.%d.stream.sample_aspect_ratio", i );
				mlt_properties_set_double( meta_media, key, av_q2d( context->streams[ i ]->sample_aspect_ratio ) );
#endif
				snprintf( key, sizeof(key), "meta.media.%d.codec.frame_rate", i );
				mlt_properties_set_double( meta_media, key, (double) codec_context->time_base.den /
										   ( codec_context->time_base.num == 0 ? 1 : codec_context->time_base.num ) );
				snprintf( key, sizeof(key), "meta.media.%d.codec.pix_fmt", i );
				mlt_properties_set( meta_media, key, avcodec_get_pix_fmt_name( codec_context->pix_fmt ) );
				snprintf( key, sizeof(key), "meta.media.%d.codec.sample_aspect_ratio", i );
				mlt_properties_set_double( meta_media, key, av_q2d( codec_context->sample_aspect_ratio ) );
				break;
			case CODEC_TYPE_AUDIO:
				if ( *audio_index < 0 )
					*audio_index = i;
				mlt_properties_set( meta_media, key, "audio" );
#if (LIBAVCODEC_VERSION_INT >= ((51<<16)+(71<<8)+0))
				snprintf( key, sizeof(key), "meta.media.%d.codec.sample_fmt", i );
				mlt_properties_set( meta_media, key, avcodec_get_sample_fmt_name( codec_context->sample_fmt ) );
#endif
				snprintf( key, sizeof(key), "meta.media.%d.codec.sample_rate", i );
				mlt_properties_set_int( meta_media, key, codec_context->sample_rate );
				snprintf( key, sizeof(key), "meta.media.%d.codec.channels", i );
				mlt_properties_set_int( meta_media, key, codec_context->channels );
				break;
			default:
				break;
		}
// 		snprintf( key, sizeof(key), "meta.media.%d.stream.time_base", i );
// 		mlt_properties_set_double( meta_media, key, av_q2d( context->streams[ i ]->time_base ) );
		snprintf( key, sizeof(key), "meta.media.%d.codec.name", i );
		mlt_properties_set( meta_media, key, codec->name );
#if (LIBAVCODEC_VERSION_INT >= ((51<<16)+(55<<8)+0))
		snprintf( key, sizeof(key), "meta.media.%d.codec.long_name", i );
		mlt_properties_set( meta_media, key, codec->long_name );
#endif
		snprintf( key, sizeof(key), "meta.media.%d.codec.bit_rate", i );
		mlt_properties_set_int( meta_media, key, codec_context->bit_rate );
// 		snprintf( key, sizeof(key), "meta.media.%d.codec.time_base", i );
// 		mlt_properties_set_double( meta_media, key, av_q2d( codec_context->time_base ) );
//		snprintf( key, sizeof(key), "meta.media.%d.codec.profile", i );
//		mlt_properties_set_int( meta_media, key, codec_context->profile );
//		snprintf( key, sizeof(key), "meta.media.%d.codec.level", i );
//		mlt_properties_set_int( meta_media, key, codec_context->level );
	}

	return meta_media;
}

static inline int dv_is_pal( AVPacket *pkt )
{
	return pkt->data[3] & 0x80;
}

static int dv_is_wide( AVPacket *pkt )
{
	int i = 80 /* block size */ *3 /* VAUX starts at block 3 */ +3 /* skip block header */;

	for ( ; i < pkt->size; i += 5 /* packet size */ )
	{
		if ( pkt->data[ i ] == 0x61 )
		{
			uint8_t x = pkt->data[ i + 2 ] & 0x7;
			return ( x == 2 ) || ( x == 7 );
		}
	}
	return 0;
}

static double get_aspect_ratio( AVStream *stream, AVCodecContext *codec_context, AVPacket *pkt )
{
	double aspect_ratio = 1.0;

	if ( codec_context->codec_id == CODEC_ID_DVVIDEO )
	{
		if ( pkt )
		{
			if ( dv_is_pal( pkt ) )
			{
				aspect_ratio = dv_is_wide( pkt )
					? 64.0/45.0 // 16:9 PAL
					: 16.0/15.0; // 4:3 PAL
			}
			else
			{
				aspect_ratio = dv_is_wide( pkt )
					? 32.0/27.0 // 16:9 NTSC
					: 8.0/9.0; // 4:3 NTSC
			}
		}
		else
		{
			AVRational ar =
#if LIBAVFORMAT_VERSION_INT >= ((52<<16)+(21<<8)+0)
				stream->sample_aspect_ratio;
#else
				codec_context->sample_aspect_ratio;
#endif
			// Override FFmpeg's notion of DV aspect ratios, which are
			// based upon a width of 704. Since we do not have a normaliser
			// that crops (nor is cropping 720 wide ITU-R 601 video always desirable)
			// we just coerce the values to facilitate a passive behaviour through
			// the rescale normaliser when using equivalent producers and consumers.
			// = display_aspect / (width * height)
			if ( ar.num == 10 && ar.den == 11 )
				aspect_ratio = 8.0/9.0; // 4:3 NTSC
			else if ( ar.num == 59 && ar.den == 54 )
				aspect_ratio = 16.0/15.0; // 4:3 PAL
			else if ( ar.num == 40 && ar.den == 33 )
				aspect_ratio = 32.0/27.0; // 16:9 NTSC
			else if ( ar.num == 118 && ar.den == 81 )
				aspect_ratio = 64.0/45.0; // 16:9 PAL
		}
	}
	else
	{
		AVRational codec_sar = codec_context->sample_aspect_ratio;
		AVRational stream_sar =
#if LIBAVFORMAT_VERSION_INT >= ((52<<16)+(21<<8)+0)
			stream->sample_aspect_ratio;
#else
			{ 0, 1 };
#endif
		if ( codec_sar.num > 0 )
			aspect_ratio = av_q2d( codec_sar );
		else if ( stream_sar.num > 0 )
			aspect_ratio = av_q2d( stream_sar );
	}
	return aspect_ratio;
}

/** Open the file.
*/

static int producer_open( producer_avformat this, mlt_profile profile, char *file )
{
	// Return an error code (0 == no error)
	int error = 0;

	// Context for avformat
	AVFormatContext *context = NULL;

	// Get the properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this->parent );

	// We will treat everything with the producer fps
	double fps = mlt_profile_fps( profile );

	// Lock the mutex now
	avformat_lock( );

	// If "MRL", then create AVInputFormat
	AVInputFormat *format = NULL;
	AVFormatParameters *params = NULL;
	char *standard = NULL;
	char *mrl = strchr( file, ':' );

	// AV option (0 = both, 1 = video, 2 = audio)
	int av = 0;

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
			// params->device = file;
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
	if ( !error )
	{
		// Get the stream info
		error = av_find_stream_info( context ) < 0;

		// Continue if no error
		if ( !error )
		{
			// We will default to the first audio and video streams found
			int audio_index = -1;
			int video_index = -1;

			// Now set properties where we can (use default unknowns if required)
			if ( context->duration != AV_NOPTS_VALUE )
			{
				// This isn't going to be accurate for all formats
				mlt_position frames = ( mlt_position )( ( ( double )context->duration / ( double )AV_TIME_BASE ) * fps + 0.5 );
				mlt_properties_set_position( properties, "out", frames - 1 );
				mlt_properties_set_position( properties, "length", frames );
			}

			// Find default audio and video streams
			find_default_streams( properties, context, &audio_index, &video_index );

			if ( context->start_time != AV_NOPTS_VALUE )
				this->start_time = context->start_time;

			// Check if we're seekable (something funny about mpeg here :-/)
			if ( strncmp( file, "pipe:", 5 ) &&
				 strncmp( file, "/dev/", 5 ) &&
				 strncmp( file, "http:", 5 ) &&
				 strncmp( file, "udp:", 4 )  &&
				 strncmp( file, "tcp:", 4 )  &&
				 strncmp( file, "rtsp:", 5 ) &&
				 strncmp( file, "rtp:", 4 ) )
			{
				this->seekable = av_seek_frame( context, -1, this->start_time, AVSEEK_FLAG_BACKWARD ) >= 0;
				mlt_properties_set_int( properties, "seekable", this->seekable );
				this->dummy_context = context;
				av_open_input_file( &context, file, NULL, 0, NULL );
				av_find_stream_info( context );
			}

			// Store selected audio and video indexes on properties
			this->audio_index = audio_index;
			this->video_index = video_index;
			this->first_pts = -1;
			this->last_position = POSITION_INITIAL;

			// Fetch the width, height and aspect ratio
			if ( video_index != -1 )
			{
				AVCodecContext *codec_context = context->streams[ video_index ]->codec;
				mlt_properties_set_int( properties, "width", codec_context->width );
				mlt_properties_set_int( properties, "height", codec_context->height );

				if ( codec_context->codec_id == CODEC_ID_DVVIDEO )
				{
					// Fetch the first frame of DV so we can read it directly
					AVPacket pkt;
					int ret = 0;
					while ( ret >= 0 )
					{
						ret = av_read_frame( context, &pkt );
						if ( ret >= 0 && pkt.stream_index == video_index && pkt.size > 0 )
						{
							mlt_properties_set_double( properties, "aspect_ratio",
								get_aspect_ratio( context->streams[ video_index ], codec_context, &pkt ) );
							break;
						}
					}
				}
				else
				{
					mlt_properties_set_double( properties, "aspect_ratio",
						get_aspect_ratio( context->streams[ video_index ], codec_context, NULL ) );
				}
			}

			// Read Metadata
			if ( context->title )
				mlt_properties_set(properties, "meta.attr.title.markup", context->title );
			if ( context->author )
				mlt_properties_set(properties, "meta.attr.author.markup", context->author );
			if ( context->copyright )
				mlt_properties_set(properties, "meta.attr.copyright.markup", context->copyright );
			if ( context->comment )
				mlt_properties_set(properties, "meta.attr.comment.markup", context->comment );
			if ( context->album )
				mlt_properties_set(properties, "meta.attr.album.markup", context->album );
			if ( context->year )
				mlt_properties_set_int(properties, "meta.attr.year.markup", context->year );
			if ( context->track )
				mlt_properties_set_int(properties, "meta.attr.track.markup", context->track );

			// We're going to cheat here - for a/v files, we will have two contexts (reasoning will be clear later)
			if ( av == 0 && audio_index != -1 && video_index != -1 )
			{
				// We'll use the open one as our video_format
				this->video_format = context;

				// And open again for our audio context
				av_open_input_file( &context, file, NULL, 0, NULL );
				av_find_stream_info( context );

				// Audio context
				this->audio_format = context;
			}
			else if ( av != 2 && video_index != -1 )
			{
				// We only have a video context
				this->video_format = context;
			}
			else if ( audio_index != -1 )
			{
				// We only have an audio context
				this->audio_format = context;
			}
			else
			{
				// Something has gone wrong
				error = -1;
			}
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
	return ( double )position / mlt_producer_get_fps( this );
}

		// Collect information about all audio streams

static void get_audio_streams_info( producer_avformat this )
{
	// Fetch the audio format context
	AVFormatContext *context = this->audio_format;
	int i;

	for ( i = 0;
		  i < context->nb_streams;
		  i++ )
	{
		if ( context->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO )
		{
			AVCodecContext *codec_context = context->streams[i]->codec;
			AVCodec *codec = avcodec_find_decoder( codec_context->codec_id );

			// If we don't have a codec and we can't initialise it, we can't do much more...
			avformat_lock( );
			if ( codec && avcodec_open( codec_context, codec ) >= 0 )
			{
				this->audio_streams++;
				this->audio_max_stream = i;
				this->total_channels += codec_context->channels;
				if ( codec_context->channels > this->max_channel )
					this->max_channel = codec_context->channels;
				if ( codec_context->sample_rate > this->max_frequency )
					this->max_frequency = codec_context->sample_rate;
				avcodec_close( codec_context );
			}
			avformat_unlock( );
		}
	}
	mlt_log_verbose( NULL, "[producer avformat] audio: total_streams %d max_stream %d total_channels %d max_channels %d\n",
		this->audio_streams, this->audio_max_stream, this->total_channels, this->max_channel );
	
	// Other audio-specific initializations
	this->resample_factor = 1.0;
}

static inline void convert_image( AVFrame *frame, uint8_t *buffer, int pix_fmt, mlt_image_format *format, int width, int height )
{
#ifdef SWSCALE
	if ( pix_fmt == PIX_FMT_RGB32 )
	{
		*format = mlt_image_rgb24a;
		struct SwsContext *context = sws_getContext( width, height, pix_fmt,
			width, height, PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
		AVPicture output;
		avpicture_fill( &output, buffer, PIX_FMT_RGBA, width, height );
		sws_scale( context, frame->data, frame->linesize, 0, height,
			output.data, output.linesize);
		sws_freeContext( context );
	}
	else if ( *format == mlt_image_yuv420p )
	{
		struct SwsContext *context = sws_getContext( width, height, pix_fmt,
			width, height, PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
		AVPicture output;
		output.data[0] = buffer;
		output.data[1] = buffer + width * height;
		output.data[2] = buffer + ( 5 * width * height ) / 4;
		output.linesize[0] = width;
		output.linesize[1] = width >> 1;
		output.linesize[2] = width >> 1;
		sws_scale( context, frame->data, frame->linesize, 0, height,
			output.data, output.linesize);
		sws_freeContext( context );
	}
	else if ( *format == mlt_image_rgb24 )
	{
		struct SwsContext *context = sws_getContext( width, height, pix_fmt,
			width, height, PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
		AVPicture output;
		avpicture_fill( &output, buffer, PIX_FMT_RGB24, width, height );
		sws_scale( context, frame->data, frame->linesize, 0, height,
			output.data, output.linesize);
		sws_freeContext( context );
	}
	else if ( *format == mlt_image_rgb24a || *format == mlt_image_opengl )
	{
		struct SwsContext *context = sws_getContext( width, height, pix_fmt,
			width, height, PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
		AVPicture output;
		avpicture_fill( &output, buffer, PIX_FMT_RGBA, width, height );
		sws_scale( context, frame->data, frame->linesize, 0, height,
			output.data, output.linesize);
		sws_freeContext( context );
	}
	else
	{
		struct SwsContext *context = sws_getContext( width, height, pix_fmt,
			width, height, PIX_FMT_YUYV422, SWS_BILINEAR, NULL, NULL, NULL);
		AVPicture output;
		avpicture_fill( &output, buffer, PIX_FMT_YUYV422, width, height );
		sws_scale( context, frame->data, frame->linesize, 0, height,
			output.data, output.linesize);
		sws_freeContext( context );
	}
#else
	if ( *format == mlt_image_yuv420p )
	{
		AVPicture pict;
		pict.data[0] = buffer;
		pict.data[1] = buffer + width * height;
		pict.data[2] = buffer + ( 5 * width * height ) / 4;
		pict.linesize[0] = width;
		pict.linesize[1] = width >> 1;
		pict.linesize[2] = width >> 1;
		img_convert( &pict, PIX_FMT_YUV420P, (AVPicture *)frame, pix_fmt, width, height );
	}
	else if ( *format == mlt_image_rgb24 )
	{
		AVPicture output;
		avpicture_fill( &output, buffer, PIX_FMT_RGB24, width, height );
		img_convert( &output, PIX_FMT_RGB24, (AVPicture *)frame, pix_fmt, width, height );
	}
	else if ( format == mlt_image_rgb24a || format == mlt_image_opengl )
	{
		AVPicture output;
		avpicture_fill( &output, buffer, PIX_FMT_RGB32, width, height );
		img_convert( &output, PIX_FMT_RGB32, (AVPicture *)frame, pix_fmt, width, height );
	}
	else
	{
		AVPicture output;
		avpicture_fill( &output, buffer, PIX_FMT_YUYV422, width, height );
		img_convert( &output, PIX_FMT_YUYV422, (AVPicture *)frame, pix_fmt, width, height );
	}
#endif
}

/** Allocate the image buffer and set it on the frame.
*/

static int allocate_buffer( mlt_properties frame_properties, AVCodecContext *codec_context, uint8_t **buffer, mlt_image_format *format, int *width, int *height )
{
	int size = 0;

	if ( codec_context->width == 0 || codec_context->height == 0 )
		return size;

	*width = codec_context->width;
	*height = codec_context->height;
	mlt_properties_set_int( frame_properties, "width", *width );
	mlt_properties_set_int( frame_properties, "height", *height );

	if ( codec_context->pix_fmt == PIX_FMT_RGB32 )
		size = *width * ( *height + 1 ) * 4;
	else switch ( *format )
	{
		case mlt_image_yuv420p:
			size = *width * 3 * ( *height + 1 ) / 2;
			break;
		case mlt_image_rgb24:
			size = *width * ( *height + 1 ) * 3;
			break;
		case mlt_image_rgb24a:
		case mlt_image_opengl:
			size = *width * ( *height + 1 ) * 4;
			break;
		default:
			*format = mlt_image_yuv422;
			size = *width * ( *height + 1 ) * 2;
			break;
	}

	// Construct the output image
	*buffer = mlt_pool_alloc( size );
	if ( *buffer )
		mlt_properties_set_data( frame_properties, "image", *buffer, size, mlt_pool_release, NULL );
	else
		size = 0;

	return size;
}

/** Get an image from a frame.
*/

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the producer
	producer_avformat this = mlt_frame_pop_service( frame );
	mlt_producer producer = this->parent;

	// Get the properties from the frame
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the frame number of this frame
	mlt_position position = mlt_properties_get_position( frame_properties, "avformat_position" );

	// Get the producer properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

	// Fetch the video format context
	AVFormatContext *context = this->video_format;

	// Get the video stream
	AVStream *stream = context->streams[ this->video_index ];

	// Get codec context
	AVCodecContext *codec_context = stream->codec;

	// Get the image cache
	if ( ! this->image_cache && ! mlt_properties_get_int( properties, "noimagecache" ) )
		this->image_cache = mlt_cache_init();
	if ( this->image_cache )
	{
		mlt_cache_item item = mlt_cache_get( this->image_cache, (void*) position );
		*buffer = mlt_cache_item_data( item, format );
		if ( *buffer )
		{
			// Set the resolution
			*width = codec_context->width;
			*height = codec_context->height;
			mlt_properties_set_int( frame_properties, "width", *width );
			mlt_properties_set_int( frame_properties, "height", *height );

			// Cache hit
			int size;
			switch ( *format )
			{
				case mlt_image_yuv420p:
					size = *width * 3 * ( *height + 1 ) / 2;
					break;
				case mlt_image_rgb24:
					size = *width * ( *height + 1 ) * 3;
					break;
				case mlt_image_rgb24a:
				case mlt_image_opengl:
					size = *width * ( *height + 1 ) * 4;
					break;
				default:
					*format = mlt_image_yuv422;
					size = *width * ( *height + 1 ) * 2;
					break;
			}
			mlt_properties_set_data( frame_properties, "avformat.image_cache", item, 0, ( mlt_destructor )mlt_cache_item_close, NULL );
			mlt_properties_set_data( frame_properties, "image", *buffer, size, NULL, NULL );
			this->top_field_first = mlt_properties_get_int( frame_properties, "top_field_first" );
			this->got_picture = 1;

			goto exit_get_image;
		}
	}
	// Cache miss
	int image_size = 0;

	avformat_lock();

	// Packet
	AVPacket pkt;

	// Special case pause handling flag
	int paused = 0;

	// Special case ffwd handling
	int ignore = 0;

	// We may want to use the source fps if available
	double source_fps = mlt_properties_get_double( properties, "source_fps" );
	double fps = mlt_producer_get_fps( producer );

	// This is the physical frame position in the source
	int req_position = ( int )( position / fps * source_fps + 0.5 );

	// Determines if we have to decode all frames in a sequence
	// Temporary hack to improve intra frame only
	int must_decode = strcmp( codec_context->codec->name, "dnxhd" ) &&
				  strcmp( codec_context->codec->name, "dvvideo" ) &&
				  strcmp( codec_context->codec->name, "huffyuv" ) &&
				  strcmp( codec_context->codec->name, "mjpeg" ) &&
				  strcmp( codec_context->codec->name, "rawvideo" );

	int last_position = this->last_position;

	// Turn on usage of new seek API and PTS for seeking
	int use_new_seek = codec_context->codec_id == CODEC_ID_H264 && !strcmp( context->iformat->name, "mpegts" );
	if ( mlt_properties_get( properties, "new_seek" ) )
		use_new_seek = mlt_properties_get_int( properties, "new_seek" );

	// Seek if necessary
	if ( position != this->video_expected || last_position < 0 )
	{
		if ( this->av_frame && position + 1 == this->video_expected )
		{
			// We're paused - use last image
			paused = 1;
		}
		else if ( !this->seekable && position > this->video_expected && ( position - this->video_expected ) < 250 )
		{
			// Fast forward - seeking is inefficient for small distances - just ignore following frames
			ignore = ( int )( ( position - this->video_expected ) / fps * source_fps );
			codec_context->skip_loop_filter = AVDISCARD_NONREF;
		}
		else if ( this->seekable && ( position < this->video_expected || position - this->video_expected >= 12 || last_position < 0 ) )
		{
			if ( use_new_seek && last_position == POSITION_INITIAL )
			{
				// find first key frame
				int ret = 0;
				int toscan = 100;

				while ( ret >= 0 && toscan-- > 0 )
				{
					ret = av_read_frame( context, &pkt );
					if ( ret >= 0 && ( pkt.flags & PKT_FLAG_KEY ) && pkt.stream_index == this->video_index )
					{
						mlt_log_verbose( MLT_PRODUCER_SERVICE(producer), "first_pts %lld dts %lld pts_dts_delta %d\n", pkt.pts, pkt.dts, (int)(pkt.pts - pkt.dts) );
						this->first_pts = pkt.pts;
						toscan = 0;
					}
					av_free_packet( &pkt );
				}
				// Rewind
				av_seek_frame( context, -1, 0, AVSEEK_FLAG_BACKWARD );
			}

			// Calculate the timestamp for the requested frame
			int64_t timestamp;
			if ( use_new_seek )
			{
				timestamp = ( req_position - 0.1 / source_fps ) /
					( av_q2d( stream->time_base ) * source_fps );
				mlt_log_verbose( MLT_PRODUCER_SERVICE(producer), "pos %d pts %lld ", req_position, timestamp );
				if ( this->first_pts > 0 )
					timestamp += this->first_pts;
				else if ( context->start_time != AV_NOPTS_VALUE )
					timestamp += context->start_time;
			}
			else
			{
				timestamp = ( int64_t )( ( double )req_position / source_fps * AV_TIME_BASE + 0.5 );
				if ( context->start_time != AV_NOPTS_VALUE )
					timestamp += context->start_time;
			}
			if ( must_decode )
				timestamp -= AV_TIME_BASE;
			if ( timestamp < 0 )
				timestamp = 0;
			mlt_log_debug( MLT_PRODUCER_SERVICE(producer), "seeking timestamp %lld position %d expected %d last_pos %d\n",
				timestamp, position, this->video_expected, last_position );

			// Seek to the timestamp
			if ( use_new_seek )
			{
				codec_context->skip_loop_filter = AVDISCARD_NONREF;
				av_seek_frame( context, this->video_index, timestamp, AVSEEK_FLAG_BACKWARD );
			}
			else
			{
				av_seek_frame( context, -1, timestamp, AVSEEK_FLAG_BACKWARD );
			}

			// Remove the cached info relating to the previous position
			this->current_position = POSITION_INVALID;
			this->last_position = POSITION_INVALID;
			av_freep( &this->av_frame );

			if ( use_new_seek )
			{
				// flush any pictures still in decode buffer
				avcodec_flush_buffers( codec_context );
			}
		}
	}

	// Duplicate the last image if necessary (see comment on rawvideo below)
	if ( this->av_frame && this->av_frame->linesize[0] && this->got_picture && this->seekable
		 && ( paused
			  || this->current_position == req_position
			  || ( !use_new_seek && this->current_position > req_position ) ) )
	{
		// Duplicate it
		if ( ( image_size = allocate_buffer( frame_properties, codec_context, buffer, format, width, height ) ) )
#ifdef VDPAU
			if ( this->vdpau && this->vdpau->buffer )
			{
				AVPicture picture;
				picture.data[0] = this->vdpau->buffer;
				picture.data[2] = this->vdpau->buffer + codec_context->width * codec_context->height;
				picture.data[1] = this->vdpau->buffer + codec_context->width * codec_context->height * 5 / 4;
				picture.linesize[0] = codec_context->width;
				picture.linesize[1] = codec_context->width / 2;
				picture.linesize[2] = codec_context->width / 2;
				convert_image( (AVFrame*) &picture, *buffer, PIX_FMT_YUV420P, format, *width, *height );
			}
			else
#endif
			convert_image( this->av_frame, *buffer, codec_context->pix_fmt, format, *width, *height );
		else
			mlt_frame_get_image( frame, buffer, format, width, height, writable );
	}
	else
	{
		int ret = 0;
		int int_position = 0;
		int decode_errors = 0;
		int got_picture = 0;

		av_init_packet( &pkt );

		// Construct an AVFrame for YUV422 conversion
		if ( !this->av_frame )
			this->av_frame = avcodec_alloc_frame( );

		while( ret >= 0 && !got_picture )
		{
			// Read a packet
			ret = av_read_frame( context, &pkt );

			// We only deal with video from the selected video_index
			if ( ret >= 0 && pkt.stream_index == this->video_index && pkt.size > 0 )
			{
				// Determine time code of the packet
				if ( use_new_seek )
				{
					int64_t pts = pkt.pts;
					if ( this->first_pts > 0 )
						pts -= this->first_pts;
					else if ( context->start_time != AV_NOPTS_VALUE )
						pts -= context->start_time;
					int_position = ( int )( av_q2d( stream->time_base ) * pts * source_fps + 0.1 );
					if ( pkt.pts == AV_NOPTS_VALUE )
					{
						this->invalid_pts_counter++;
						if ( this->invalid_pts_counter > 20 )
						{
							mlt_log_panic( MLT_PRODUCER_SERVICE(producer), "\ainvalid PTS; DISABLING NEW_SEEK!\n" );
							mlt_properties_set_int( properties, "new_seek", 0 );
							int_position = req_position;
							use_new_seek = 0;
						}
					}
					else
					{
						this->invalid_pts_counter = 0;
					}
					mlt_log_debug( MLT_PRODUCER_SERVICE(producer), "pkt.pts %llu req_pos %d cur_pos %d pkt_pos %d\n",
						pkt.pts, req_position, this->current_position, int_position );
				}
				else
				{
					if ( pkt.dts != AV_NOPTS_VALUE )
					{
						int_position = ( int )( av_q2d( stream->time_base ) * pkt.dts * source_fps + 0.5 );
						if ( context->start_time != AV_NOPTS_VALUE )
							int_position -= ( int )( context->start_time * source_fps / AV_TIME_BASE + 0.5 );
						last_position = this->last_position;
						if ( int_position == last_position )
							int_position = last_position + 1;
					}
					else
					{
						int_position = req_position;
					}
					mlt_log_debug( MLT_PRODUCER_SERVICE(producer), "pkt.dts %llu req_pos %d cur_pos %d pkt_pos %d\n",
						pkt.dts, req_position, this->current_position, int_position );
					// Make a dumb assumption on streams that contain wild timestamps
					if ( abs( req_position - int_position ) > 999 )
					{
						int_position = req_position;
						mlt_log_debug( MLT_PRODUCER_SERVICE(producer), " WILD TIMESTAMP!" );
					}
				}
				this->last_position = int_position;

				// Decode the image
				if ( must_decode || int_position >= req_position )
				{
#ifdef VDPAU
					if ( g_vdpau && this->vdpau )
					{
						if ( g_vdpau->producer != this )
						{
							vdpau_decoder_close();
							vdpau_decoder_init( this );
						}
						if ( this->vdpau )
							this->vdpau->is_decoded = 0;
					}
#endif
					codec_context->reordered_opaque = pkt.pts;
					if ( int_position >= req_position )
						codec_context->skip_loop_filter = AVDISCARD_NONE;
#if (LIBAVCODEC_VERSION_INT >= ((52<<16)+(26<<8)+0))
					ret = avcodec_decode_video2( codec_context, this->av_frame, &got_picture, &pkt );
#else
					ret = avcodec_decode_video( codec_context, this->av_frame, &got_picture, pkt.data, pkt.size );
#endif
					// Note: decode may fail at the beginning of MPEGfile (B-frames referencing before first I-frame), so allow a few errors.
					if ( ret < 0 )
					{
						if ( ++decode_errors <= 10 )
							ret = 0;
					}
					else
					{
						decode_errors = 0;
					}
				}

				if ( got_picture )
				{
					if ( use_new_seek )
					{
						// Determine time code of the packet
						int64_t pts = this->av_frame->reordered_opaque;
						if ( this->first_pts > 0 )
							pts -= this->first_pts;
						else if ( context->start_time != AV_NOPTS_VALUE )
							pts -= context->start_time;
						int_position = ( int )( av_q2d( stream->time_base) * pts * source_fps + 0.1 );
						mlt_log_debug( MLT_PRODUCER_SERVICE(producer), "got frame %d, key %d\n", int_position, this->av_frame->key_frame );
					}
					// Handle ignore
					if ( int_position < req_position )
					{
						ignore = 0;
						got_picture = 0;
					}
					else if ( int_position >= req_position )
					{
						ignore = 0;
						codec_context->skip_loop_filter = AVDISCARD_NONE;
					}
					else if ( ignore -- )
					{
						got_picture = 0;
					}
				}
				mlt_log_debug( MLT_PRODUCER_SERVICE(producer), " got_pic %d key %d\n", got_picture, pkt.flags & PKT_FLAG_KEY );
				av_free_packet( &pkt );
			}
			else if ( ret >= 0 )
			{
				av_free_packet( &pkt );
			}

			// Now handle the picture if we have one
			if ( got_picture )
			{
				if ( ( image_size = allocate_buffer( frame_properties, codec_context, buffer, format, width, height ) ) )
				{
#ifdef VDPAU
					if ( this->vdpau )
					{
						if ( this->vdpau->is_decoded )
						{
							struct vdpau_render_state *render = (struct vdpau_render_state*) this->av_frame->data[0];
							void *planes[3];
							uint32_t pitches[3];
							VdpYCbCrFormat dest_format = VDP_YCBCR_FORMAT_YV12;
							AVPicture picture;
							
							if ( !this->vdpau->buffer )
								this->vdpau->buffer = mlt_pool_alloc( codec_context->width * codec_context->height * 3 / 2 );
							picture.data[0] = planes[0] = this->vdpau->buffer;
							picture.data[2] = planes[1] = this->vdpau->buffer + codec_context->width * codec_context->height;
							picture.data[1] = planes[2] = this->vdpau->buffer + codec_context->width * codec_context->height * 5 / 4;
							picture.linesize[0] = pitches[0] = codec_context->width;
							picture.linesize[1] = pitches[1] = codec_context->width / 2;
							picture.linesize[2] = pitches[2] = codec_context->width / 2;

							VdpStatus status = vdp_surface_get_bits( render->surface, dest_format, planes, pitches );
							if ( status == VDP_STATUS_OK )
							{
								convert_image( (AVFrame*) &picture, *buffer, PIX_FMT_YUV420P, format, *width, *height );
							}
							else
							{
								mlt_log_error( MLT_PRODUCER_SERVICE(producer), "VDPAU Error: %s\n", vdp_get_error_string( status ) );
								this->vdpau->is_decoded = 0;
							}
						}
						else
						{
							mlt_log_error( MLT_PRODUCER_SERVICE(producer), "VDPAU error in VdpDecoderRender\n" );
							got_picture = 0;
						}
					}
					else
#endif
					convert_image( this->av_frame, *buffer, codec_context->pix_fmt, format, *width, *height );
					this->top_field_first |= this->av_frame->top_field_first;
					this->current_position = int_position;
					this->got_picture = 1;
				}
				else
				{
					got_picture = 0;
				}
			}
		}
	}

	avformat_unlock();

	if ( this->got_picture && image_size > 0 && this->image_cache )
	{
		// Copy buffer to image cache	
		uint8_t *image = mlt_pool_alloc( image_size );
		memcpy( image, *buffer, image_size );
		mlt_cache_put( this->image_cache, (void*) position, image, *format, mlt_pool_release );
	}

	// Regardless of speed, we expect to get the next frame (cos we ain't too bright)
	this->video_expected = position + 1;

exit_get_image:
	// Set the progressive flag
	if ( mlt_properties_get( properties, "force_progressive" ) )
		mlt_properties_set_int( frame_properties, "progressive", !!mlt_properties_get_int( properties, "force_progressive" ) );
	else if ( this->av_frame )
		mlt_properties_set_int( frame_properties, "progressive", !this->av_frame->interlaced_frame );

	// Set the field order property for this frame
	mlt_properties_set_int( frame_properties, "top_field_first", this->top_field_first );

	return !this->got_picture;
}

/** Process properties as AVOptions and apply to AV context obj
*/

static void apply_properties( void *obj, mlt_properties properties, int flags )
{
	int i;
	int count = mlt_properties_count( properties );
	for ( i = 0; i < count; i++ )
	{
		const char *opt_name = mlt_properties_get_name( properties, i );
		const AVOption *opt = av_find_opt( obj, opt_name, NULL, flags, flags );
		if ( opt_name && mlt_properties_get( properties, opt_name ) )
		{
			if ( opt )
#if LIBAVCODEC_VERSION_INT >= ((52<<16)+(7<<8)+0)
				av_set_string3( obj, opt_name, mlt_properties_get( properties, opt_name), 0, NULL );
#elif LIBAVCODEC_VERSION_INT >= ((51<<16)+(59<<8)+0)
				av_set_string2( obj, opt_name, mlt_properties_get( properties, opt_name), 0 );
#else
				av_set_string( obj, opt_name, mlt_properties_get( properties, opt_name) );
#endif
		}
	}
}

/** Initialize the video codec context.
 */

static int video_codec_init( producer_avformat this, int index, mlt_properties properties )
{
	// Initialise the codec if necessary
	if ( !this->video_codec )
	{
		// Get the video stream
		AVStream *stream = this->video_format->streams[ index ];

		// Get codec context
		AVCodecContext *codec_context = stream->codec;

		// Find the codec
		AVCodec *codec = avcodec_find_decoder( codec_context->codec_id );
#ifdef VDPAU
		if ( codec_context->codec_id == CODEC_ID_H264 )
		{
			if ( ( codec = avcodec_find_decoder_by_name( "h264_vdpau" ) ) )
			{
				if ( vdpau_init( this ) )
				{
					this->video_codec = codec_context;
					if ( !vdpau_decoder_init( this ) )
						vdpau_decoder_close();
				}
			}
			if ( !this->vdpau )
				codec = avcodec_find_decoder( codec_context->codec_id );
		}
#endif

		// Initialise multi-threading
		int thread_count = mlt_properties_get_int( properties, "threads" );
		if ( thread_count == 0 && getenv( "MLT_AVFORMAT_THREADS" ) )
			thread_count = atoi( getenv( "MLT_AVFORMAT_THREADS" ) );
		if ( thread_count > 1 )
		{
			avcodec_thread_init( codec_context, thread_count );
			codec_context->thread_count = thread_count;
		}

		// If we don't have a codec and we can't initialise it, we can't do much more...
		avformat_lock( );
		if ( codec && avcodec_open( codec_context, codec ) >= 0 )
		{
			// Now store the codec with its destructor
			this->video_codec = codec_context;
		}
		else
		{
			// Remember that we can't use this later
			this->video_index = -1;
		}
		avformat_unlock( );

		// Process properties as AVOptions
		apply_properties( codec_context, properties, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM );

		// Reset some image properties
		mlt_properties_set_int( properties, "width", this->video_codec->width );
		mlt_properties_set_int( properties, "height", this->video_codec->height );
		// For DV, we'll just use the saved aspect ratio
		if ( codec_context->codec_id != CODEC_ID_DVVIDEO )
			mlt_properties_set_double( properties, "aspect_ratio", get_aspect_ratio( stream, this->video_codec, NULL ) );

		// Determine the fps first from the codec
		double source_fps = (double) this->video_codec->time_base.den /
								   ( this->video_codec->time_base.num == 0 ? 1 : this->video_codec->time_base.num );
		
		if ( mlt_properties_get( properties, "force_fps" ) )
		{
			source_fps = mlt_properties_get_double( properties, "force_fps" );
			stream->time_base = av_d2q( source_fps, 255 );
		}
		else
		{
			// If the muxer reports a frame rate different than the codec
			double muxer_fps = av_q2d( stream->r_frame_rate );
			// Choose the lesser - the wrong tends to be off by some multiple of 10
			source_fps = FFMIN( source_fps, muxer_fps );
		}

		// We'll use fps if it's available
		if ( source_fps > 0 )
			mlt_properties_set_double( properties, "source_fps", source_fps );
		else
			mlt_properties_set_double( properties, "source_fps", mlt_producer_get_fps( this->parent ) );
	}
	return this->video_codec && this->video_index > -1;
}

/** Set up video handling.
*/

static void producer_set_up_video( producer_avformat this, mlt_frame frame )
{
	// Get the producer
	mlt_producer producer = this->parent;

	// Get the properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

	// Fetch the video format context
	AVFormatContext *context = this->video_format;

	// Get the video_index
	int index = mlt_properties_get_int( properties, "video_index" );

	// Reopen the file if necessary
	if ( !context && index > -1 )
	{
		mlt_events_block( properties, producer );
		producer_open( this, mlt_service_profile( MLT_PRODUCER_SERVICE(producer) ),
			mlt_properties_get( properties, "resource" ) );
		context = this->video_format;
		if ( this->dummy_context )
		{
			avformat_lock();
			av_close_input_file( this->dummy_context );
			avformat_unlock();
		}
		this->dummy_context = NULL;
		mlt_events_unblock( properties, producer );
		if ( this->audio_format )
			get_audio_streams_info( this );

		// Process properties as AVOptions
		apply_properties( context, properties, AV_OPT_FLAG_DECODING_PARAM );
	}

	// Exception handling for video_index
	if ( context && index >= (int) context->nb_streams )
	{
		// Get the last video stream
		for ( index = context->nb_streams - 1;
			  index >= 0 && context->streams[ index ]->codec->codec_type != CODEC_TYPE_VIDEO;
			  index-- );
		mlt_properties_set_int( properties, "video_index", index );
	}
	if ( context && index > -1 && context->streams[ index ]->codec->codec_type != CODEC_TYPE_VIDEO )
	{
		// Invalidate the video stream
		index = -1;
		mlt_properties_set_int( properties, "video_index", index );
	}

	// Update the video properties if the index changed
	if ( index != this->video_index )
	{
		// Reset the video properties if the index changed
		this->video_index = index;
		if ( this->video_codec )
		{
			avformat_lock();
			avcodec_close( this->video_codec );
			avformat_unlock();
		}
		this->video_codec = NULL;
	}

	// Get the frame properties
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Get the codec
	if ( context && index > -1 && video_codec_init( this, index, properties ) )
	{
		// Set the frame properties
		double force_aspect_ratio = mlt_properties_get_double( properties, "force_aspect_ratio" );
		double aspect_ratio = ( force_aspect_ratio > 0.0 ) ?
			force_aspect_ratio : mlt_properties_get_double( properties, "aspect_ratio" );

		// Set the width and height
		mlt_properties_set_int( frame_properties, "width", this->video_codec->width );
		mlt_properties_set_int( frame_properties, "height", this->video_codec->height );
		mlt_properties_set_int( frame_properties, "real_width", this->video_codec->width );
		mlt_properties_set_int( frame_properties, "real_height", this->video_codec->height );
		mlt_properties_set_double( frame_properties, "aspect_ratio", aspect_ratio );

		// Add our image operation
		mlt_frame_push_service( frame, this );
		mlt_frame_push_get_image( frame, producer_get_image );
	}
	else
	{
		// If something failed, use test card image
		mlt_properties_set_int( frame_properties, "test_image", 1 );
	}
}

static int seek_audio( producer_avformat this, mlt_position position, double timecode, int *ignore )
{
	int paused = 0;

	// Seek if necessary
	if ( position != this->audio_expected )
	{
		if ( position + 1 == this->audio_expected )
		{
			// We're paused - silence required
			paused = 1;
		}
		else if ( !this->seekable && position > this->audio_expected && ( position - this->audio_expected ) < 250 )
		{
			// Fast forward - seeking is inefficient for small distances - just ignore following frames
			*ignore = position - this->audio_expected;
		}
		else if ( position < this->audio_expected || position - this->audio_expected >= 12 )
		{
			AVFormatContext *context = this->audio_format;
			int64_t timestamp = ( int64_t )( timecode * AV_TIME_BASE + 0.5 );
			if ( context->start_time != AV_NOPTS_VALUE )
				timestamp += context->start_time;
			if ( timestamp < 0 )
				timestamp = 0;

			// Set to the real timecode
			if ( av_seek_frame( context, -1, timestamp, AVSEEK_FLAG_BACKWARD ) != 0 )
				paused = 1;

			// Clear the usage in the audio buffer
			int i = MAX_AUDIO_STREAMS + 1;
			while ( --i )
				this->audio_used[i - 1] = 0;
		}
	}
	return paused;
}

static int decode_audio( producer_avformat this, int *ignore, AVPacket *pkt, int channels, int samples, double timecode, double fps )
{
	// Fetch the audio_format
	AVFormatContext *context = this->audio_format;

	// Get the current stream index
	int index = pkt->stream_index;

	// Get codec context
	AVCodecContext *codec_context = this->audio_codec[ index ];

	// Obtain the resample context if it exists (not always needed)
	ReSampleContext *resample = this->audio_resample[ index ];

	// Obtain the audio buffers
	int16_t *audio_buffer = this->audio_buffer[ index ];
	int16_t *decode_buffer = this->decode_buffer[ index ];

	int audio_used = this->audio_used[ index ];
	uint8_t *ptr = pkt->data;
	int len = pkt->size;
	int ret = 0;

	while ( ptr && ret >= 0 && len > 0 )
	{
		int data_size = sizeof( int16_t ) * AVCODEC_MAX_AUDIO_FRAME_SIZE;

		// Decode the audio
#if (LIBAVCODEC_VERSION_INT >= ((52<<16)+(26<<8)+0))
		ret = avcodec_decode_audio3( codec_context, decode_buffer, &data_size, pkt );
#elif (LIBAVCODEC_VERSION_INT >= ((51<<16)+(29<<8)+0))
		ret = avcodec_decode_audio2( codec_context, decode_buffer, &data_size, ptr, len );
#else
		ret = avcodec_decode_audio( codec_context, decode_buffer, &data_size, ptr, len );
#endif
		if ( ret < 0 )
		{
			mlt_log_warning( MLT_PRODUCER_SERVICE(this->parent), "audio decoding error %d\n", ret );
			break;
		}

		len -= ret;
		ptr += ret;

		// If decoded successfully
		if ( data_size > 0 )
		{
			// Figure out how many samples will be needed after resampling
			int convert_samples = data_size / codec_context->channels / ( av_get_bits_per_sample_format( codec_context->sample_fmt ) / 8 );
			int samples_needed = this->resample_factor * convert_samples + 1;
			
			// Resize audio buffer to prevent overflow
			if ( audio_used * channels + samples_needed > this->audio_buffer_size[ index ] )
			{
				this->audio_buffer_size[ index ] *= 2;
				audio_buffer = this->audio_buffer[ index ] = mlt_pool_realloc( audio_buffer, this->audio_buffer_size[ index ] * sizeof(int16_t) );
			}
			if ( resample )
			{
				// Copy to audio buffer while resampling
				int16_t *source = decode_buffer;
				int16_t *dest = &audio_buffer[ audio_used * channels ];
				audio_used += audio_resample( resample, dest, source, convert_samples );
			}
			else
			{
				// Straight copy to audio buffer
				memcpy( &audio_buffer[ audio_used * codec_context->channels ], decode_buffer, data_size );
				audio_used += convert_samples;
			}

			// Handle ignore
			while ( *ignore && audio_used > samples )
			{
				*ignore -= 1;
				audio_used -= samples;
				memmove( audio_buffer, &audio_buffer[ samples * (resample? channels : codec_context->channels) ],
				         audio_used * sizeof( int16_t ) );
			}
		}
	}

	// If we're behind, ignore this packet
	if ( pkt->pts >= 0 )
	{
		double current_pts = av_q2d( context->streams[ index ]->time_base ) * pkt->pts;
		int req_position = ( int )( timecode * fps + 0.5 );
		int int_position = ( int )( current_pts * fps + 0.5 );
		if ( context->start_time != AV_NOPTS_VALUE )
			int_position -= ( int )( fps * context->start_time / AV_TIME_BASE + 0.5 );

		if ( this->seekable && *ignore == 0 )
		{
			if ( int_position < req_position )
				// We are behind, so skip some
				*ignore = 1;
			else if ( int_position > req_position + 2 )
				// We are ahead, so seek backwards some more
				seek_audio( this, req_position, timecode - 1.0, ignore );
		}
	}

	this->audio_used[ index ] = audio_used;

	return ret;
}

/** Get the audio from a frame.
*/

static int producer_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the producer
	producer_avformat this = mlt_frame_pop_audio( frame );

	// Obtain the frame number of this frame
	mlt_position position = mlt_properties_get_position( MLT_FRAME_PROPERTIES( frame ), "avformat_position" );

	// Calculate the real time code
	double real_timecode = producer_time_of_frame( this->parent, position );

	// Get the producer fps
	double fps = mlt_producer_get_fps( this->parent );

	// Number of frames to ignore (for ffwd)
	int ignore = 0;

	// Flag for paused (silence)
	int paused = seek_audio( this, position, real_timecode, &ignore );

	// Fetch the audio_format
	AVFormatContext *context = this->audio_format;
	
	// Determine the tracks to use
	int index = this->audio_index;
	int index_max = this->audio_index + 1;
	if ( this->audio_index == INT_MAX )
	{
		index = 0;
		index_max = context->nb_streams;
		*channels = this->total_channels;
		*frequency = this->max_frequency;
	}

	// Initialize the resamplers and buffers
	for ( ; index < index_max; index++ )
	{
		// Get codec context
		AVCodecContext *codec_context = this->audio_codec[ index ];

		if ( codec_context && !this->audio_buffer[ index ] )
		{
			// Check for resample and create if necessary
			if ( codec_context->channels <= 2 )
			{
				// Determine by how much resampling will increase number of samples
				double resample_factor = this->audio_index == INT_MAX ? 1 : (double) *channels / codec_context->channels;
				resample_factor *= (double) *frequency / codec_context->sample_rate;
				if ( resample_factor > this->resample_factor )
					this->resample_factor = resample_factor;
				
				// Create the resampler
#if (LIBAVCODEC_VERSION_INT >= ((52<<16)+(15<<8)+0))
				this->audio_resample[ index ] = av_audio_resample_init(
					this->audio_index == INT_MAX ? codec_context->channels : *channels,
					codec_context->channels, *frequency, codec_context->sample_rate,
					SAMPLE_FMT_S16, codec_context->sample_fmt, 16, 10, 0, 0.8 );
#else
				this->audio_resample[ index ] = audio_resample_init(
					this->audio_index == INT_MAX ? codec_context->channels : *channels,
					codec_context->channels, *frequency, codec_context->sample_rate );
#endif
			}
			else
			{
				codec_context->request_channels = this->audio_index == INT_MAX ? codec_context->channels : *channels;
			}

			// Check for audio buffer and create if necessary
			this->audio_buffer_size[ index ] = AVCODEC_MAX_AUDIO_FRAME_SIZE;
			this->audio_buffer[ index ] = mlt_pool_alloc( this->audio_buffer_size[ index ] * sizeof( int16_t ) );

			// Check for decoder buffer and create if necessary
			this->decode_buffer[ index ] = av_malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE * sizeof( int16_t ) );
		}
	}

	// Get the audio if required
	if ( !paused )
	{
		int ret = 0;
		int got_audio = 0;
		AVPacket pkt;

		av_init_packet( &pkt );
		
		// If not resampling, give consumer more than requested.
		// It requested number samples based on requested frame rate.
		// Do not clean this up with a samples *= ...!
		if ( this->audio_index != INT_MAX && ! this->audio_resample[ this->audio_index ] )
			*samples = *samples * this->audio_codec[ this->audio_index ]->sample_rate / *frequency;

		while ( ret >= 0 && !got_audio )
		{
			// Check if the buffer already contains the samples required
			if ( this->audio_index != INT_MAX && this->audio_used[ this->audio_index ] >= *samples && ignore == 0 )
			{
				got_audio = 1;
				break;
			}

			// Read a packet
			ret = av_read_frame( context, &pkt );

			// We only deal with audio from the selected audio index
			if ( ret >= 0 && pkt.data && pkt.size > 0 && ( pkt.stream_index == this->audio_index ||
				 ( this->audio_index == INT_MAX && context->streams[ pkt.stream_index ]->codec->codec_type == CODEC_TYPE_AUDIO ) ) )
			{
				int channels2 = this->audio_index == INT_MAX ? this->audio_codec[pkt.stream_index]->channels : *channels;
				ret = decode_audio( this, &ignore, &pkt, channels2, *samples, real_timecode, fps );
			}
			av_free_packet( &pkt );

			if ( this->audio_index == INT_MAX && ret >= 0 )
			{
				// Determine if there is enough audio for all streams
				got_audio = 1;
				for ( index = 0; index < context->nb_streams; index++ )
				{
					if ( this->audio_codec[ index ] && this->audio_used[ index ] < *samples )
						got_audio = 0;
				}
			}
		}
		
		// Allocate and set the frame's audio buffer
		int size = *samples * *channels * sizeof(int16_t);
		*buffer = mlt_pool_alloc( size );
		*format = mlt_audio_s16;
		mlt_frame_set_audio( frame, *buffer, *format, size, mlt_pool_release );

		// Interleave tracks if audio_index=all
		if ( this->audio_index == INT_MAX )
		{
			int16_t *dest = *buffer;
			int i;
			for ( i = 0; i < *samples; i++ )
			{
				for ( index = 0; index < index_max; index++ )
				if ( this->audio_codec[ index ] )
				{
					int current_channels = this->audio_codec[ index ]->channels;
					int16_t *src = this->audio_buffer[ index ] + i * current_channels;
					memcpy( dest, src, current_channels * sizeof(int16_t) );
					dest += current_channels;
				}
			}
			for ( index = 0; index < index_max; index++ )
			if ( this->audio_codec[ index ] && this->audio_used[ index ] >= *samples )
			{
				int current_channels = this->audio_codec[ index ]->channels;
				int16_t *src = this->audio_buffer[ index ] + *samples * current_channels;
				this->audio_used[index] -= *samples;
				memmove( this->audio_buffer[ index ], src, this->audio_used[ index ] * current_channels * sizeof(int16_t) );
			}
		}
		// Copy a single track to the output buffer
		else
		{
			index = this->audio_index;

			// Now handle the audio if we have enough
			if ( this->audio_used[ index ] >= *samples )
			{
				int16_t *src = this->audio_buffer[ index ];
				memcpy( *buffer, src, *samples * *channels * sizeof(int16_t) );
				this->audio_used[ index ] -= *samples;
				memmove( src, &src[ *samples * *channels ], this->audio_used[ index ] * *channels * sizeof(int16_t) );
			}
			else
			{
				// Otherwise fill with silence
				memset( *buffer, 0, *samples * *channels * sizeof(int16_t) );
			}
			if ( !this->audio_resample[ index ] )
			{
				// TODO: uncomment and remove following line when full multi-channel support is ready
				// *channels = codec_context->channels;
				*frequency = this->audio_codec[ index ]->sample_rate;
			}
		}
	}
	else
	{
		// Get silence and don't touch the context
		mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	}
	
	// Regardless of speed (other than paused), we expect to get the next frame
	if ( !paused )
		this->audio_expected = position + 1;

	return 0;
}

/** Initialize the audio codec context.
*/

static int audio_codec_init( producer_avformat this, int index, mlt_properties properties )
{
	// Initialise the codec if necessary
	if ( !this->audio_codec[ index ] )
	{
		// Get codec context
		AVCodecContext *codec_context = this->audio_format->streams[index]->codec;

		// Find the codec
		AVCodec *codec = avcodec_find_decoder( codec_context->codec_id );

		// If we don't have a codec and we can't initialise it, we can't do much more...
		avformat_lock( );
		if ( codec && avcodec_open( codec_context, codec ) >= 0 )
		{
			// Now store the codec with its destructor
			if ( this->audio_codec[ index ] )
				avcodec_close( this->audio_codec[ index ] );
			this->audio_codec[ index ] = codec_context;
		}
		else
		{
			// Remember that we can't use this later
			this->audio_index = -1;
		}
		avformat_unlock( );

		// Process properties as AVOptions
		apply_properties( codec_context, properties, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_DECODING_PARAM );
	}
	return this->audio_codec[ index ] && this->audio_index > -1;
}

/** Set up audio handling.
*/

static void producer_set_up_audio( producer_avformat this, mlt_frame frame )
{
	// Get the producer
	mlt_producer producer = this->parent;

	// Get the properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

	// Fetch the audio format context
	AVFormatContext *context = this->audio_format;

	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Get the audio_index
	int index = mlt_properties_get_int( properties, "audio_index" );

	// Handle all audio tracks
	if ( this->audio_index > -1 &&
	     mlt_properties_get( properties, "audio_index" ) &&
	     !strcmp( mlt_properties_get( properties, "audio_index" ), "all" ) )
		index = INT_MAX;

	// Reopen the file if necessary
	if ( !context && this->audio_index > -1 && index > -1 )
	{
		mlt_events_block( properties, producer );
		producer_open( this, mlt_service_profile( MLT_PRODUCER_SERVICE(producer) ),
			mlt_properties_get( properties, "resource" ) );
		context = this->audio_format;
		if ( this->dummy_context )
		{
			avformat_lock();
			av_close_input_file( this->dummy_context );
			avformat_unlock();
		}
		this->dummy_context = NULL;
		mlt_events_unblock( properties, producer );
		get_audio_streams_info( this );
	}

	// Exception handling for audio_index
	if ( context && index >= (int) context->nb_streams && index < INT_MAX )
	{
		for ( index = context->nb_streams - 1;
			  index >= 0 && context->streams[ index ]->codec->codec_type != CODEC_TYPE_AUDIO;
			  index-- );
		mlt_properties_set_int( properties, "audio_index", index );
	}
	if ( context && index > -1 && index < INT_MAX &&
		 context->streams[ index ]->codec->codec_type != CODEC_TYPE_AUDIO )
	{
		index = this->audio_index;
		mlt_properties_set_int( properties, "audio_index", index );
	}

	// Update the audio properties if the index changed
	if ( context && index > -1 && index != this->audio_index )
	{
		if ( this->audio_codec[ this->audio_index ] )
		{
			avformat_lock();
			avcodec_close( this->audio_codec[ this->audio_index ] );
			avformat_unlock();
		}
		this->audio_codec[ this->audio_index ] = NULL;
	}
	if ( this->audio_index != -1 )
		this->audio_index = index;
	else
		index = -1;

	// Get the codec(s)
	if ( context && index == INT_MAX )
	{
		mlt_properties_set_int( frame_properties, "frequency", this->max_frequency );
		mlt_properties_set_int( frame_properties, "channels", this->total_channels );
		for ( index = 0; index < context->nb_streams; index++ )
		{
			if ( context->streams[ index ]->codec->codec_type == CODEC_TYPE_AUDIO )
				audio_codec_init( this, index, properties );
		}
	}
	else if ( context && index > -1 && audio_codec_init( this, index, properties ) )
	{
		// Set the frame properties
		if ( index < INT_MAX )
		{
			mlt_properties_set_int( frame_properties, "frequency", this->audio_codec[ index ]->sample_rate );
			mlt_properties_set_int( frame_properties, "channels", this->audio_codec[ index ]->channels );
		}
	}
	if ( context && index > -1 )
	{
		// Add our audio operation
		mlt_frame_push_audio( frame, this );
		mlt_frame_push_audio( frame, producer_get_audio );
	}
}

/** Our get frame implementation.
*/

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Access the private data
	mlt_service service = MLT_PRODUCER_SERVICE( producer );
	mlt_cache_item cache_item = mlt_service_cache_get( service, "producer_avformat" );
	producer_avformat this = mlt_cache_item_data( cache_item, NULL );

	// If cache miss
	if ( !this )
	{
		this = calloc( 1, sizeof( struct producer_avformat_s ) );
		producer->child = this;
		this->parent = producer;
		mlt_service_cache_put( service, "producer_avformat", this, 0, (mlt_destructor) producer_avformat_close );
		cache_item = mlt_service_cache_get( service, "producer_avformat" );
	}

	// Create an empty frame
	*frame = mlt_frame_init( service);
	
	if ( *frame )
	{
		mlt_properties_set_data( MLT_FRAME_PROPERTIES(*frame), "avformat_cache", cache_item, 0, (mlt_destructor) mlt_cache_item_close, NULL );
	}
	else
	{
		mlt_cache_item_close( cache_item );
		return 1;
	}

	// Update timecode on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

	// Set the position of this producer
	mlt_properties_set_position( MLT_FRAME_PROPERTIES( *frame ), "avformat_position", mlt_producer_frame( producer ) );
	
	// Set up the video
	producer_set_up_video( this, *frame );

	// Set up the audio
	producer_set_up_audio( this, *frame );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_avformat_close( producer_avformat this )
{
	mlt_log_debug( NULL, "producer_avformat_close\n" );
	// Close the file
	av_free( this->av_frame );
	avformat_lock();
	int i;
	for ( i = 0; i < MAX_AUDIO_STREAMS; i++ )
	{
		if ( this->audio_resample[i] )
			audio_resample_close( this->audio_resample[i] );
		mlt_pool_release( this->audio_buffer[i] );
		av_free( this->decode_buffer[i] );
		if ( this->audio_codec[i] )
			avcodec_close( this->audio_codec[i] );
	}
	if ( this->video_codec )
		avcodec_close( this->video_codec );
	if ( this->dummy_context )
		av_close_input_file( this->dummy_context );
	if ( this->audio_format )
		av_close_input_file( this->audio_format );
	if ( this->video_format )
		av_close_input_file( this->video_format );
	avformat_unlock();
#ifdef VDPAU
	vdpau_producer_close( this );
#endif
	if ( this->image_cache )
		mlt_cache_close( this->image_cache );
	free( this );
}

static void producer_close( mlt_producer parent )
{
	// Close the parent
	parent->close = NULL;
	mlt_producer_close( parent );

	// Free the memory
	free( parent );
}
