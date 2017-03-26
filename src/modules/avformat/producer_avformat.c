/*
 * producer_avformat.c -- avformat producer
 * Copyright (C) 2003-2017 Meltytech, LLC
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// MLT Header files
#include <framework/mlt_producer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_profile.h>
#include <framework/mlt_log.h>
#include <framework/mlt_deque.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_cache.h>
#include <framework/mlt_slices.h>

// ffmpeg Header files
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/samplefmt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/dict.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>

#ifdef VDPAU
#  include <libavcodec/vdpau.h>
#endif

#ifdef AVFILTER
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#endif

// System header files
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>
#include <math.h>
#include <wchar.h>

#if LIBAVCODEC_VERSION_MAJOR < 55
#define AV_CODEC_ID_H264    CODEC_ID_H264
#endif

#define POSITION_INITIAL (-2)
#define POSITION_INVALID (-1)

#define MAX_AUDIO_STREAMS (32)
#define MAX_VDPAU_SURFACES (10)
#define MAX_AUDIO_FRAME_SIZE (192000) // 1 second of 48khz 32bit audio

struct producer_avformat_s
{
	mlt_producer parent;
	AVFormatContext *dummy_context;
	AVFormatContext *audio_format;
	AVFormatContext *video_format;
	AVCodecContext *audio_codec[ MAX_AUDIO_STREAMS ];
	AVCodecContext *video_codec;
	AVFrame *video_frame;
	AVFrame *audio_frame;
	AVPacket pkt;
	mlt_position audio_expected;
	mlt_position video_expected;
	int audio_index;
	int video_index;
	int64_t first_pts;
	int64_t last_position;
	int seekable;
	int64_t current_position;
	mlt_position nonseek_position;
	int top_field_first;
	uint8_t *audio_buffer[ MAX_AUDIO_STREAMS ];
	size_t audio_buffer_size[ MAX_AUDIO_STREAMS ];
	uint8_t *decode_buffer[ MAX_AUDIO_STREAMS ];
	int audio_used[ MAX_AUDIO_STREAMS ];
	int audio_streams;
	int audio_max_stream;
	int total_channels;
	int max_channel;
	int max_frequency;
	unsigned int invalid_pts_counter;
	unsigned int invalid_dts_counter;
	mlt_cache image_cache;
	int yuv_colorspace, color_primaries, color_trc;
	int full_luma;
	pthread_mutex_t video_mutex;
	pthread_mutex_t audio_mutex;
	mlt_deque apackets;
	mlt_deque vpackets;
	pthread_mutex_t packets_mutex;
	pthread_mutex_t open_mutex;
	int is_mutex_init;
	AVRational video_time_base;
	mlt_frame last_good_frame; // for video error concealment
	int last_good_position;    // for video error concealment
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

		VdpDevice device;
		VdpDecoder decoder;
	} *vdpau;
#endif
#ifdef AVFILTER
	AVFilterGraph *vfilter_graph;
	AVFilterContext *vfilter_in;
	AVFilterContext* vfilter_out;
#endif
	int autorotate;
};
typedef struct producer_avformat_s *producer_avformat;

// Forward references.
static int list_components( char* file );
static int producer_open( producer_avformat self, mlt_profile profile, const char *URL, int take_lock, int test_open );
static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index );
static void producer_avformat_close( producer_avformat );
static void producer_close( mlt_producer parent );
static void producer_set_up_video( producer_avformat self, mlt_frame frame );
static void producer_set_up_audio( producer_avformat self, mlt_frame frame );
static void apply_properties( void *obj, mlt_properties properties, int flags );
static int video_codec_init( producer_avformat self, int index, mlt_properties properties );
static void get_audio_streams_info( producer_avformat self );
static mlt_audio_format pick_audio_format( int sample_fmt );
static int pick_av_pixel_format( int *pix_fmt );

#ifdef VDPAU
#include "vdpau.c"
#endif

/** Constructor for libavformat.
*/

mlt_producer producer_avformat_init( mlt_profile profile, const char *service, char *file )
{
	if ( list_components( file ) )
		return NULL;

	mlt_producer producer = NULL;

	// Check that we have a non-NULL argument
	if ( file )
	{
		// Construct the producer
		producer_avformat self = calloc( 1, sizeof( struct producer_avformat_s ) );
		producer = calloc( 1, sizeof( struct mlt_producer_s ) );

		// Initialise it
		if ( mlt_producer_init( producer, self ) == 0 )
		{
			self->parent = producer;

			// Get the properties
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

			// Set the resource property (required for all producers)
			mlt_properties_set( properties, "resource", file );

			// Register transport implementation with the producer
			producer->close = (mlt_destructor) producer_close;

			// Register our get_frame implementation
			producer->get_frame = producer_get_frame;

			// Force the duration to be computed unless explicitly provided.
			mlt_properties_set_position( properties, "length", 0 );
			mlt_properties_set_position( properties, "out", 0 );

			if ( strcmp( service, "avformat-novalidate" ) )
			{
				// Open the file
				mlt_properties_from_utf8( properties, "resource", "_resource" );
				if ( producer_open( self, profile, mlt_properties_get( properties, "_resource" ), 1, 1 ) != 0 )
				{
					// Clean up
					mlt_producer_close( producer );
					producer = NULL;
					producer_avformat_close( self );
				}
				else if ( self->seekable )
				{
					// Close the file to release resources for large playlists - reopen later as needed
					if ( self->audio_format )
						avformat_close_input( &self->audio_format );
					if ( self->video_format )
						avformat_close_input( &self->video_format );
					self->audio_format = NULL;
					self->video_format = NULL;
				}
			}
			if ( producer )
			{
				// Default the user-selectable indices from the auto-detected indices
				mlt_properties_set_int( properties, "audio_index",  self->audio_index );
				mlt_properties_set_int( properties, "video_index",  self->video_index );
#ifdef VDPAU
				mlt_service_cache_set_size( MLT_PRODUCER_SERVICE(producer), "producer_avformat", 5 );
#endif
				mlt_service_cache_put( MLT_PRODUCER_SERVICE(producer), "producer_avformat", self, 0, (mlt_destructor) producer_avformat_close );

				mlt_properties_set_int( properties, "mute_on_pause",  1 );
			}
		}
	}
	return producer;
}

int list_components( char* file )
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
			if ( codec->decode && codec->type == AVMEDIA_TYPE_AUDIO )
				fprintf( stderr, "  - %s\n", codec->name );
		fprintf( stderr, "...\n" );
		skip = 1;
	}
	if ( file && strstr( file, "vcodec-list" ) )
	{
		fprintf( stderr, "---\nvideo_codecs:\n" );
		AVCodec *codec = NULL;
		while ( ( codec = av_codec_next( codec ) ) )
			if ( codec->decode && codec->type == AVMEDIA_TYPE_VIDEO )
				fprintf( stderr, "  - %s\n", codec->name );
		fprintf( stderr, "...\n" );
		skip = 1;
	}

	return skip;
}

static int first_video_index( producer_avformat self )
{
	AVFormatContext *context = self->video_format? self->video_format : self->audio_format;
	int i = -1; // not found

	if ( context ) {
		for ( i = 0; i < context->nb_streams; i++ ) {
			if ( context->streams[i]->codec &&
				 context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
				break;
		}
		if ( i == context->nb_streams )
			i = -1;
	}
	return i;
}

#if (LIBAVFORMAT_VERSION_INT >= ((55<<16)+(39<<8)+100))
#include <libavutil/display.h>

static double get_rotation(AVStream *st)
{
	AVDictionaryEntry *rotate_tag = av_dict_get( st->metadata, "rotate", NULL, 0 );
	uint8_t* displaymatrix = av_stream_get_side_data( st, AV_PKT_DATA_DISPLAYMATRIX, NULL);
	double theta = 0;

	if ( rotate_tag && *rotate_tag->value && strcmp( rotate_tag->value, "0" ) )
	{
		char *tail;
		theta = strtod( rotate_tag->value, &tail );
		if ( *tail )
			theta = 0;
	}
	if ( displaymatrix && !theta )
		theta = -av_display_rotation_get( (int32_t*) displaymatrix );

	theta -= 360 * floor( theta/360 + 0.9/360 );

	return theta;
}
#else
static double get_rotation(AVStream *st)
{
	return 0.0;
}
#endif

static char* filter_restricted( const char *in )
{
	if ( !in ) return NULL;
	size_t n = strlen( in );
	char *out = calloc( 1, n + 1 );
	char *p = out;
	mbstate_t mbs;
	memset( &mbs, 0, sizeof(mbs) );
	while ( *in )
	{
		wchar_t w;
		size_t c = mbrtowc( &w, in, n, &mbs );
		if ( c <= 0 || c > n ) break;
		n -= c;
		in += c;
		if ( w == 0x9 || w == 0xA || w == 0xD ||
				( w >= 0x20 && w <= 0xD7FF ) ||
				( w >= 0xE000 && w <= 0xFFFD ) ||
				( w >= 0x10000 && w <= 0x10FFFF ) )
		{
			mbstate_t ps;
			memset( &ps, 0, sizeof(ps) );
			c = wcrtomb( p, w, &ps );
			if ( c > 0 )
				p += c;
		}
	}
	return out;
}

/** Find the default streams.
*/

static mlt_properties find_default_streams( producer_avformat self )
{
	int i;
	char key[200];
	AVDictionaryEntry *tag = NULL;
	AVFormatContext *context = self->video_format;
	mlt_properties meta_media = MLT_PRODUCER_PROPERTIES( self->parent );

	// Default to the first audio and video streams found
	self->audio_index = -1;
	self->video_index = -1;

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
			case AVMEDIA_TYPE_VIDEO:
				// Use first video stream
				if ( self->video_index < 0 )
					self->video_index = i;
				mlt_properties_set( meta_media, key, "video" );
				snprintf( key, sizeof(key), "meta.media.%d.stream.frame_rate", i );
				double ffmpeg_fps = av_q2d( context->streams[ i ]->avg_frame_rate );
#if LIBAVFORMAT_VERSION_MAJOR < 55
				if ( isnan( ffmpeg_fps ) || ffmpeg_fps == 0 )
					ffmpeg_fps = av_q2d( context->streams[ i ]->r_frame_rate );
#endif
				mlt_properties_set_double( meta_media, key, ffmpeg_fps );

				snprintf( key, sizeof(key), "meta.media.%d.stream.sample_aspect_ratio", i );
				mlt_properties_set_double( meta_media, key, av_q2d( context->streams[ i ]->sample_aspect_ratio ) );
				snprintf( key, sizeof(key), "meta.media.%d.codec.width", i );
				mlt_properties_set_int( meta_media, key, codec_context->width );
				snprintf( key, sizeof(key), "meta.media.%d.codec.height", i );
				mlt_properties_set_int( meta_media, key, codec_context->height );
#if (LIBAVFORMAT_VERSION_INT >= ((55<<16)+(39<<8)+100))
				snprintf( key, sizeof(key), "meta.media.%d.codec.rotate", i );
				mlt_properties_set_int( meta_media, key, get_rotation(context->streams[i]) );
#endif
				snprintf( key, sizeof(key), "meta.media.%d.codec.frame_rate", i );
				AVRational frame_rate = { codec_context->time_base.den, codec_context->time_base.num * codec_context->ticks_per_frame };
				mlt_properties_set_double( meta_media, key, av_q2d( frame_rate ) );
				snprintf( key, sizeof(key), "meta.media.%d.codec.pix_fmt", i );
				mlt_properties_set( meta_media, key, av_get_pix_fmt_name( codec_context->pix_fmt ) );
				snprintf( key, sizeof(key), "meta.media.%d.codec.sample_aspect_ratio", i );
				mlt_properties_set_double( meta_media, key, av_q2d( codec_context->sample_aspect_ratio ) );
				snprintf( key, sizeof(key), "meta.media.%d.codec.colorspace", i );
				switch ( codec_context->colorspace )
				{
				case AVCOL_SPC_SMPTE240M:
					mlt_properties_set_int( meta_media, key, 240 );
					break;
				case AVCOL_SPC_BT470BG:
				case AVCOL_SPC_SMPTE170M:
					mlt_properties_set_int( meta_media, key, 601 );
					break;
				case AVCOL_SPC_BT709:
					mlt_properties_set_int( meta_media, key, 709 );
					break;
				default:
					// This is a heuristic Charles Poynton suggests in "Digital Video and HDTV"
					mlt_properties_set_int( meta_media, key, codec_context->width * codec_context->height > 750000 ? 709 : 601 );
					break;
				}
				if ( codec_context->color_trc && codec_context->color_trc != 2 )
				{
					snprintf( key, sizeof(key), "meta.media.%d.codec.color_trc", i );
					mlt_properties_set_double( meta_media, key, codec_context->color_trc );
				}
				break;
			case AVMEDIA_TYPE_AUDIO:
				if ( !codec_context->channels )
					break;
				// Use first audio stream
				if ( self->audio_index < 0 && pick_audio_format( codec_context->sample_fmt ) != mlt_audio_none )
					self->audio_index = i;

				mlt_properties_set( meta_media, key, "audio" );
				snprintf( key, sizeof(key), "meta.media.%d.codec.sample_fmt", i );
				mlt_properties_set( meta_media, key, av_get_sample_fmt_name( codec_context->sample_fmt ) );
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
		snprintf( key, sizeof(key), "meta.media.%d.codec.long_name", i );
		mlt_properties_set( meta_media, key, codec->long_name );
		snprintf( key, sizeof(key), "meta.media.%d.codec.bit_rate", i );
		mlt_properties_set_int( meta_media, key, codec_context->bit_rate );
// 		snprintf( key, sizeof(key), "meta.media.%d.codec.time_base", i );
// 		mlt_properties_set_double( meta_media, key, av_q2d( codec_context->time_base ) );
//		snprintf( key, sizeof(key), "meta.media.%d.codec.profile", i );
//		mlt_properties_set_int( meta_media, key, codec_context->profile );
//		snprintf( key, sizeof(key), "meta.media.%d.codec.level", i );
//		mlt_properties_set_int( meta_media, key, codec_context->level );

		// Read Metadata
		while ( ( tag = av_dict_get( stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX ) ) )
		{
			if ( tag->value && strcmp( tag->value, "" ) && strcmp( tag->value, "und" ) )
			{
				snprintf( key, sizeof(key), "meta.attr.%d.stream.%s.markup", i, tag->key );
				char* value = filter_restricted( tag->value );
				mlt_properties_set( meta_media, key, value );
				free( value );
			}
		}
	}
	while ( ( tag = av_dict_get( context->metadata, "", tag, AV_DICT_IGNORE_SUFFIX ) ) )
	{
		if ( tag->value && strcmp( tag->value, "" ) && strcmp( tag->value, "und" ) )
		{
			snprintf( key, sizeof(key), "meta.attr.%s.markup", tag->key );
			char* value = filter_restricted( tag->value );
			mlt_properties_set( meta_media, key, value );
			free( value );
		}
	}

	return meta_media;
}

static void get_aspect_ratio( mlt_properties properties, AVStream *stream, AVCodecContext *codec_context )
{
	AVRational sar = stream->sample_aspect_ratio;
	if ( sar.num <= 0 || sar.den <= 0 )
		sar = codec_context->sample_aspect_ratio;
	if ( sar.num <= 0 || sar.den <= 0 )
		sar.num = sar.den = 1;
	mlt_properties_set_int( properties, "meta.media.sample_aspect_num", sar.num );
	mlt_properties_set_int( properties, "meta.media.sample_aspect_den", sar.den );
	mlt_properties_set_double( properties, "aspect_ratio", av_q2d( sar ) );
}

static char* parse_url( mlt_profile profile, const char* URL, AVInputFormat **format, AVDictionary **params )
{
	if ( !URL ) return NULL;

	char *result = NULL;
	char *protocol = strdup( URL );
	char *url = strchr( protocol, ':' );

	// Only if there is not a protocol specification that avformat can handle
	if ( url && avio_check( URL, 0 ) < 0 )
	{
		// Truncate protocol string
		url[0] = 0;
		mlt_log_debug( NULL, "%s: protocol=%s resource=%s\n", __FUNCTION__, protocol, url + 1 );

		// Lookup the format
		*format = av_find_input_format( protocol );

		// Eat the format designator
		result = ++url;

		if ( *format )
		{
			// support for legacy width and height parameters
			char *width = NULL;
			char *height = NULL;

			// Parse out params
			url = strchr( url, '?' );
			while ( url )
			{
				url[0] = 0;
				char *name = strdup( ++url );
				char *value = strchr( name, '=' );
				if ( !value )
					// Also accept : as delimiter for backwards compatibility.
					value = strchr( name, ':' );
				if ( value )
				{
					value[0] = 0;
					value++;
					char *t = strchr( value, '&' );
					if ( t )
						t[0] = 0;
					// translate old parameters to new av_dict names
					if ( !strcmp( name, "frame_rate" ) )
						av_dict_set( params, "framerate", value, 0 );
					else if ( !strcmp( name, "pix_fmt" ) )
						av_dict_set( params, "pixel_format", value, 0 );
					else if ( !strcmp( name, "width" ) )
						width = strdup( value );
					else if ( !strcmp( name, "height" ) )
						height = strdup( value );
					else
						// generic demux/device option support
						av_dict_set( params, name, value, 0 );
				}
				free( name );
				url = strchr( url, '&' );
			}
			// continued support for legacy width and height parameters
			if ( width && height )
			{
				char *s = malloc( strlen( width ) + strlen( height ) + 2 );
				strcpy( s, width );
				strcat( s, "x");
				strcat( s, height );
				av_dict_set( params, "video_size", s, 0 );
				free( s );
			}
			free( width );
			free ( height );
		}
		result = strdup( result );
	}
	else
	{
		result = strdup( URL );
	}
	free( protocol );
	return result;
}

static enum AVPixelFormat pick_pix_fmt( enum AVPixelFormat pix_fmt )
{
	switch ( pix_fmt )
	{
	case AV_PIX_FMT_ARGB:
	case AV_PIX_FMT_RGBA:
	case AV_PIX_FMT_ABGR:
	case AV_PIX_FMT_BGRA:
		return AV_PIX_FMT_RGBA;
#if defined(FFUDIV) && (LIBSWSCALE_VERSION_INT >= ((2<<16)+(5<<8)+102))
	case AV_PIX_FMT_BAYER_RGGB16LE:
		return AV_PIX_FMT_RGB24;
#endif
	default:
		return AV_PIX_FMT_YUV422P;
	}
}

static int get_basic_info( producer_avformat self, mlt_profile profile, const char *filename )
{
	int error = 0;

	// Get the properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( self->parent );

	AVFormatContext *format = self->video_format;

	// Get the duration
	if ( mlt_properties_get_position( properties, "length" ) <= 0 ||
		 mlt_properties_get_position( properties, "out" ) <= 0 )
	{
		if ( format->duration != AV_NOPTS_VALUE )
		{
			// This isn't going to be accurate for all formats
			// We will treat everything with the producer fps.
			mlt_position frames = ( mlt_position ) lrint( format->duration * mlt_profile_fps( profile ) / AV_TIME_BASE );
			if ( mlt_properties_get_position( properties, "out" ) <= 0 )
				mlt_properties_set_position( properties, "out", frames - 1 );
			if ( mlt_properties_get_position( properties, "length" ) <= 0 )
				mlt_properties_set_position( properties, "length", frames );
		}
		else
		{
			// Set live sources to run forever
			if ( mlt_properties_get_position( properties, "length" ) <= 0 )
				mlt_properties_set_position( properties, "length", INT_MAX );
			if ( mlt_properties_get_position( properties, "out" ) <= 0 )
				mlt_properties_set_position( properties, "out", INT_MAX - 1 );
			mlt_properties_set( properties, "eof", "loop" );
		}
	}

	// Check if we're seekable
	// avdevices are typically AVFMT_NOFILE and not seekable
	self->seekable = !format->iformat || !( format->iformat->flags & AVFMT_NOFILE );
	if ( format->pb )
	{
		// protocols can indicate if they support seeking
		self->seekable = format->pb->seekable;
	}
	if ( self->seekable )
	{
		// Do a more rigourous test of seekable on a disposable context
		self->seekable = av_seek_frame( format, -1, format->start_time, AVSEEK_FLAG_BACKWARD ) >= 0;
		mlt_properties_set_int( properties, "seekable", self->seekable );
		self->dummy_context = format;
		self->video_format = NULL;
		avformat_open_input( &self->video_format, filename, NULL, NULL );
		avformat_find_stream_info( self->video_format, NULL );
		format = self->video_format;
	}

	// Fetch the width, height and aspect ratio
	if ( self->video_index != -1 )
	{
		AVCodecContext *codec_context = format->streams[ self->video_index ]->codec;
		mlt_properties_set_int( properties, "width", codec_context->width );
		mlt_properties_set_int( properties, "height", codec_context->height );
		get_aspect_ratio( properties, format->streams[ self->video_index ], codec_context );

		int pix_fmt = codec_context->pix_fmt;
		pick_av_pixel_format( &pix_fmt );
		if ( pix_fmt != AV_PIX_FMT_NONE ) {
			// Verify that we can convert this to one of our image formats.
			struct SwsContext *context = sws_getContext( codec_context->width, codec_context->height, pix_fmt,
				codec_context->width, codec_context->height, pick_pix_fmt( codec_context->pix_fmt ), SWS_BILINEAR, NULL, NULL, NULL);
			if ( context )
				sws_freeContext( context );
			else
				error = 1;
		} else {
			self->video_index = -1;
		}
	}
	return error;
}

#ifdef AVFILTER
static int setup_video_filters( producer_avformat self )
{
	mlt_properties properties = MLT_PRODUCER_PROPERTIES(self->parent);
	AVFormatContext *format = self->video_format;
	AVStream* stream = format->streams[ self->video_index ];
	AVCodecContext *codec_context = stream->codec;

	self->vfilter_graph = avfilter_graph_alloc();

	// From ffplay.c:configure_video_filters().
	char buffersrc_args[256];
	snprintf(buffersrc_args, sizeof(buffersrc_args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d:frame_rate=%d/%d",
		codec_context->width, codec_context->height, codec_context->pix_fmt,
		stream->time_base.num, stream->time_base.den,
		mlt_properties_get_int(properties, "meta.media.sample_aspect_num"),
		FFMAX(mlt_properties_get_int(properties, "meta.media.sample_aspect_den"), 1),
		stream->avg_frame_rate.num, FFMAX(stream->avg_frame_rate.den, 1));

	int result = avfilter_graph_create_filter(&self->vfilter_in, avfilter_get_by_name("buffer"),
		"mlt_buffer", buffersrc_args, NULL, self->vfilter_graph);

	if (result >= 0) {
		result = avfilter_graph_create_filter(&self->vfilter_out, avfilter_get_by_name("buffersink"),
			"mlt_buffersink", NULL, NULL, self->vfilter_graph);

		if (result >= 0) {
			enum AVPixelFormat pix_fmts[] = { codec_context->pix_fmt, AV_PIX_FMT_NONE };
			result = av_opt_set_int_list(self->vfilter_out, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
		}
	}

	return result;
}

static int insert_filter(AVFilterGraph *graph, AVFilterContext **last_filter, const char *name, const char *args)
{
	AVFilterContext *filt_ctx;
	int result = avfilter_graph_create_filter(&filt_ctx, avfilter_get_by_name(name),
		name, args, NULL, graph);
	if (result >= 0) {
		result = avfilter_link(filt_ctx, 0, *last_filter, 0);
		if (result >= 0)
			*last_filter = filt_ctx;
	}
	return result;
}
#endif

/** Open the file.
*/

static int producer_open(producer_avformat self, mlt_profile profile, const char *URL, int take_lock, int test_open )
{
	// Return an error code (0 == no error)
	int error = 0;
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( self->parent );

	if ( !self->is_mutex_init )
	{
		pthread_mutex_init( &self->audio_mutex, NULL );
		pthread_mutex_init( &self->video_mutex, NULL );
		pthread_mutex_init( &self->packets_mutex, NULL );
		pthread_mutex_init( &self->open_mutex, NULL );
		self->is_mutex_init = 1;
	}

	// Lock the service
	if ( take_lock )
	{
		pthread_mutex_lock( &self->audio_mutex );
		pthread_mutex_lock( &self->video_mutex );
	}
	mlt_events_block( properties, self->parent );

	// Parse URL
	AVInputFormat *format = NULL;
	AVDictionary *params = NULL;
	char *filename = parse_url( profile, URL, &format, &params );

	// Now attempt to open the file or device with filename
	error = avformat_open_input( &self->video_format, filename, format, &params ) < 0;
	if ( error )
		// If the URL is a network stream URL, then we probably need to open with full URL
		error = avformat_open_input( &self->video_format, URL, format, &params ) < 0;

	// Set MLT properties onto video AVFormatContext
	if ( !error && self->video_format )
	{
		apply_properties( self->video_format, properties, AV_OPT_FLAG_DECODING_PARAM );
		if ( self->video_format->iformat && self->video_format->iformat->priv_class && self->video_format->priv_data )
			apply_properties( self->video_format->priv_data, properties, AV_OPT_FLAG_DECODING_PARAM );
	}

	av_dict_free( &params );

	// If successful, then try to get additional info
	if ( !error && self->video_format )
	{
		// Get the stream info
		error = avformat_find_stream_info( self->video_format, NULL ) < 0;

		// Continue if no error
		if ( !error && self->video_format )
		{
			// Find default audio and video streams
			find_default_streams( self );
			error = get_basic_info( self, profile, filename );

			// Initialize position info
			self->first_pts = AV_NOPTS_VALUE;
			self->last_position = POSITION_INITIAL;

			if ( !self->audio_format )
			{
				// We're going to cheat here - for seekable A/V files, we will have separate contexts
				// to support independent seeking of audio from video.
				// TODO: Is this really necessary?
				if ( self->audio_index != -1 && self->video_index != -1 )
				{
					if ( self->seekable )
					{
						// And open again for our audio context
						avformat_open_input( &self->audio_format, filename, NULL, NULL );
						apply_properties( self->audio_format, properties, AV_OPT_FLAG_DECODING_PARAM );
						if ( self->audio_format->iformat && self->audio_format->iformat->priv_class && self->audio_format->priv_data )
							apply_properties( self->audio_format->priv_data, properties, AV_OPT_FLAG_DECODING_PARAM );
						avformat_find_stream_info( self->audio_format, NULL );
					}
					else
					{
						self->audio_format = self->video_format;
					}
				}
				else if ( self->audio_index != -1 )
				{
					// We only have an audio context
					self->audio_format = self->video_format;
					self->video_format = NULL;
				}
				else if ( self->video_index == -1 )
				{
					// Something has gone wrong
					error = -1;
				}
				if ( self->audio_format && !self->audio_streams )
					get_audio_streams_info( self );

#ifdef AVFILTER
				// Setup autorotate filters.
				if (self->video_index != -1) {
					self->autorotate = !mlt_properties_get(properties, "autorotate") || mlt_properties_get_int(properties, "autorotate");
					if (!test_open && self->autorotate && !self->vfilter_graph) {
						double theta  = get_rotation(self->video_format->streams[self->video_index]);

						if (fabs(theta - 90) < 1.0) {
							error = ( setup_video_filters(self) < 0 );
							AVFilterContext *last_filter = self->vfilter_out;
							if (!error) error = ( insert_filter(self->vfilter_graph, &last_filter, "transpose", "clock") < 0 );
							if (!error) error = ( avfilter_link(self->vfilter_in, 0, last_filter, 0) < 0 );
							if (!error) error = ( avfilter_graph_config(self->vfilter_graph, NULL) < 0 );
						} else if (fabs(theta - 180) < 1.0) {
							error = ( setup_video_filters(self) < 0 );
							AVFilterContext *last_filter = self->vfilter_out;
							if (!error) error = ( insert_filter(self->vfilter_graph, &last_filter, "hflip", NULL) < 0 );
							if (!error) error = ( insert_filter(self->vfilter_graph, &last_filter, "vflip", NULL) < 0 );
							if (!error) error = ( avfilter_link(self->vfilter_in, 0, last_filter, 0) < 0 );
							if (!error) error = ( avfilter_graph_config(self->vfilter_graph, NULL) < 0 );
						} else if (fabs(theta - 270) < 1.0) {
							error = ( setup_video_filters(self) < 0 );
							AVFilterContext *last_filter = self->vfilter_out;
							if (!error) error = ( insert_filter(self->vfilter_graph, &last_filter, "transpose", "cclock") < 0 );
							if (!error) error = ( avfilter_link(self->vfilter_in, 0, last_filter, 0) < 0 );
							if (!error) error = ( avfilter_graph_config(self->vfilter_graph, NULL) < 0 );
						}
					}
				}
#endif
			}
		}
	}
	free( filename );
	if ( !error )
	{
		self->apackets = mlt_deque_init();
		self->vpackets = mlt_deque_init();
	}

	if ( self->dummy_context )
	{
		pthread_mutex_lock( &self->open_mutex );
		avformat_close_input( &self->dummy_context );
		self->dummy_context = NULL;
		pthread_mutex_unlock( &self->open_mutex );
	}

	// Unlock the service
	if ( take_lock )
	{
		pthread_mutex_unlock( &self->audio_mutex );
		pthread_mutex_unlock( &self->video_mutex );
	}
	mlt_events_unblock( properties, self->parent );

	return error;
}

static void prepare_reopen( producer_avformat self )
{
	mlt_service_lock( MLT_PRODUCER_SERVICE( self->parent ) );
	pthread_mutex_lock( &self->audio_mutex );
	pthread_mutex_lock( &self->open_mutex );

	int i;
	for ( i = 0; i < MAX_AUDIO_STREAMS; i++ )
	{
		mlt_pool_release( self->audio_buffer[i] );
		self->audio_buffer[i] = NULL;
		av_free( self->decode_buffer[i] );
		self->decode_buffer[i] = NULL;
		if ( self->audio_codec[i] )
			avcodec_close( self->audio_codec[i] );
		self->audio_codec[i] = NULL;
	}
	if ( self->video_codec )
		avcodec_close( self->video_codec );
	self->video_codec = NULL;

	if ( self->seekable && self->audio_format )
		avformat_close_input( &self->audio_format );
	if ( self->video_format )
		avformat_close_input( &self->video_format );
	self->audio_format = NULL;
	self->video_format = NULL;
#ifdef AVFILTER
	avfilter_graph_free( &self->vfilter_graph );
#endif
	pthread_mutex_unlock( &self->open_mutex );

	// Cleanup the packet queues
	AVPacket *pkt;
	if ( self->apackets )
	{
		while ( ( pkt = mlt_deque_pop_back( self->apackets ) ) )
		{
			av_free_packet( pkt );
			free( pkt );
		}
		mlt_deque_close( self->apackets );
		self->apackets = NULL;
	}
	if ( self->vpackets )
	{
		while ( ( pkt = mlt_deque_pop_back( self->vpackets ) ) )
		{
			av_free_packet( pkt );
			free( pkt );
		}
		mlt_deque_close( self->vpackets );
		self->vpackets = NULL;
	}
	pthread_mutex_unlock( &self->audio_mutex );
	mlt_service_unlock( MLT_PRODUCER_SERVICE( self->parent ) );
}

static int64_t best_pts( producer_avformat self, int64_t pts, int64_t dts )
{
	self->invalid_pts_counter += pts == AV_NOPTS_VALUE;
	self->invalid_dts_counter += dts == AV_NOPTS_VALUE;
	if ( ( self->invalid_pts_counter <= self->invalid_dts_counter
		   || dts == AV_NOPTS_VALUE ) && pts != AV_NOPTS_VALUE )
		return pts;
	else
		return dts;
}

static void find_first_pts( producer_avformat self, int video_index )
{
	// find initial PTS
	AVFormatContext *context = self->video_format? self->video_format : self->audio_format;
	int ret = 0;
	int toscan = 500;
	AVPacket pkt;

	av_init_packet( &pkt );
	while ( ret >= 0 && toscan-- > 0 )
	{
		ret = av_read_frame( context, &pkt );
		if ( ret >= 0 && pkt.stream_index == video_index && ( pkt.flags & AV_PKT_FLAG_KEY ) )
		{
			mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent),
				"first_pts %"PRId64" dts %"PRId64" pts_dts_delta %d\n",
				pkt.pts, pkt.dts, (int)(pkt.pts - pkt.dts) );
			if ( pkt.dts != AV_NOPTS_VALUE && pkt.dts < 0 )
				// Decoding Time Stamps with negative values are reported by ffmpeg code for
				// (at least) MP4 files containing h.264 video using b-frames.
				// For reasons not understood yet, the first PTS computed then is that of the
				// third frame, causing MLT to display the third frame as if it was the first.
				// This if-clause is meant to catch and work around this issue - if there is
				// a valid, but negative DTS value, we just guess that the first valid
				// Presentation Time Stamp is == 0.
				self->first_pts = 0;
			else
				self->first_pts = best_pts( self, pkt.pts, pkt.dts );
			if ( self->first_pts != AV_NOPTS_VALUE )
				toscan = 0;
		}
		av_free_packet( &pkt );
	}
	av_seek_frame( context, -1, 0, AVSEEK_FLAG_BACKWARD );
}

static int seek_video( producer_avformat self, mlt_position position,
	int64_t req_position, int preseek )
{
	mlt_producer producer = self->parent;
        mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
	int paused = 0;
	int seek_threshold = mlt_properties_get_int( properties, "seek_threshold" );
	if ( seek_threshold <= 0 ) seek_threshold = 12;

	pthread_mutex_lock( &self->packets_mutex );

	if ( self->seekable && ( position != self->video_expected || self->last_position < 0 ) )
	{

		// Fetch the video format context
		AVFormatContext *context = self->video_format;

		// Get the video stream
		AVStream *stream = context->streams[ self->video_index ];

		// Get codec context
		AVCodecContext *codec_context = stream->codec;

		// We may want to use the source fps if available
		double source_fps = mlt_properties_get_double( properties, "meta.media.frame_rate_num" ) /
			mlt_properties_get_double( properties, "meta.media.frame_rate_den" );

		if ( self->last_position == POSITION_INITIAL )
			find_first_pts( self, self->video_index );

		if ( self->video_frame && position + 1 == self->video_expected )
		{
			// We're paused - use last image
			paused = 1;
		}
		else if ( self->seekable && ( position < self->video_expected || position - self->video_expected >= seek_threshold || self->last_position < 0 ) )
		{
			// Calculate the timestamp for the requested frame
			int64_t timestamp = req_position / ( av_q2d( self->video_time_base ) * source_fps );
			if ( req_position <= 0 )
				timestamp = 0;
			else if ( self->first_pts != AV_NOPTS_VALUE )
				timestamp += self->first_pts;
			else if ( context->start_time != AV_NOPTS_VALUE )
				timestamp += context->start_time;
			if ( preseek && av_q2d( self->video_time_base ) != 0 )
				timestamp -= 2 / av_q2d( self->video_time_base );
			if ( timestamp < 0 )
				timestamp = 0;
			mlt_log_debug( MLT_PRODUCER_SERVICE(producer), "seeking timestamp %"PRId64" position " MLT_POSITION_FMT " expected "MLT_POSITION_FMT" last_pos %"PRId64"\n",
				timestamp, position, self->video_expected, self->last_position );

			// Seek to the timestamp
			codec_context->skip_loop_filter = AVDISCARD_NONREF;
			av_seek_frame( context, self->video_index, timestamp, AVSEEK_FLAG_BACKWARD );

			// flush any pictures still in decode buffer
			avcodec_flush_buffers( codec_context );

			// Remove the cached info relating to the previous position
			self->current_position = POSITION_INVALID;
			self->last_position = POSITION_INVALID;
			av_freep( &self->video_frame );
		}
	}
	pthread_mutex_unlock( &self->packets_mutex );
	return paused;
}

/** Convert a frame position to a time code.
*/

static double producer_time_of_frame( mlt_producer producer, mlt_position position )
{
	return ( double )position / mlt_producer_get_fps( producer );
}

// Collect information about all audio streams

static void get_audio_streams_info( producer_avformat self )
{
	// Fetch the audio format context
	AVFormatContext *context = self->audio_format;
	int i;

	for ( i = 0;
		  i < context->nb_streams;
		  i++ )
	{
		if ( context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
		{
			AVCodecContext *codec_context = context->streams[i]->codec;
			AVCodec *codec = avcodec_find_decoder( codec_context->codec_id );

			// If we don't have a codec and we can't initialise it, we can't do much more...
			pthread_mutex_lock( &self->open_mutex );
			if ( codec && avcodec_open2( codec_context, codec, NULL ) >= 0 )
			{
				self->audio_streams++;
				self->audio_max_stream = i;
				self->total_channels += codec_context->channels;
				if ( codec_context->channels > self->max_channel )
					self->max_channel = codec_context->channels;
				if ( codec_context->sample_rate > self->max_frequency )
					self->max_frequency = codec_context->sample_rate;
				avcodec_close( codec_context );
			}
			pthread_mutex_unlock( &self->open_mutex );
		}
	}
	mlt_log_verbose( NULL, "[producer avformat] audio: total_streams %d max_stream %d total_channels %d max_channels %d\n",
		self->audio_streams, self->audio_max_stream, self->total_channels, self->max_channel );
}

static int set_luma_transfer( struct SwsContext *context, int src_colorspace,
	int dst_colorspace, int src_full_range, int dst_full_range )
{
	const int *src_coefficients = sws_getCoefficients( SWS_CS_DEFAULT );
	const int *dst_coefficients = sws_getCoefficients( SWS_CS_DEFAULT );
	int brightness = 0;
	int contrast = 1 << 16;
	int saturation = 1  << 16;
	int src_range = src_full_range ? 1 : 0;
	int dst_range = dst_full_range ? 1 : 0;

	switch ( src_colorspace )
	{
	case 170:
	case 470:
	case 601:
	case 624:
		src_coefficients = sws_getCoefficients( SWS_CS_ITU601 );
		break;
	case 240:
		src_coefficients = sws_getCoefficients( SWS_CS_SMPTE240M );
		break;
	case 709:
		src_coefficients = sws_getCoefficients( SWS_CS_ITU709 );
		break;
	default:
		break;
	}
	switch ( dst_colorspace )
	{
	case 170:
	case 470:
	case 601:
	case 624:
		dst_coefficients = sws_getCoefficients( SWS_CS_ITU601 );
		break;
	case 240:
		dst_coefficients = sws_getCoefficients( SWS_CS_SMPTE240M );
		break;
	case 709:
		dst_coefficients = sws_getCoefficients( SWS_CS_ITU709 );
		break;
	default:
		break;
	}
	return sws_setColorspaceDetails( context, src_coefficients, src_range, dst_coefficients, dst_range,
		brightness, contrast, saturation );
}

static mlt_image_format pick_image_format( enum AVPixelFormat pix_fmt )
{
	switch ( pix_fmt )
	{
	case AV_PIX_FMT_ARGB:
	case AV_PIX_FMT_RGBA:
	case AV_PIX_FMT_ABGR:
	case AV_PIX_FMT_BGRA:
		return mlt_image_rgb24a;
	case AV_PIX_FMT_YUV420P:
	case AV_PIX_FMT_YUVJ420P:
	case AV_PIX_FMT_YUVA420P:
		return mlt_image_yuv420p;
	case AV_PIX_FMT_RGB24:
	case AV_PIX_FMT_BGR24:
	case AV_PIX_FMT_GRAY8:
	case AV_PIX_FMT_MONOWHITE:
	case AV_PIX_FMT_MONOBLACK:
	case AV_PIX_FMT_RGB8:
	case AV_PIX_FMT_BGR8:
#if defined(FFUDIV) && (LIBSWSCALE_VERSION_INT >= ((2<<16)+(5<<8)+102))
	case AV_PIX_FMT_BAYER_RGGB16LE:
		return mlt_image_rgb24;
#endif
	default:
		return mlt_image_yuv422;
	}
}

static mlt_audio_format pick_audio_format( int sample_fmt )
{
	switch ( sample_fmt )
	{
	// interleaved
	case AV_SAMPLE_FMT_U8:
		return mlt_audio_u8;
	case AV_SAMPLE_FMT_S16:
		return mlt_audio_s16;
	case AV_SAMPLE_FMT_S32:
		return mlt_audio_s32le;
	case AV_SAMPLE_FMT_FLT:
		return mlt_audio_f32le;
	// planar - this producer converts planar to interleaved
	case AV_SAMPLE_FMT_U8P:
		return mlt_audio_u8;
	case AV_SAMPLE_FMT_S16P:
		return mlt_audio_s16;
	case AV_SAMPLE_FMT_S32P:
		return mlt_audio_s32le;
	case AV_SAMPLE_FMT_FLTP:
		return mlt_audio_f32le;
	default:
		return mlt_audio_none;
	}
}

/**
 * Handle deprecated pixel format (JPEG range in YUV420P for exemple).
 *
 * Replace pix_fmt with the official pixel format to use.
 * @return 0 if no pix_fmt replacement, 1 otherwise
 */
static int pick_av_pixel_format( int *pix_fmt )
{
#if defined(FFUDIV) && (LIBAVFORMAT_VERSION_INT >= ((55<<16)+(48<<8)+100))
	switch (*pix_fmt)
	{
	case AV_PIX_FMT_YUVJ420P:
		*pix_fmt = AV_PIX_FMT_YUV420P;
		return 1;
	case AV_PIX_FMT_YUVJ411P:
		*pix_fmt = AV_PIX_FMT_YUV411P;
		return 1;
	case AV_PIX_FMT_YUVJ422P:
		*pix_fmt = AV_PIX_FMT_YUV422P;
		return 1;
	case AV_PIX_FMT_YUVJ444P:
		*pix_fmt = AV_PIX_FMT_YUV444P;
		return 1;
	case AV_PIX_FMT_YUVJ440P:
		*pix_fmt = AV_PIX_FMT_YUV440P;
		return 1;
	}
#endif
	return 0;
}

#if LIBSWSCALE_VERSION_INT >= AV_VERSION_INT( 3, 1, 101 )
struct sliced_pix_fmt_conv_t
{
	int width, height, slice_w;
	AVFrame *frame;
	AVPicture *output;
	enum AVPixelFormat src_format, dst_format;
	const AVPixFmtDescriptor *src_desc, *dst_desc;
	int flags, src_colorspace, dst_colorspace, src_full_range, dst_full_range;
};

static int sliced_h_pix_fmt_conv_proc( int id, int idx, int jobs, void* cookie )
{
	uint8_t *out[4];
	const uint8_t *in[4];
	int in_stride[4], out_stride[4];
	int src_v_chr_pos = -513, dst_v_chr_pos = -513, ret, i, slice_x, slice_w, w, h, mul, field, slices, interlaced = 0;

	struct SwsContext *sws;
	struct sliced_pix_fmt_conv_t* ctx = ( struct sliced_pix_fmt_conv_t* )cookie;

	interlaced = ctx->frame->interlaced_frame;
	field = ( interlaced ) ? ( idx & 1 ) : 0;
	idx = ( interlaced ) ? ( idx / 2 ) : idx;
	slices = ( interlaced ) ? ( jobs / 2 ) : jobs;
	mul = ( interlaced ) ? 2 : 1;
	h = ctx->height >> !!interlaced;
	slice_w = ctx->slice_w;
	slice_x = slice_w * idx;
	slice_w = FFMIN( slice_w, ctx->width - slice_x );

	if ( AV_PIX_FMT_YUV420P == ctx->src_format )
		src_v_chr_pos = ( !interlaced ) ? 128 : ( !field ) ? 64 : 192;

	if ( AV_PIX_FMT_YUV420P == ctx->dst_format )
		dst_v_chr_pos = ( !interlaced ) ? 128 : ( !field ) ? 64 : 192;

	mlt_log_debug( NULL, "%s:%d: [id=%d, idx=%d, jobs=%d], interlaced=%d, field=%d, slices=%d, mul=%d, h=%d, slice_w=%d, slice_x=%d ctx->src_desc=[log2_chroma_h=%d, log2_chroma_w=%d], src_v_chr_pos=%d, dst_v_chr_pos=%d\n",
		__FUNCTION__, __LINE__, id, idx, jobs, interlaced, field, slices, mul, h, slice_w, slice_x, ctx->src_desc->log2_chroma_h, ctx->src_desc->log2_chroma_w, src_v_chr_pos, dst_v_chr_pos );

	if ( slice_w <= 0 )
		return 0;

	sws = sws_alloc_context();

	av_opt_set_int( sws, "srcw", slice_w, 0 );
	av_opt_set_int( sws, "srch", h, 0 );
	av_opt_set_int( sws, "src_format", ctx->src_format, 0 );
	av_opt_set_int( sws, "dstw", slice_w, 0 );
	av_opt_set_int( sws, "dsth", h, 0 );
	av_opt_set_int( sws, "dst_format", ctx->dst_format, 0 );
	av_opt_set_int( sws, "sws_flags", ctx->flags | SWS_FULL_CHR_H_INP, 0 );

	av_opt_set_int( sws, "src_h_chr_pos", -513, 0 );
	av_opt_set_int( sws, "src_v_chr_pos", src_v_chr_pos, 0 );
	av_opt_set_int( sws, "dst_h_chr_pos", -513, 0 );
	av_opt_set_int( sws, "dst_v_chr_pos", dst_v_chr_pos, 0 );

	if ( ( ret = sws_init_context( sws, NULL, NULL ) ) < 0 )
	{
		mlt_log_error( NULL, "%s:%d: sws_init_context failed, ret=%d\n", __FUNCTION__, __LINE__, ret );
		sws_freeContext( sws );
		return 0;
	}

	set_luma_transfer( sws, ctx->src_colorspace, ctx->dst_colorspace, ctx->src_full_range, ctx->dst_full_range );

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(55, 0, 100)
#define PIX_DESC_BPP(DESC) (DESC.step_minus1 + 1)
#else
#define PIX_DESC_BPP(DESC) (DESC.step)
#endif

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 32, 100)
#define AV_PIX_FMT_FLAG_PLANAR PIX_FMT_PLANAR
#endif
	for( i = 0; i < 4; i++ )
	{
		int in_offset = (AV_PIX_FMT_FLAG_PLANAR & ctx->src_desc->flags)
			? ( ( 1 == i || 2 == i ) ? ( slice_x >> ctx->src_desc->log2_chroma_w ) : slice_x )
			: ( ( 0 == i ) ? slice_x : 0 );

		int out_offset = (AV_PIX_FMT_FLAG_PLANAR & ctx->dst_desc->flags)
			? ( ( 1 == i || 2 == i ) ? ( slice_x >> ctx->dst_desc->log2_chroma_w ) : slice_x )
			: ( ( 0 == i ) ? slice_x : 0 );

		in_offset *= PIX_DESC_BPP(ctx->src_desc->comp[i]);
		out_offset *= PIX_DESC_BPP(ctx->dst_desc->comp[i]);

		in_stride[i]  = ctx->frame->linesize[i] * mul;
		out_stride[i] = ctx->output->linesize[i] * mul;

		in[i] =  ctx->frame->data[i] + ctx->frame->linesize[i] * field + in_offset;
		out[i] = ctx->output->data[i] + ctx->output->linesize[i] * field + out_offset;
	}

	sws_scale( sws, in, in_stride, 0, h, out, out_stride );

	sws_freeContext( sws );

	return 0;
}
#endif

// returns resulting YUV colorspace
static int convert_image( producer_avformat self, AVFrame *frame, uint8_t *buffer, int pix_fmt,
	mlt_image_format *format, int width, int height, uint8_t **alpha )
{
	int flags = SWS_BICUBIC | SWS_ACCURATE_RND;
	mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( self->parent ) );
	int result = self->yuv_colorspace;

	mlt_log_timings_begin();

	mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent), "%s @ %dx%d space %d->%d\n",
		mlt_image_format_name( *format ),
		width, height, self->yuv_colorspace, profile->colorspace );

	// extract alpha from planar formats
	if ( ( pix_fmt == AV_PIX_FMT_YUVA420P
#if defined(FFUDIV)
			|| pix_fmt == AV_PIX_FMT_YUVA444P
#endif
			) &&
		*format != mlt_image_rgb24a && *format != mlt_image_opengl &&
		frame->data[3] && frame->linesize[3] )
	{
		int i;
		uint8_t *src, *dst;

		dst = *alpha = mlt_pool_alloc( width * height );
		src = frame->data[3];

		for ( i = 0; i < height; dst += width, src += frame->linesize[3], i++ )
			memcpy( dst, src, FFMIN( width, frame->linesize[3] ) );
	}

	int src_pix_fmt = pix_fmt;
	pick_av_pixel_format( &src_pix_fmt );
	if ( *format == mlt_image_yuv420p )
	{
		// This is a special case. Movit wants the full range, if available.
		// Thankfully, there is not much other use of yuv420p except consumer
		// avformat with no filters and explicitly requested.
#if defined(FFUDIV) && (LIBAVFORMAT_VERSION_INT >= ((55<<16)+(48<<8)+100))
		struct SwsContext *context = sws_getContext(width, height, src_pix_fmt,
			width, height, AV_PIX_FMT_YUV420P, flags, NULL, NULL, NULL);
#else
		struct SwsContext *context = sws_getContext( width, height, pix_fmt,
					width, height, self->full_luma ? AV_PIX_FMT_YUVJ420P : AV_PIX_FMT_YUV420P,
					flags, NULL, NULL, NULL);
#endif

		AVPicture output;
		output.data[0] = buffer;
		output.data[1] = buffer + width * height;
		output.data[2] = buffer + ( 5 * width * height ) / 4;
		output.linesize[0] = width;
		output.linesize[1] = width >> 1;
		output.linesize[2] = width >> 1;
		if ( !set_luma_transfer( context, self->yuv_colorspace, profile->colorspace, self->full_luma, self->full_luma ) )
			result = profile->colorspace;
		sws_scale( context, (const uint8_t* const*) frame->data, frame->linesize, 0, height,
			output.data, output.linesize);
		sws_freeContext( context );
	}
	else if ( *format == mlt_image_rgb24 )
	{
		struct SwsContext *context = sws_getContext( width, height, src_pix_fmt,
			width, height, AV_PIX_FMT_RGB24, flags | SWS_FULL_CHR_H_INT, NULL, NULL, NULL);
		AVPicture output;
		avpicture_fill( &output, buffer, AV_PIX_FMT_RGB24, width, height );
		// libswscale wants the RGB colorspace to be SWS_CS_DEFAULT, which is = SWS_CS_ITU601.
		set_luma_transfer( context, self->yuv_colorspace, 601, self->full_luma, 0 );
		sws_scale( context, (const uint8_t* const*) frame->data, frame->linesize, 0, height,
			output.data, output.linesize);
		sws_freeContext( context );
	}
	else if ( *format == mlt_image_rgb24a || *format == mlt_image_opengl )
	{
		struct SwsContext *context = sws_getContext( width, height, src_pix_fmt,
			width, height, AV_PIX_FMT_RGBA, flags | SWS_FULL_CHR_H_INT, NULL, NULL, NULL);
		AVPicture output;
		avpicture_fill( &output, buffer, AV_PIX_FMT_RGBA, width, height );
		// libswscale wants the RGB colorspace to be SWS_CS_DEFAULT, which is = SWS_CS_ITU601.
		set_luma_transfer( context, self->yuv_colorspace, 601, self->full_luma, 0 );
		sws_scale( context, (const uint8_t* const*) frame->data, frame->linesize, 0, height,
			output.data, output.linesize);
		sws_freeContext( context );
	}
	else
#if LIBSWSCALE_VERSION_INT >= AV_VERSION_INT( 3, 1, 101 )
	{
		int i, c;
		AVPicture output;
		struct sliced_pix_fmt_conv_t ctx =
		{
			.flags = flags,
			.width = width,
			.height = height,
			.frame = frame,
			.output = &output,
			.dst_format = AV_PIX_FMT_YUYV422,
			.src_colorspace = self->yuv_colorspace,
			.dst_colorspace = profile->colorspace,
			.src_full_range = self->full_luma,
			.dst_full_range = 0,
		};
#if defined(FFUDIV) && (LIBAVFORMAT_VERSION_INT >= ((55<<16)+(48<<8)+100))
		ctx.src_format = src_pix_fmt;
#else
		ctx.src_format = pix_fmt;
#endif
		ctx.src_desc = av_pix_fmt_desc_get( ctx.src_format );
		ctx.dst_desc = av_pix_fmt_desc_get( ctx.dst_format );

		avpicture_fill( ctx.output, buffer, ctx.dst_format, width, height );

		if ( !getenv("MLT_AVFORMAT_SLICED_PIXFMT_DISABLE") )
			ctx.slice_w = ( width < 1000 )
				? ( 256 >> frame->interlaced_frame )
				: ( 512 >> frame->interlaced_frame );
		else
			ctx.slice_w = width;

		c = ( width + ctx.slice_w - 1 ) / ctx.slice_w;
		c *= frame->interlaced_frame ? 2 : 1;

		if ( !getenv("MLT_AVFORMAT_SLICED_PIXFMT_DISABLE") )
			mlt_slices_run_normal( c, sliced_h_pix_fmt_conv_proc, &ctx );
		else
			for ( i = 0 ; i < c; i++ )
				sliced_h_pix_fmt_conv_proc( i, i, c, &ctx );

		result = profile->colorspace;
	}
#else
	{
#if defined(FFUDIV) && (LIBAVFORMAT_VERSION_INT >= ((55<<16)+(48<<8)+100))
		struct SwsContext *context = sws_getContext( width, height, src_pix_fmt,
			width, height, AV_PIX_FMT_YUYV422, flags | SWS_FULL_CHR_H_INP, NULL, NULL, NULL);
#else
		struct SwsContext *context = sws_getContext( width, height, pix_fmt,
			width, height, AV_PIX_FMT_YUYV422, flags | SWS_FULL_CHR_H_INP, NULL, NULL, NULL);
#endif
		AVPicture output;
		avpicture_fill( &output, buffer, AV_PIX_FMT_YUYV422, width, height );
		if ( !set_luma_transfer( context, self->yuv_colorspace, profile->colorspace, self->full_luma, 0 ) )
			result = profile->colorspace;
		sws_scale( context, (const uint8_t* const*) frame->data, frame->linesize, 0, height,
			output.data, output.linesize);
		sws_freeContext( context );
	}
#endif
	mlt_log_timings_end( NULL, __FUNCTION__ );

	return result;
}

static void set_image_size( producer_avformat self, int *width, int *height )
{
	double dar = mlt_profile_dar( mlt_service_profile( MLT_PRODUCER_SERVICE(self->parent) ) );
	double theta  = self->autorotate? get_rotation( self->video_format->streams[self->video_index] ) : 0.0;
	if ( fabs(theta - 90.0) < 1.0 || fabs(theta - 270.0) < 1.0 )
	{
		*height = self->video_codec->width;
		// Workaround 1088 encodings missing cropping info.
		if ( self->video_codec->height == 1088 && dar == 16.0/9.0 )
			*width = 1080;
		else
			*width = self->video_codec->height;
	} else {
		*width = self->video_codec->width;
		// Workaround 1088 encodings missing cropping info.
		if ( self->video_codec->height == 1088 && dar == 16.0/9.0 )
			*height = 1080;
		else
			*height = self->video_codec->height;
	}
}

/** Allocate the image buffer and set it on the frame.
*/

static int allocate_buffer( mlt_frame frame, AVCodecContext *codec_context, uint8_t **buffer, mlt_image_format format, int width, int height )
{
	int size = 0;

	if ( codec_context->width == 0 || codec_context->height == 0 )
		return size;

	size = mlt_image_format_size( format, width, height, NULL );
	*buffer = mlt_pool_alloc( size );
	if ( *buffer )
		mlt_frame_set_image( frame, *buffer, size, mlt_pool_release );
	else
		size = 0;

	return size;
}

/** Get an image from a frame.
*/

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the producer
	producer_avformat self = mlt_frame_pop_service( frame );
	mlt_producer producer = self->parent;

	// Get the properties from the frame
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the frame number of this frame
	mlt_position position = mlt_frame_original_position( frame );

	// Get the producer properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

	pthread_mutex_lock( &self->video_mutex );

	uint8_t *alpha = NULL;
	int got_picture = 0;
	int image_size = 0;

	// Fetch the video format context
	AVFormatContext *context = self->video_format;
	if ( !context )
		goto exit_get_image;

	// Get the video stream
	AVStream *stream = context->streams[ self->video_index ];

	// Get codec context
	AVCodecContext *codec_context = stream->codec;

	mlt_log_timings_begin();

	// Get the image cache
	if ( ! self->image_cache )
	{
		// if cache size supplied by environment variable
		int cache_supplied = getenv( "MLT_AVFORMAT_CACHE" ) != NULL;
		int cache_size = cache_supplied? atoi( getenv( "MLT_AVFORMAT_CACHE" ) ) : 0;

		// cache size supplied via property
		if ( mlt_properties_get( properties, "cache" ) )
		{
			cache_supplied = 1;
			cache_size = mlt_properties_get_int( properties, "cache" );
		}
		if ( mlt_properties_get_int( properties, "noimagecache" ) )
			cache_size = 0;
		// create cache if not disabled
		if ( !cache_supplied || cache_size > 0 )
			self->image_cache = mlt_cache_init();
		// set cache size if supplied
		if ( self->image_cache && cache_supplied )
			mlt_cache_set_size( self->image_cache, cache_size );
	}
	if ( self->image_cache )
	{
		mlt_frame original = mlt_cache_get_frame( self->image_cache, position );
		if ( original )
		{
			mlt_properties orig_props = MLT_FRAME_PROPERTIES( original );
			int size = 0;

			*buffer = mlt_properties_get_data( orig_props, "alpha", &size );
			if (*buffer)
				mlt_frame_set_alpha( frame, *buffer, size, NULL );
			*buffer = mlt_properties_get_data( orig_props, "image", &size );
			mlt_frame_set_image( frame, *buffer, size, NULL );
			mlt_properties_set_data( frame_properties, "avformat.image_cache", original, 0, (mlt_destructor) mlt_frame_close, NULL );
			*format = mlt_properties_get_int( orig_props, "format" );
			set_image_size( self, width, height );
			got_picture = 1;
			goto exit_get_image;
		}
	}
	// Cache miss

	// We may want to use the source fps if available
	double source_fps = mlt_properties_get_double( properties, "meta.media.frame_rate_num" ) /
		mlt_properties_get_double( properties, "meta.media.frame_rate_den" );

	// This is the physical frame position in the source
	int64_t req_position = ( int64_t )( position / mlt_producer_get_fps( producer ) * source_fps + 0.5 );

	// Determines if we have to decode all frames in a sequence - when there temporal compression is used.
	const AVCodecDescriptor *descriptor = codec_context->codec? avcodec_descriptor_get( codec_context->codec->id ) : NULL;
	int must_decode = descriptor && !( descriptor->props & AV_CODEC_PROP_INTRA_ONLY );

	double delay = mlt_properties_get_double( properties, "video_delay" );

	// Seek if necessary
	int preseek = must_decode && codec_context->has_b_frames;
#if defined(FFUDIV)
	const char *interp = mlt_properties_get( frame_properties, "rescale.interp" );
	preseek = preseek && interp && strcmp( interp, "nearest" );
#endif
	int paused = seek_video( self, position, req_position, preseek );

	// Seek might have reopened the file
	context = self->video_format;
	stream = context->streams[ self->video_index ];
	codec_context = stream->codec;
	if ( *format == mlt_image_none || *format == mlt_image_glsl ||
			codec_context->pix_fmt == AV_PIX_FMT_ARGB ||
			codec_context->pix_fmt == AV_PIX_FMT_RGBA ||
			codec_context->pix_fmt == AV_PIX_FMT_ABGR ||
			codec_context->pix_fmt == AV_PIX_FMT_BGRA )
		*format = pick_image_format( codec_context->pix_fmt );
#if defined(FFUDIV) && (LIBSWSCALE_VERSION_INT >= ((2<<16)+(5<<8)+102))
	else if ( codec_context->pix_fmt == AV_PIX_FMT_BAYER_RGGB16LE ) {
		if ( *format == mlt_image_yuv422 )
			*format = mlt_image_yuv420p;
		else if ( *format == mlt_image_rgb24a )
			*format = mlt_image_rgb24;
	}
#endif

	// Duplicate the last image if necessary
	if ( self->video_frame && self->video_frame->linesize[0]
		 && ( paused || self->current_position >= req_position ) )
	{
		// Duplicate it
		set_image_size( self, width, height );
		if ( ( image_size = allocate_buffer( frame, codec_context, buffer, *format, *width, *height ) ) )
		{
			int yuv_colorspace;
#ifdef VDPAU
			if ( self->vdpau && self->vdpau->buffer )
			{
				AVPicture picture;
				picture.data[0] = self->vdpau->buffer;
				picture.data[2] = self->vdpau->buffer + codec_context->width * codec_context->height;
				picture.data[1] = self->vdpau->buffer + codec_context->width * codec_context->height * 5 / 4;
				picture.linesize[0] = codec_context->width;
				picture.linesize[1] = codec_context->width / 2;
				picture.linesize[2] = codec_context->width / 2;
				yuv_colorspace = convert_image( self, (AVFrame*) &picture, *buffer,
					AV_PIX_FMT_YUV420P, format, *width, *height, &alpha );
			}
			else
#endif
			yuv_colorspace = convert_image( self, self->video_frame, *buffer, codec_context->pix_fmt,
				format, *width, *height, &alpha );
			mlt_properties_set_int( frame_properties, "colorspace", yuv_colorspace );
			got_picture = 1;
		}
	}
	else
	{
		int ret = 0;
		int64_t int_position = 0;
		int decode_errors = 0;

		// Construct an AVFrame for YUV422 conversion
		if ( !self->video_frame )
#if LIBAVCODEC_VERSION_INT >= ((55<<16)+(45<<8)+0)
			self->video_frame = av_frame_alloc();
#else
			self->video_frame = avcodec_alloc_frame( );
#endif

		while( ret >= 0 && !got_picture )
		{
			// Read a packet
			if ( self->pkt.stream_index == self->video_index )
				av_free_packet( &self->pkt );
			av_init_packet( &self->pkt );
			pthread_mutex_lock( &self->packets_mutex );
			if ( mlt_deque_count( self->vpackets ) )
			{
				AVPacket *tmp = (AVPacket*) mlt_deque_pop_front( self->vpackets );
				self->pkt = *tmp;
				free( tmp );
			}
			else
			{
				ret = av_read_frame( context, &self->pkt );
				if ( ret >= 0 && !self->seekable && self->pkt.stream_index == self->audio_index )
				{
					if ( !av_dup_packet( &self->pkt ) )
					{
						AVPacket *tmp = malloc( sizeof(AVPacket) );
						*tmp = self->pkt;
						mlt_deque_push_back( self->apackets, tmp );
					}
				}
				else if ( ret < 0 )
				{
					if ( ret != AVERROR_EOF )
						mlt_log_verbose( MLT_PRODUCER_SERVICE(producer), "av_read_frame returned error %d inside get_image\n", ret );
					if ( !self->seekable && mlt_properties_get_int( properties, "reconnect" ) )
					{
						// Try to reconnect to live sources by closing context and codecs,
						// and letting next call to get_frame() reopen.
						prepare_reopen( self );
						pthread_mutex_unlock( &self->packets_mutex );
						goto exit_get_image;
					}
					if ( !self->seekable && mlt_properties_get_int( properties, "exit_on_disconnect" ) )
					{
						mlt_log_fatal( MLT_PRODUCER_SERVICE(producer), "Exiting with error due to disconnected source.\n" );
						exit( EXIT_FAILURE );
					}
					// Send null packets to drain decoder.
					self->pkt.size = 0;
					self->pkt.data = NULL;
				}
			}
			pthread_mutex_unlock( &self->packets_mutex );

			// We only deal with video from the selected video_index
			if ( self->pkt.stream_index == self->video_index )
			{
				int64_t pts = best_pts( self, self->pkt.pts, self->pkt.dts );
				if ( pts != AV_NOPTS_VALUE )
				{
					if ( !self->seekable && self->first_pts == AV_NOPTS_VALUE )
						self->first_pts = pts;
					if ( self->first_pts != AV_NOPTS_VALUE )
						pts -= self->first_pts;
					else if ( context->start_time != AV_NOPTS_VALUE )
						pts -= context->start_time;
					int_position = ( int64_t )( ( av_q2d( self->video_time_base ) * pts + delay ) * source_fps + 0.5 );
					if ( int_position == self->last_position )
						int_position = self->last_position + 1;
				}
				mlt_log_debug( MLT_PRODUCER_SERVICE(producer),
					"V pkt.pts %"PRId64" pkt.dts %"PRId64" req_pos %"PRId64" cur_pos %"PRId64" pkt_pos %"PRId64"\n",
					self->pkt.pts, self->pkt.dts, req_position, self->current_position, int_position );

				// Make a dumb assumption on streams that contain wild timestamps
				if ( llabs( req_position - int_position ) > 999 )
				{
					mlt_log_verbose( MLT_PRODUCER_SERVICE(producer), " WILD TIMESTAMP: "
						"pkt.pts=[%"PRId64"], pkt.dts=[%"PRId64"], req_position=[%"PRId64"], "
						"current_position=[%"PRId64"], int_position=[%"PRId64"], pts=[%"PRId64"] \n",
						self->pkt.pts, self->pkt.dts, req_position,
						self->current_position, int_position, pts );
					int_position = req_position;
				}
				self->last_position = int_position;

				// Decode the image
				if ( must_decode  || int_position >= req_position || !self->pkt.data )
				{
#ifdef VDPAU
					if ( self->vdpau )
					{
						if ( self->vdpau->decoder == VDP_INVALID_HANDLE )
						{
							vdpau_decoder_init( self );
						}
						self->vdpau->is_decoded = 0;
					}
#endif
					codec_context->reordered_opaque = int_position;
					if ( int_position >= req_position )
						codec_context->skip_loop_filter = AVDISCARD_NONE;
					ret = avcodec_decode_video2( codec_context, self->video_frame, &got_picture, &self->pkt );
					mlt_log_debug( MLT_PRODUCER_SERVICE(producer), "decoded packet with size %d => %d\n", self->pkt.size, ret );
					// Note: decode may fail at the beginning of MPEGfile (B-frames referencing before first I-frame), so allow a few errors.
					if ( ret < 0 )
					{
						if ( ++decode_errors <= 10 ) {
							ret = 0;
						} else {
							mlt_log_warning( MLT_PRODUCER_SERVICE(producer), "video decoding error %d\n", ret );
							self->last_good_position = POSITION_INVALID;
						}
					}
					else
					{
						decode_errors = 0;
					}
				}

				if ( got_picture )
				{
					// Get position of reordered frame
					int_position = self->video_frame->reordered_opaque;
					pts = best_pts( self, self->video_frame->pkt_pts, self->video_frame->pkt_dts );
					if ( pts != AV_NOPTS_VALUE )
					{
						if ( self->first_pts != AV_NOPTS_VALUE )
							pts -= self->first_pts;
						else if ( context->start_time != AV_NOPTS_VALUE )
							pts -= context->start_time;
						int_position = ( int64_t )( ( av_q2d( self->video_time_base ) * pts + delay ) * source_fps + 0.5 );
					}

					if ( int_position < req_position )
						got_picture = 0;
					else if ( int_position >= req_position )
						codec_context->skip_loop_filter = AVDISCARD_NONE;
				}
				else if ( !self->pkt.data ) // draining decoder with null packets
				{
					ret = -1;
				}
				mlt_log_debug( MLT_PRODUCER_SERVICE(producer), " got_pic %d key %d ret %d pkt_pos %"PRId64"\n",
							   got_picture, self->pkt.flags & AV_PKT_FLAG_KEY, ret, int_position );
			}

			// Now handle the picture if we have one
			if ( got_picture )
			{
#ifdef AVFILTER
				if (self->autorotate && self->vfilter_graph) {
					ret = av_buffersrc_add_frame(self->vfilter_in, self->video_frame);
					if (ret < 0) {
						got_picture = 0;
						break;
					}
					while (ret >= 0) {
						ret = av_buffersink_get_frame_flags(self->vfilter_out, self->video_frame, 0);
						if (ret < 0) {
							ret = 0;
							break;
						}
					}
				}
#endif
				set_image_size( self, width, height );
				if ( ( image_size = allocate_buffer( frame, codec_context, buffer, *format, *width, *height ) ) )
				{
					int yuv_colorspace;
#ifdef VDPAU
					if ( self->vdpau )
					{
						if ( self->vdpau->is_decoded )
						{
							struct vdpau_render_state *render = (struct vdpau_render_state*) self->video_frame->data[0];
							void *planes[3];
							uint32_t pitches[3];
							VdpYCbCrFormat dest_format = VDP_YCBCR_FORMAT_YV12;
							
							if ( !self->vdpau->buffer )
								self->vdpau->buffer = mlt_pool_alloc( codec_context->width * codec_context->height * 3 / 2 );
							self->video_frame->data[0] = planes[0] = self->vdpau->buffer;
							self->video_frame->data[2] = planes[1] = self->vdpau->buffer + codec_context->width * codec_context->height;
							self->video_frame->data[1] = planes[2] = self->vdpau->buffer + codec_context->width * codec_context->height * 5 / 4;
							self->video_frame->linesize[0] = pitches[0] = codec_context->width;
							self->video_frame->linesize[1] = pitches[1] = codec_context->width / 2;
							self->video_frame->linesize[2] = pitches[2] = codec_context->width / 2;

							VdpStatus status = vdp_surface_get_bits( render->surface, dest_format, planes, pitches );
							if ( status == VDP_STATUS_OK )
							{
								yuv_colorspace = convert_image( self, self->video_frame, *buffer, AV_PIX_FMT_YUV420P,
									format, *width, *height, &alpha );
								mlt_properties_set_int( frame_properties, "colorspace", yuv_colorspace );
							}
							else
							{
								mlt_log_error( MLT_PRODUCER_SERVICE(producer), "VDPAU Error: %s\n", vdp_get_error_string( status ) );
								image_size = self->vdpau->is_decoded = 0;
							}
						}
						else
						{
							mlt_log_error( MLT_PRODUCER_SERVICE(producer), "VDPAU error in VdpDecoderRender\n" );
							image_size = got_picture = 0;
							self->last_good_position = POSITION_INVALID;
						}
					}
					else
#endif
					yuv_colorspace = convert_image( self, self->video_frame, *buffer, codec_context->pix_fmt,
						format, *width, *height, &alpha );
					mlt_properties_set_int( frame_properties, "colorspace", yuv_colorspace );
					self->top_field_first |= self->video_frame->top_field_first;
					self->current_position = int_position;
				}
				else
				{
					got_picture = 0;
				}
			}

			// Free packet data if not video and not live audio packet
			if ( self->pkt.stream_index != self->video_index &&
				 !( !self->seekable && self->pkt.stream_index == self->audio_index ) )
				av_free_packet( &self->pkt );
		}
	}

	// set alpha
	if ( alpha )
		mlt_frame_set_alpha( frame, alpha, (*width) * (*height), mlt_pool_release );

	if ( image_size > 0 )
	{
		mlt_properties_set_int( frame_properties, "format", *format );
		// Cache the image for rapid repeated access.
		if ( self->image_cache ) {
			mlt_cache_put_frame( self->image_cache, frame );
		}
		// Clone frame for error concealment.
		if ( self->current_position >= self->last_good_position ) {
			self->last_good_position = self->current_position;
			if ( self->last_good_frame )
				mlt_frame_close( self->last_good_frame );
			self->last_good_frame = mlt_frame_clone( frame, 1 );
		}
	}
	else if ( self->last_good_frame )
	{
		// Use last known good frame if there was a decoding failure.
		mlt_frame original = mlt_frame_clone( self->last_good_frame, 1 );
		mlt_properties orig_props = MLT_FRAME_PROPERTIES( original );
		int size = 0;

		*buffer = mlt_properties_get_data( orig_props, "alpha", &size );
		if (*buffer)
			mlt_frame_set_alpha( frame, *buffer, size, NULL );
		*buffer = mlt_properties_get_data( orig_props, "image", &size );
		mlt_frame_set_image( frame, *buffer, size, NULL );
		mlt_properties_set_data( frame_properties, "avformat.conceal_error", original, 0, (mlt_destructor) mlt_frame_close, NULL );
		*format = mlt_properties_get_int( orig_props, "format" );
		set_image_size( self, width, height );
		got_picture = 1;
	}

	// Regardless of speed, we expect to get the next frame (cos we ain't too bright)
	self->video_expected = position + 1;

exit_get_image:

	pthread_mutex_unlock( &self->video_mutex );

	// Set the progressive flag
	if ( mlt_properties_get( properties, "force_progressive" ) )
		mlt_properties_set_int( frame_properties, "progressive", !!mlt_properties_get_int( properties, "force_progressive" ) );
	else if ( self->video_frame )
		mlt_properties_set_int( frame_properties, "progressive", !self->video_frame->interlaced_frame );

	// Set the field order property for this frame
	if ( mlt_properties_get( properties, "force_tff" ) )
		mlt_properties_set_int( frame_properties, "top_field_first", !!mlt_properties_get_int( properties, "force_tff" ) );
	else
		mlt_properties_set_int( frame_properties, "top_field_first", self->top_field_first );

	// Set immutable properties of the selected track's (or overridden) source attributes.
	mlt_service_lock( MLT_PRODUCER_SERVICE( producer ) );
	mlt_properties_set_int( properties, "meta.media.top_field_first", self->top_field_first );
	mlt_properties_set_int( properties, "meta.media.progressive", mlt_properties_get_int( frame_properties, "progressive" ) );
	mlt_service_unlock( MLT_PRODUCER_SERVICE( producer ) );

	mlt_log_timings_end( NULL, __FUNCTION__ );

	return !got_picture;
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
		const AVOption *opt = av_opt_find( obj, opt_name, NULL, flags, flags );
		if ( opt_name && mlt_properties_get( properties, opt_name ) )
		{
			if ( opt )
				av_opt_set( obj, opt_name, mlt_properties_get( properties, opt_name), 0 );
		}
	}
}

/** Initialize the video codec context.
 */

static int video_codec_init( producer_avformat self, int index, mlt_properties properties )
{
	// Initialise the codec if necessary
	if ( !self->video_codec )
	{
		// Get the video stream
		AVStream *stream = self->video_format->streams[ index ];

		// Get codec context
		AVCodecContext *codec_context = stream->codec;

		// Find the codec
		AVCodec *codec = avcodec_find_decoder( codec_context->codec_id );
#ifdef VDPAU
		if ( codec_context->codec_id == AV_CODEC_ID_H264 )
		{
			if ( ( codec = avcodec_find_decoder_by_name( "h264_vdpau" ) ) )
			{
				if ( vdpau_init( self ) )
				{
					self->video_codec = codec_context;
					if ( !vdpau_decoder_init( self ) )
						vdpau_fini( self );
				}
			}
			if ( !self->vdpau )
				codec = avcodec_find_decoder( codec_context->codec_id );
		}
#endif

		// Initialise multi-threading
		int thread_count = mlt_properties_get_int( properties, "threads" );
		if ( thread_count == 0 && getenv( "MLT_AVFORMAT_THREADS" ) )
			thread_count = atoi( getenv( "MLT_AVFORMAT_THREADS" ) );
		if ( thread_count > 1 )
			codec_context->thread_count = thread_count;

		// If we don't have a codec and we can't initialise it, we can't do much more...
		pthread_mutex_lock( &self->open_mutex );
		if ( codec && avcodec_open2( codec_context, codec, NULL ) >= 0 )
		{
			// Now store the codec with its destructor
			self->video_codec = codec_context;
		}
		else
		{
			// Remember that we can't use this later
			self->video_index = -1;
			pthread_mutex_unlock( &self->open_mutex );
			return 0;
		}
		pthread_mutex_unlock( &self->open_mutex );

		// Process properties as AVOptions
		apply_properties( codec_context, properties, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM );
		if ( codec->priv_class && codec_context->priv_data )
			apply_properties( codec_context->priv_data, properties, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM );

		// Reset some image properties
		mlt_properties_set_int( properties, "width", self->video_codec->width );
		mlt_properties_set_int( properties, "height", self->video_codec->height );
		get_aspect_ratio( properties, stream, self->video_codec );

		// Start with the muxer frame rate.
		AVRational frame_rate = stream->avg_frame_rate;
		double fps = av_q2d( frame_rate );

#if LIBAVFORMAT_VERSION_MAJOR < 55 || defined(FFUDIV)
		// Verify and sanitize the muxer frame rate.
		if ( isnan( fps ) || isinf( fps ) || fps == 0 )
		{
			frame_rate = stream->r_frame_rate;
			fps = av_q2d( frame_rate );
		}
#endif
#if (LIBAVFORMAT_VERSION_MAJOR < 55) || defined(FFUDIV)
		// With my samples when r_frame_rate != 1000 but avg_frame_rate is valid,
		// avg_frame_rate gives some approximate value that does not well match the media.
		// Also, on my sample where r_frame_rate = 1000, using avg_frame_rate directly
		// results in some very choppy output, but some value slightly different works
		// great.
		if ( av_q2d( stream->r_frame_rate ) >= 1000 && av_q2d( stream->avg_frame_rate ) > 0 )
		{
			frame_rate = av_d2q( av_q2d( stream->avg_frame_rate ), 1024 );
			fps = av_q2d( frame_rate );
		}
#endif
		// XXX frame rates less than 1 fps are not considered sane
		if ( isnan( fps ) || isinf( fps ) || fps < 1.0 )
		{
			// Get the frame rate from the codec.
			frame_rate.num = self->video_codec->time_base.den;
			frame_rate.den = self->video_codec->time_base.num * self->video_codec->ticks_per_frame;
			fps = av_q2d( frame_rate );
		}
		if ( isnan( fps ) || isinf( fps ) || fps < 1.0 )
		{
			// Use the profile frame rate if all else fails.
			mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( self->parent ) );
			frame_rate.num = profile->frame_rate_num;
			frame_rate.den = profile->frame_rate_den;
		}

		self->video_time_base = stream->time_base;
		if ( mlt_properties_get( properties, "force_fps" ) )
		{
			AVRational force_fps = av_d2q( mlt_properties_get_double( properties, "force_fps" ), 1024 );
			self->video_time_base = av_mul_q( stream->time_base, av_div_q( frame_rate, force_fps ) );
			frame_rate = force_fps;
		}
		mlt_properties_set_int( properties, "meta.media.frame_rate_num", frame_rate.num );
		mlt_properties_set_int( properties, "meta.media.frame_rate_den", frame_rate.den );

		// Set the YUV colorspace from override or detect
		self->yuv_colorspace = mlt_properties_get_int( properties, "force_colorspace" );
		if ( ! self->yuv_colorspace )
		{
			switch ( self->video_codec->colorspace )
			{
			case AVCOL_SPC_SMPTE240M:
				self->yuv_colorspace = 240;
				break;
			case AVCOL_SPC_BT470BG:
			case AVCOL_SPC_SMPTE170M:
				self->yuv_colorspace = 601;
				break;
			case AVCOL_SPC_BT709:
				self->yuv_colorspace = 709;
				break;
			default:
				// This is a heuristic Charles Poynton suggests in "Digital Video and HDTV"
				self->yuv_colorspace = self->video_codec->width * self->video_codec->height > 750000 ? 709 : 601;
				break;
			}
		}
		// Let apps get chosen colorspace
		mlt_properties_set_int( properties, "meta.media.colorspace", self->yuv_colorspace );

		// Get the color transfer characteristic (gamma).
		self->color_trc = mlt_properties_get_int( properties, "force_color_trc" );
		if ( !self->color_trc )
			self->color_trc = self->video_codec->color_trc;
		mlt_properties_set_int( properties, "meta.media.color_trc", self->color_trc );

		// Get the RGB color primaries.
		switch ( self->video_codec->color_primaries )
		{
		case AVCOL_PRI_BT470BG:
			self->color_primaries = 601625;
			break;
		case AVCOL_PRI_SMPTE170M:
		case AVCOL_PRI_SMPTE240M:
			self->color_primaries = 601525;
			break;
		case AVCOL_PRI_BT709:
		case AVCOL_PRI_UNSPECIFIED:
		default:
			self->color_primaries = 709;
			break;
		}

		self->full_luma = 0;
		mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent), "color_range %d\n", codec_context->color_range );
		if ( codec_context->color_range == AVCOL_RANGE_JPEG )
			self->full_luma = 1;
		if ( mlt_properties_get( properties, "set.force_full_luma" ) )
			self->full_luma = mlt_properties_get_int( properties, "set.force_full_luma" );
	}
	return self->video_index > -1;
}

/** Set up video handling.
*/

static void producer_set_up_video( producer_avformat self, mlt_frame frame )
{
	// Get the producer
	mlt_producer producer = self->parent;

	// Get the properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

	// Fetch the video format context
	AVFormatContext *context = self->video_format;

	// Get the video_index
	int index = mlt_properties_get_int( properties, "video_index" );

	int unlock_needed = 0;

	// Reopen the file if necessary
	if ( !context && index > -1 )
	{
		unlock_needed = 1;
		pthread_mutex_lock( &self->video_mutex );
		mlt_properties_from_utf8( properties, "resource", "_resource" );
		producer_open( self, mlt_service_profile( MLT_PRODUCER_SERVICE(producer) ),
			mlt_properties_get( properties, "_resource" ), 0, 0 );
		context = self->video_format;
	}

	// Exception handling for video_index
	if ( context && index >= (int) context->nb_streams )
	{
		// Get the last video stream
		for ( index = context->nb_streams - 1;
			  index >= 0 && context->streams[ index ]->codec->codec_type != AVMEDIA_TYPE_VIDEO;
			  index-- );
		mlt_properties_set_int( properties, "video_index", index );
	}
	if ( context && index > -1 && context->streams[ index ]->codec->codec_type != AVMEDIA_TYPE_VIDEO )
	{
		// Invalidate the video stream
		index = -1;
		mlt_properties_set_int( properties, "video_index", index );
	}

	// Update the video properties if the index changed
	if ( context && index > -1 && index != self->video_index )
	{
		// Reset the video properties if the index changed
		self->video_index = index;
		pthread_mutex_lock( &self->open_mutex );
		if ( self->video_codec )
			avcodec_close( self->video_codec );
		self->video_codec = NULL;
		pthread_mutex_unlock( &self->open_mutex );
	}

	// Get the frame properties
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Get the codec
	if ( context && index > -1 && video_codec_init( self, index, properties ) )
	{
		// Set the frame properties
		double force_aspect_ratio = mlt_properties_get_double( properties, "force_aspect_ratio" );
		double aspect_ratio = ( force_aspect_ratio > 0.0 ) ?
			force_aspect_ratio : mlt_properties_get_double( properties, "aspect_ratio" );

		// Set the width and height
		double dar = mlt_profile_dar( mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) ) );
		double theta  = self->autorotate? get_rotation( self->video_format->streams[index] ) : 0.0;
		if ( fabs(theta - 90.0) < 1.0 || fabs(theta - 270.0) < 1.0 )
		{
			// Workaround 1088 encodings missing cropping info.
			if ( self->video_codec->height == 1088 && dar == 16.0/9.0 ) {
				mlt_properties_set_int( frame_properties, "width", 1080 );
				mlt_properties_set_int( properties, "meta.media.width", 1080 );
			} else {
				mlt_properties_set_int( frame_properties, "width", self->video_codec->height );
				mlt_properties_set_int( properties, "meta.media.width", self->video_codec->height );
			}
			mlt_properties_set_int( frame_properties, "height", self->video_codec->width );
			mlt_properties_set_int( properties, "meta.media.height", self->video_codec->width );
			aspect_ratio = ( force_aspect_ratio > 0.0 ) ? force_aspect_ratio : 1.0 / aspect_ratio;
			mlt_properties_set_double( frame_properties, "aspect_ratio", 1.0/aspect_ratio );
		} else {
			mlt_properties_set_int( frame_properties, "width", self->video_codec->width );
			mlt_properties_set_int( properties, "meta.media.width", self->video_codec->width );
			// Workaround 1088 encodings missing cropping info.
			if ( self->video_codec->height == 1088 && dar == 16.0/9.0 ) {
				mlt_properties_set_int( frame_properties, "height", 1080 );
				mlt_properties_set_int( properties, "meta.media.height", 1080 );
			} else {
				mlt_properties_set_int( frame_properties, "height", self->video_codec->height );
				mlt_properties_set_int( properties, "meta.media.height", self->video_codec->height );
			}
			mlt_properties_set_double( frame_properties, "aspect_ratio", aspect_ratio );
		}
		mlt_properties_set_int( frame_properties, "colorspace", self->yuv_colorspace );
		mlt_properties_set_int( frame_properties, "color_trc", self->color_trc );
		mlt_properties_set_int( frame_properties, "color_primaries", self->color_primaries );
		mlt_properties_set_int( frame_properties, "full_luma", self->full_luma );

		// Add our image operation
		mlt_frame_push_service( frame, self );
		mlt_frame_push_get_image( frame, producer_get_image );
	}
	else
	{
		// If something failed, use test card image
		mlt_properties_set_int( frame_properties, "test_image", 1 );
	}
	if ( unlock_needed )
		pthread_mutex_unlock( &self->video_mutex );
}

static int seek_audio( producer_avformat self, mlt_position position, double timecode )
{
	int paused = 0;

	pthread_mutex_lock( &self->packets_mutex );

	// Seek if necessary
	if ( self->seekable && ( position != self->audio_expected || self->last_position < 0 ) )
	{
		if ( self->last_position == POSITION_INITIAL )
		{
			int video_index = self->video_index;
			if ( video_index == -1 )
				video_index = first_video_index( self );
			if ( video_index >= 0 )
				find_first_pts( self, video_index );
		}

		if ( position + 1 == self->audio_expected  &&
			mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( self->parent ), "mute_on_pause" ) )
		{
			// We're paused - silence required
			paused = 1;
		}
		else if ( position < self->audio_expected || position - self->audio_expected >= 12 )
		{
			AVFormatContext *context = self->audio_format;
			int64_t timestamp = llrint( timecode * AV_TIME_BASE );
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
				self->audio_used[i - 1] = 0;
		}
	}
	pthread_mutex_unlock( &self->packets_mutex );
	return paused;
}

static int sample_bytes( AVCodecContext *context )
{
	return av_get_bytes_per_sample( context->sample_fmt );
}

static void planar_to_interleaved( uint8_t *dest, AVFrame *src, int samples, int channels, int bytes_per_sample )
{
	int s, c;
	for ( s = 0; s < samples; s++ )
	{
		for ( c = 0; c < channels; c++ )
		{
			if ( c < AV_NUM_DATA_POINTERS )
				memcpy( dest, &src->data[c][s * bytes_per_sample], bytes_per_sample );
			dest += bytes_per_sample;
		}
	}
}

static int decode_audio( producer_avformat self, int *ignore, AVPacket pkt, int samples, double timecode, double fps )
{
	// Fetch the audio_format
	AVFormatContext *context = self->audio_format;

	// Get the current stream index
	int index = pkt.stream_index;

	// Get codec context
	AVCodecContext *codec_context = self->audio_codec[ index ];

	// Obtain the audio buffers
	uint8_t *audio_buffer = self->audio_buffer[ index ];

	int channels = codec_context->channels;
	int audio_used = self->audio_used[ index ];
	int ret = 0;

	while ( pkt.data && pkt.size > 0 )
	{
		int sizeof_sample = sample_bytes( codec_context );
		int got_frame = 0;

		// Decode the audio
		if ( !self->audio_frame )
#if LIBAVCODEC_VERSION_INT >= ((55<<16)+(45<<8)+0)
			self->audio_frame = av_frame_alloc();
		else
			av_frame_unref( self->audio_frame );
#else
			self->audio_frame = avcodec_alloc_frame();
		else
			avcodec_get_frame_defaults( self->audio_frame );
#endif
		ret = avcodec_decode_audio4( codec_context, self->audio_frame, &got_frame, &pkt );
		if ( ret < 0 )
		{
			mlt_log_warning( MLT_PRODUCER_SERVICE(self->parent), "audio decoding error %d\n", ret );
			break;
		}

		// Consume (sometimes partial) data in the packet.
		pkt.size -= ret;
		pkt.data += ret;

		// If decoded successfully
		if ( got_frame )
		{
			// Figure out how many samples will be needed after resampling
			int convert_samples = self->audio_frame->nb_samples;
			channels = codec_context->channels;

			// Resize audio buffer to prevent overflow
			if ( ( audio_used + convert_samples ) * channels * sizeof_sample > self->audio_buffer_size[ index ] )
			{
				self->audio_buffer_size[ index ] = ( audio_used + convert_samples * 2 ) * channels * sizeof_sample;
				audio_buffer = self->audio_buffer[ index ] = mlt_pool_realloc( audio_buffer, self->audio_buffer_size[ index ] );
			}
			uint8_t *dest = &audio_buffer[ audio_used * channels * sizeof_sample ];
			switch ( codec_context->sample_fmt )
			{
			case AV_SAMPLE_FMT_U8P:
			case AV_SAMPLE_FMT_S16P:
			case AV_SAMPLE_FMT_S32P:
			case AV_SAMPLE_FMT_FLTP:
				planar_to_interleaved( dest, self->audio_frame, convert_samples, channels, sizeof_sample );
				break;
			default: {
				int data_size = av_samples_get_buffer_size( NULL, channels,
					self->audio_frame->nb_samples, codec_context->sample_fmt, 1 );
				// Straight copy to audio buffer
				memcpy( dest, self->audio_frame->data[0], data_size );
				}
			}
			audio_used += convert_samples;
		}
		
		// Handle ignore
		if ( *ignore > 0 && audio_used )
		{
			int n = FFMIN( audio_used, *ignore );
			*ignore -= n;
			audio_used -= n;
			memmove( audio_buffer, &audio_buffer[ n * channels * sizeof_sample ],
					 audio_used * channels * sizeof_sample );
		}
	}

	// If we're behind, ignore this packet
	// Skip this on non-seekable, audio-only inputs.
	if ( pkt.pts >= 0 && ( self->seekable || self->video_format ) && *ignore == 0 && audio_used > samples )
	{
		int64_t pts = pkt.pts;
		if ( self->first_pts != AV_NOPTS_VALUE )
			pts -= self->first_pts;
		else if ( context->start_time != AV_NOPTS_VALUE )
			pts -= context->start_time;
		double timebase = av_q2d( context->streams[ index ]->time_base );
		int64_t int_position = llrint( timebase * pts * fps );
		int64_t req_position = llrint( timecode * fps );
		int64_t req_pts =      llrint( timecode / timebase );

		mlt_log_debug( MLT_PRODUCER_SERVICE(self->parent),
			"A pkt.pts %"PRId64" pkt.dts %"PRId64" req_pos %"PRId64" cur_pos %"PRId64" pkt_pos %"PRId64"\n",
			pkt.pts, pkt.dts, req_position, self->current_position, int_position );

		if ( self->seekable || int_position > 0 )
		{
			if ( req_pts > pts )
				// We are behind, so skip some
				*ignore = lrint( timebase * (req_pts - pts) * codec_context->sample_rate );
			else if ( self->audio_index != INT_MAX && int_position > req_position + 2 )
				// We are ahead, so seek backwards some more
				seek_audio( self, req_position, timecode - 1.0 );
		}

		// Cancel the find_first_pts() in seek_audio()
		if ( self->video_index == -1 && self->last_position == POSITION_INITIAL )
			self->last_position = int_position;
	}

	self->audio_used[ index ] = audio_used;

	return ret;
}

/** Get the audio from a frame.
*/
static int producer_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the producer
	producer_avformat self = mlt_frame_pop_audio( frame );

	pthread_mutex_lock( &self->audio_mutex );
	
	// Obtain the frame number of this frame
	mlt_position position = mlt_frame_original_position( frame );

	// Calculate the real time code
	double real_timecode = producer_time_of_frame( self->parent, position );

	// Get the producer fps
	double fps = mlt_producer_get_fps( self->parent );
	if ( mlt_properties_get( MLT_FRAME_PROPERTIES(frame), "producer_consumer_fps" ) )
		fps = mlt_properties_get_double( MLT_FRAME_PROPERTIES(frame), "producer_consumer_fps" );

	// Number of frames to ignore (for ffwd)
	int ignore[ MAX_AUDIO_STREAMS ] = { 0 };

	// Flag for paused (silence)
	double timecode = self->audio_expected > 0 ? real_timecode : FFMAX(real_timecode - 0.25, 0.0);
	int paused = seek_audio( self, position, timecode );

	// Initialize ignore for all streams from the seek return value
	int i = MAX_AUDIO_STREAMS;
	while ( i-- )
		ignore[i] = ignore[0];

	// Fetch the audio_format
	AVFormatContext *context = self->audio_format;
	if ( !context )
		goto exit_get_audio;

	int sizeof_sample = sizeof( int16_t );
	
	// Determine the tracks to use
	int index = self->audio_index;
	int index_max = self->audio_index + 1;
	if ( self->audio_index == INT_MAX )
	{
		index = 0;
		index_max = FFMIN( MAX_AUDIO_STREAMS, context->nb_streams );
		*channels = self->total_channels;
		*samples = mlt_sample_calculator( fps, self->max_frequency, position );
		*frequency = self->max_frequency;
	}

	// Initialize the buffers
	for ( ; index < index_max && index < MAX_AUDIO_STREAMS; index++ )
	{
		// Get codec context
		AVCodecContext *codec_context = self->audio_codec[ index ];

		if ( codec_context && !self->audio_buffer[ index ] )
		{
			if ( self->audio_index != INT_MAX && !mlt_properties_get( MLT_PRODUCER_PROPERTIES(self->parent), "request_channel_layout" ) )
				codec_context->request_channel_layout = av_get_default_channel_layout( *channels );
			sizeof_sample = sample_bytes( codec_context );

			// Check for audio buffer and create if necessary
			self->audio_buffer_size[ index ] = MAX_AUDIO_FRAME_SIZE * sizeof_sample;
			self->audio_buffer[ index ] = mlt_pool_alloc( self->audio_buffer_size[ index ] );

			// Check for decoder buffer and create if necessary
			self->decode_buffer[ index ] = av_malloc( self->audio_buffer_size[ index ] );
		}
	}

	// Get the audio if required
	if ( !paused && *frequency > 0 )
	{
		int ret	= 0;
		int got_audio = 0;
		AVPacket pkt;

		av_init_packet( &pkt );
		
		// Caller requested number samples based on requested sample rate.
		if ( self->audio_index != INT_MAX )
			*samples = mlt_sample_calculator( fps, self->audio_codec[ self->audio_index ]->sample_rate, position );

		while ( ret >= 0 && !got_audio )
		{
			// Check if the buffer already contains the samples required
			if ( self->audio_index != INT_MAX &&
				 self->audio_used[ self->audio_index ] >= *samples &&
				 ignore[ self->audio_index ] == 0 )
			{
				got_audio = 1;
				break;
			}
			else if ( self->audio_index == INT_MAX )
			{
				// Check if there is enough audio for all streams
				got_audio = 1;
				for ( index = 0; got_audio && index < index_max; index++ )
					if ( ( self->audio_codec[ index ] && self->audio_used[ index ] < *samples ) || ignore[ index ] )
						got_audio = 0;
				if ( got_audio )
					break;
			}

			// Read a packet
			pthread_mutex_lock( &self->packets_mutex );
			if ( mlt_deque_count( self->apackets ) )
			{
				AVPacket *tmp = (AVPacket*) mlt_deque_pop_front( self->apackets );
				pkt = *tmp;
				free( tmp );
			}
			else
			{
				ret = av_read_frame( context, &pkt );
				if ( ret >= 0 && !self->seekable && pkt.stream_index == self->video_index )
				{
					if ( !av_dup_packet( &pkt ) )
					{
						AVPacket *tmp = malloc( sizeof(AVPacket) );
						*tmp = pkt;
						mlt_deque_push_back( self->vpackets, tmp );
					}
				}
				else if ( ret < 0 )
				{
					mlt_producer producer = self->parent;
					mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
					if ( ret != AVERROR_EOF )
						mlt_log_verbose( MLT_PRODUCER_SERVICE(producer), "av_read_frame returned error %d inside get_audio\n", ret );
					if ( !self->seekable && mlt_properties_get_int( properties, "reconnect" ) )
					{
						// Try to reconnect to live sources by closing context and codecs,
						// and letting next call to get_frame() reopen.
						prepare_reopen( self );
						pthread_mutex_unlock( &self->packets_mutex );
						goto exit_get_audio;
					}
					if ( !self->seekable && mlt_properties_get_int( properties, "exit_on_disconnect" ) )
					{
						mlt_log_fatal( MLT_PRODUCER_SERVICE(producer), "Exiting with error due to disconnected source.\n" );
						exit( EXIT_FAILURE );
					}
				}
			}
			pthread_mutex_unlock( &self->packets_mutex );

			// We only deal with audio from the selected audio index
			index = pkt.stream_index;
			if ( index < MAX_AUDIO_STREAMS && ret >= 0 && pkt.data && pkt.size > 0 && ( index == self->audio_index ||
				 ( self->audio_index == INT_MAX && context->streams[ index ]->codec->codec_type == AVMEDIA_TYPE_AUDIO ) ) )
			{
				ret = decode_audio( self, &ignore[index], pkt, *samples, real_timecode, fps );
			}

			if ( self->seekable || index != self->video_index )
				av_free_packet( &pkt );

		}

		// Set some additional return values
		*format = mlt_audio_s16;
		if ( self->audio_index != INT_MAX )
		{
			index = self->audio_index;
			*channels = self->audio_codec[ index ]->channels;
			*frequency = self->audio_codec[ index ]->sample_rate;
			*format = pick_audio_format( self->audio_codec[ index ]->sample_fmt );
			sizeof_sample = sample_bytes( self->audio_codec[ index ] );
		}
		else if ( self->audio_index == INT_MAX )
		{
			for ( index = 0; index < index_max; index++ )
				if ( self->audio_codec[ index ] )
				{
					// XXX: This only works if all audio tracks have the same sample format.
					*format = pick_audio_format( self->audio_codec[ index ]->sample_fmt );
					sizeof_sample = sample_bytes( self->audio_codec[ index ] );
					break;
				}
		}

		// Allocate and set the frame's audio buffer
		int size = mlt_audio_format_size( *format, *samples, *channels );
		*buffer = mlt_pool_alloc( size );
		mlt_frame_set_audio( frame, *buffer, *format, size, mlt_pool_release );

		// Interleave tracks if audio_index=all
		if ( self->audio_index == INT_MAX )
		{
			uint8_t *dest = *buffer;
			int i;
			for ( i = 0; i < *samples; i++ )
			{
				for ( index = 0; index < index_max; index++ )
				if ( self->audio_codec[ index ] )
				{
					int current_channels = self->audio_codec[ index ]->channels;
					uint8_t *src = self->audio_buffer[ index ] + i * current_channels * sizeof_sample;
					memcpy( dest, src, current_channels * sizeof_sample );
					dest += current_channels * sizeof_sample;
				}
			}
			for ( index = 0; index < index_max; index++ )
			if ( self->audio_codec[ index ] && self->audio_used[ index ] >= *samples )
			{
				int current_channels = self->audio_codec[ index ]->channels;
				uint8_t *src = self->audio_buffer[ index ] + *samples * current_channels * sizeof_sample;
				self->audio_used[index] -= *samples;
				memmove( self->audio_buffer[ index ], src, self->audio_used[ index ] * current_channels * sizeof_sample );
			}
		}
		// Copy a single track to the output buffer
		else
		{
			index = self->audio_index;

			// Now handle the audio if we have enough
			if ( self->audio_used[ index ] > 0 )
			{
				uint8_t *src = self->audio_buffer[ index ];
				// copy samples from audio_buffer
				size = self->audio_used[ index ] < *samples ? self->audio_used[ index ] : *samples;
				memcpy( *buffer, src, size * *channels * sizeof_sample );
				// supply the remaining requested samples as silence
				if ( *samples > self->audio_used[ index ] )
					memset( *buffer + size * *channels * sizeof_sample, 0, ( *samples - self->audio_used[ index ] ) * *channels * sizeof_sample );
				// reposition the samples within audio_buffer
				self->audio_used[ index ] -= size;
				memmove( src, src + size * *channels * sizeof_sample, self->audio_used[ index ] * *channels * sizeof_sample );
			}
			else
			{
				// Otherwise fill with silence
				memset( *buffer, 0, *samples * *channels * sizeof_sample );
			}
		}
	}
	else
	{
exit_get_audio:
		// Get silence and don't touch the context
		mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	}
	
	// Regardless of speed (other than paused), we expect to get the next frame
	if ( !paused )
		self->audio_expected = position + 1;

	pthread_mutex_unlock( &self->audio_mutex );

	return 0;
}

/** Initialize the audio codec context.
*/

static int audio_codec_init( producer_avformat self, int index, mlt_properties properties )
{
	// Initialise the codec if necessary
	if ( !self->audio_codec[ index ] )
	{
		// Get codec context
		AVCodecContext *codec_context = self->audio_format->streams[index]->codec;

		// Find the codec
		AVCodec *codec = avcodec_find_decoder( codec_context->codec_id );

		// If we don't have a codec and we can't initialise it, we can't do much more...
		pthread_mutex_lock( &self->open_mutex );
		if ( codec && avcodec_open2( codec_context, codec, NULL ) >= 0 )
		{
			// Now store the codec with its destructor
			if ( self->audio_codec[ index ] )
				avcodec_close( self->audio_codec[ index ] );
			self->audio_codec[ index ] = codec_context;
		}
		else
		{
			// Remember that we can't use self later
			self->audio_index = -1;
		}
		pthread_mutex_unlock( &self->open_mutex );

		// Process properties as AVOptions
		apply_properties( codec_context, properties, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_DECODING_PARAM );
		if ( codec && codec->priv_class && codec_context->priv_data )
			apply_properties( codec_context->priv_data, properties, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_DECODING_PARAM );
	}
	return self->audio_codec[ index ] && self->audio_index > -1;
}

/** Set up audio handling.
*/

static void producer_set_up_audio( producer_avformat self, mlt_frame frame )
{
	// Get the producer
	mlt_producer producer = self->parent;

	// Get the properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

	// Fetch the audio format context
	AVFormatContext *context = self->audio_format;

	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Get the audio_index
	int index = mlt_properties_get_int( properties, "audio_index" );

	// Handle all audio tracks
	if ( self->audio_index > -1 &&
	     mlt_properties_get( properties, "audio_index" ) &&
	     !strcmp( mlt_properties_get( properties, "audio_index" ), "all" ) )
		index = INT_MAX;

	// Reopen the file if necessary
	if ( !context && self->audio_index > -1 && index > -1 )
	{
		mlt_properties_from_utf8( properties, "resource", "_resource" );
		producer_open( self, mlt_service_profile( MLT_PRODUCER_SERVICE(producer) ),
			mlt_properties_get( properties, "_resource" ), 1, 0 );
		context = self->audio_format;
	}

	// Exception handling for audio_index
	if ( context && index >= (int) context->nb_streams && index < INT_MAX )
	{
		for ( index = context->nb_streams - 1;
			  index >= 0 && context->streams[ index ]->codec->codec_type != AVMEDIA_TYPE_AUDIO;
			  index-- );
		mlt_properties_set_int( properties, "audio_index", index );
	}
	if ( context && index > -1 && index < INT_MAX &&
		 context->streams[ index ]->codec->codec_type != AVMEDIA_TYPE_AUDIO )
	{
		index = self->audio_index;
		mlt_properties_set_int( properties, "audio_index", index );
	}
	if ( context && index > -1 && index < INT_MAX &&
		 pick_audio_format( context->streams[ index ]->codec->sample_fmt ) == mlt_audio_none )
	{
		index = -1;
	}

	// Update the audio properties if the index changed
	if ( context && index > -1 && self->audio_index > -1 && index != self->audio_index )
	{
		pthread_mutex_lock( &self->open_mutex );
		if ( self->audio_codec[ self->audio_index ] )
			avcodec_close( self->audio_codec[ self->audio_index ] );
		self->audio_codec[ self->audio_index ] = NULL;
		pthread_mutex_unlock( &self->open_mutex );
	}
	if ( self->audio_index != -1 )
		self->audio_index = index;
	else
		index = -1;

	// Get the codec(s)
	if ( context && index == INT_MAX )
	{
		mlt_properties_set_int( frame_properties, "audio_frequency", self->max_frequency );
		mlt_properties_set_int( frame_properties, "audio_channels", self->total_channels );
		for ( index = 0; index < context->nb_streams && index < MAX_AUDIO_STREAMS; index++ )
		{
			if ( context->streams[ index ]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
				audio_codec_init( self, index, properties );
		}
	}
	else if ( context && index > -1 && index < MAX_AUDIO_STREAMS &&
		audio_codec_init( self, index, properties ) )
	{
		mlt_properties_set_int( frame_properties, "audio_frequency", self->audio_codec[ index ]->sample_rate );
		mlt_properties_set_int( frame_properties, "audio_channels", self->audio_codec[ index ]->channels );
	}
	if ( context && index > -1 )
	{
		// Add our audio operation
		mlt_frame_push_audio( frame, self );
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
	producer_avformat self = mlt_cache_item_data( cache_item, NULL );

	// If cache miss
	if ( !self )
	{
		self = calloc( 1, sizeof( struct producer_avformat_s ) );
		producer->child = self;
		self->parent = producer;
		mlt_service_cache_put( service, "producer_avformat", self, 0, (mlt_destructor) producer_avformat_close );
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

	// Set up the video
	producer_set_up_video( self, *frame );

	// Set up the audio
	producer_set_up_audio( self, *frame );

	// Set the position of this producer
	mlt_position position = mlt_producer_frame( producer );
	mlt_properties_set_position( MLT_FRAME_PROPERTIES( *frame ), "original_position", position );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_avformat_close( producer_avformat self )
{
	mlt_log_debug( NULL, "producer_avformat_close\n" );

	// Cleanup av contexts
	av_free_packet( &self->pkt );
	av_free( self->video_frame );
	av_free( self->audio_frame );
	if ( self->is_mutex_init )
		pthread_mutex_lock( &self->open_mutex );
	int i;
	for ( i = 0; i < MAX_AUDIO_STREAMS; i++ )
	{
		mlt_pool_release( self->audio_buffer[i] );
		av_free( self->decode_buffer[i] );
		if ( self->audio_codec[i] )
			avcodec_close( self->audio_codec[i] );
		self->audio_codec[i] = NULL;
	}
	if ( self->video_codec )
		avcodec_close( self->video_codec );
	self->video_codec = NULL;
	// Close the file
	if ( self->dummy_context )
		avformat_close_input( &self->dummy_context );
	if ( self->seekable && self->audio_format )
		avformat_close_input( &self->audio_format );
	if ( self->video_format )
		avformat_close_input( &self->video_format );
	if ( self->is_mutex_init )
		pthread_mutex_unlock( &self->open_mutex );
#ifdef VDPAU
	vdpau_producer_close( self );
#endif
#ifdef AVFILTER
	avfilter_graph_free(&self->vfilter_graph);
#endif

	// Cleanup caches.
	mlt_cache_close( self->image_cache );
	if ( self->last_good_frame )
		mlt_frame_close( self->last_good_frame );

	// Cleanup the mutexes
	if ( self->is_mutex_init )
	{
		pthread_mutex_destroy( &self->audio_mutex );
		pthread_mutex_destroy( &self->video_mutex );
		pthread_mutex_destroy( &self->packets_mutex );
		pthread_mutex_destroy( &self->open_mutex );
	}

	// Cleanup the packet queues
	AVPacket *pkt;
	if ( self->apackets )
	{
		while ( ( pkt = mlt_deque_pop_back( self->apackets ) ) )
		{
			av_free_packet( pkt );
			free( pkt );
		}
		mlt_deque_close( self->apackets );
		self->apackets = NULL;
	}
	if ( self->vpackets )
	{
		while ( ( pkt = mlt_deque_pop_back( self->vpackets ) ) )
		{
			av_free_packet( pkt );
			free( pkt );
		}
		mlt_deque_close( self->vpackets );
		self->vpackets = NULL;
	}

	free( self );
}

static void producer_close( mlt_producer parent )
{
	// Remove this instance from the cache
	mlt_service_cache_purge( MLT_PRODUCER_SERVICE(parent) );

	// Close the parent
	parent->close = NULL;
	mlt_producer_close( parent );

	// Free the memory
	free( parent );
}
