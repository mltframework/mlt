/*
 * consumer_avformat.c -- an encoder based on avformat
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

// mlt Header files
#include <framework/mlt_consumer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_profile.h>
#include <framework/mlt_log.h>
#include <framework/mlt_events.h>

// System header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

// avformat header files
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>

#if LIBAVCODEC_VERSION_MAJOR < 55
#define AV_CODEC_ID_PCM_S16LE CODEC_ID_PCM_S16LE
#define AV_CODEC_ID_PCM_S16BE CODEC_ID_PCM_S16BE
#define AV_CODEC_ID_PCM_U16LE CODEC_ID_PCM_U16LE
#define AV_CODEC_ID_PCM_U16BE CODEC_ID_PCM_U16BE
#define AV_CODEC_ID_H264      CODEC_ID_H264
#define AV_CODEC_ID_NONE      CODEC_ID_NONE
#define AV_CODEC_ID_AC3       CODEC_ID_AC3
#define AV_CODEC_ID_VORBIS    CODEC_ID_VORBIS
#define AV_CODEC_ID_RAWVIDEO  CODEC_ID_RAWVIDEO
#define AV_CODEC_ID_MJPEG     CODEC_ID_MJPEG
#endif

#ifndef AV_CODEC_FLAG_GLOBAL_HEADER
#define AV_CODEC_FLAG_GLOBAL_HEADER  CODEC_FLAG_GLOBAL_HEADER
#define AV_CODEC_FLAG_QSCALE         CODEC_FLAG_QSCALE
#define AV_CODEC_FLAG_INTERLACED_DCT CODEC_FLAG_INTERLACED_DCT
#define AV_CODEC_FLAG_INTERLACED_ME  CODEC_FLAG_INTERLACED_ME
#define AV_CODEC_FLAG_PASS1          CODEC_FLAG_PASS1
#define AV_CODEC_FLAG_PASS2          CODEC_FLAG_PASS2
#endif

#define MAX_AUDIO_STREAMS (8)
#define AUDIO_ENCODE_BUFFER_SIZE (48000 * 2 * MAX_AUDIO_STREAMS)
#define AUDIO_BUFFER_SIZE (1024 * 42)
#define VIDEO_BUFFER_SIZE (8192 * 8192)

//
// This structure should be extended and made globally available in mlt
//

typedef struct
{
	uint8_t *buffer;
	int size;
	int used;
	double time;
	int frequency;
	int channels;
}
*sample_fifo, sample_fifo_s;

sample_fifo sample_fifo_init( int frequency, int channels )
{
	sample_fifo fifo = calloc( 1, sizeof( sample_fifo_s ) );
	fifo->frequency = frequency;
	fifo->channels = channels;
	return fifo;
}

// count is the number of samples multiplied by the number of bytes per sample
void sample_fifo_append( sample_fifo fifo, uint8_t *samples, int count )
{
	if ( ( fifo->size - fifo->used ) < count )
	{
		fifo->size += count * 5;
		fifo->buffer = realloc( fifo->buffer, fifo->size );
	}

	memcpy( &fifo->buffer[ fifo->used ], samples, count );
	fifo->used += count;
}

int sample_fifo_used( sample_fifo fifo )
{
	return fifo->used;
}

int sample_fifo_fetch( sample_fifo fifo, uint8_t *samples, int count )
{
	if ( count > fifo->used )
		count = fifo->used;

	memcpy( samples, fifo->buffer, count );
	fifo->used -= count;
	memmove( fifo->buffer, &fifo->buffer[ count ], fifo->used );

	fifo->time += ( double )count / fifo->channels / fifo->frequency;

	return count;
}

void sample_fifo_close( sample_fifo fifo )
{
	free( fifo->buffer );
	free( fifo );
}

// Forward references.
static void property_changed( mlt_properties owner, mlt_consumer self, char *name );
static int consumer_start( mlt_consumer consumer );
static int consumer_stop( mlt_consumer consumer );
static int consumer_is_stopped( mlt_consumer consumer );
static void *consumer_thread( void *arg );
static void consumer_close( mlt_consumer consumer );

/** Initialise the consumer.
*/

mlt_consumer consumer_avformat_init( mlt_profile profile, char *arg )
{
	// Allocate the consumer
	mlt_consumer consumer = mlt_consumer_new( profile );

	// If memory allocated and initialises without error
	if ( consumer != NULL )
	{
		// Get properties from the consumer
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

		// Assign close callback
		consumer->close = consumer_close;

		// Interpret the argument
		if ( arg != NULL )
			mlt_properties_set( properties, "target", arg );

		// sample and frame queue
		mlt_properties_set_data( properties, "frame_queue", mlt_deque_init( ), 0, ( mlt_destructor )mlt_deque_close, NULL );

		// Audio options not fully handled by AVOptions
#define QSCALE_NONE (-99999)
		mlt_properties_set_int( properties, "aq", QSCALE_NONE );
		
		// Video options not fully handled by AVOptions
		mlt_properties_set_int( properties, "dc", 8 );

		// Muxer options not fully handled by AVOptions
		mlt_properties_set_double( properties, "muxdelay", 0.7 );
		mlt_properties_set_double( properties, "muxpreload", 0.5 );

		// Ensure termination at end of the stream
		mlt_properties_set_int( properties, "terminate_on_pause", 1 );
		
		// Default to separate processing threads for producer and consumer with no frame dropping!
		mlt_properties_set_int( properties, "real_time", -1 );
		mlt_properties_set_int( properties, "prefill", 1 );

		// Set up start/stop/terminated callbacks
		consumer->start = consumer_start;
		consumer->stop = consumer_stop;
		consumer->is_stopped = consumer_is_stopped;
		
		mlt_events_register( properties, "consumer-fatal-error", NULL );
		mlt_event event = mlt_events_listen( properties, consumer, "property-changed", ( mlt_listener )property_changed );
		mlt_properties_set_data( properties, "property-changed event", event, 0, NULL, NULL );
	}

	// Return consumer
	return consumer;
}

static void recompute_aspect_ratio( mlt_properties properties )
{
	double ar = mlt_properties_get_double( properties, "aspect" );
	AVRational rational = av_d2q( ar, 255 );
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );

	// Update the profile and properties as well since this is an alias
	// for mlt properties that correspond to profile settings
	mlt_properties_set_int( properties, "display_aspect_num", rational.num );
	mlt_properties_set_int( properties, "display_aspect_den", rational.den );

	// Now compute the sample aspect ratio
	rational = av_d2q( ar * height / FFMAX(width, 1), 255 );

	// Update the profile and properties as well since this is an alias
	// for mlt properties that correspond to profile settings
	mlt_properties_set_int( properties, "sample_aspect_num", rational.num );
	mlt_properties_set_int( properties, "sample_aspect_den", rational.den );
}

static void color_trc_from_colorspace( mlt_properties properties )
{
	// Default color transfer characteristic from colorspace.
	switch ( mlt_properties_get_int( properties, "colorspace" ) )
	{
	case 709:
		mlt_properties_set_int( properties, "color_trc", AVCOL_TRC_BT709 );
		break;
	case 470:
		mlt_properties_set_int( properties, "color_trc", AVCOL_TRC_GAMMA28 );
		break;
	case 240:
		mlt_properties_set_int( properties, "color_trc", AVCOL_TRC_SMPTE240M );
		break;
#if LIBAVCODEC_VERSION_INT >= ((55<<16)+(52<<8)+0)
	case 0: // sRGB
		mlt_properties_set_int( properties, "color_trc", AVCOL_TRC_IEC61966_2_1 );
		break;
	case 601:
	case 170:
		mlt_properties_set_int( properties, "color_trc", AVCOL_TRC_SMPTE170M );
		break;
#endif
	default:
		break;
	}
}

static void property_changed( mlt_properties owner, mlt_consumer self, char *name )
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( self );

	if ( !strcmp( name, "s" ) )
	{
		// Obtain the size property
		char *size = mlt_properties_get( properties, "s" );
		int width = mlt_properties_get_int( properties, "width" );
		int height = mlt_properties_get_int( properties, "height" );
		int tw, th;

		if ( sscanf( size, "%dx%d", &tw, &th ) == 2 && tw > 0 && th > 0 )
		{
			width = tw;
			height = th;
		}
		else
		{
			mlt_log_warning( MLT_CONSUMER_SERVICE(self), "Invalid size property %s - ignoring.\n", size );
		}

		// Now ensure we honour the multiple of two requested by libavformat
		width = ( width / 2 ) * 2;
		height = ( height / 2 ) * 2;
		mlt_properties_set_int( properties, "width", width );
		mlt_properties_set_int( properties, "height", height );
		recompute_aspect_ratio( properties );
	}
	// "-aspect" on ffmpeg command line is display aspect ratio
	else if ( !strcmp( name, "aspect" ) || !strcmp( name, "width" ) || !strcmp( name, "height" ) )
	{
		recompute_aspect_ratio( properties );
	}
	// Handle the ffmpeg command line "-r" property for frame rate
	else if ( !strcmp( name, "r" ) )
	{
		double frame_rate = mlt_properties_get_double( properties, "r" );
		AVRational rational = av_d2q( frame_rate, 255 );
		mlt_properties_set_int( properties, "frame_rate_num", rational.num );
		mlt_properties_set_int( properties, "frame_rate_den", rational.den );
	}
}

/** Start the consumer.
*/

static int consumer_start( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	int error = 0;

	// Report information about available muxers and codecs as YAML Tiny
	char *s = mlt_properties_get( properties, "f" );
	if ( s && strcmp( s, "list" ) == 0 )
	{
		mlt_properties doc = mlt_properties_new();
		mlt_properties formats = mlt_properties_new();
		char key[20];
		AVOutputFormat *format = NULL;
		
		mlt_properties_set_data( properties, "f", formats, 0, (mlt_destructor) mlt_properties_close, NULL );
		mlt_properties_set_data( doc, "formats", formats, 0, NULL, NULL );
		while ( ( format = av_oformat_next( format ) ) )
		{
			snprintf( key, sizeof(key), "%d", mlt_properties_count( formats ) );
			mlt_properties_set( formats, key, format->name );
		}
		s = mlt_properties_serialise_yaml( doc );
		fprintf( stdout, "%s", s );
		free( s );
		mlt_properties_close( doc );
		error = 1;
	}
	s = mlt_properties_get( properties, "acodec" );
	if ( s && strcmp( s, "list" ) == 0 )
	{
		mlt_properties doc = mlt_properties_new();
		mlt_properties codecs = mlt_properties_new();
		char key[20];
		AVCodec *codec = NULL;

		mlt_properties_set_data( properties, "acodec", codecs, 0, (mlt_destructor) mlt_properties_close, NULL );
		mlt_properties_set_data( doc, "audio_codecs", codecs, 0, NULL, NULL );
		while ( ( codec = av_codec_next( codec ) ) )
			if ( codec->encode2 && codec->type == AVMEDIA_TYPE_AUDIO )
			{
				snprintf( key, sizeof(key), "%d", mlt_properties_count( codecs ) );
				mlt_properties_set( codecs, key, codec->name );
			}
		s = mlt_properties_serialise_yaml( doc );
		fprintf( stdout, "%s", s );
		free( s );
		mlt_properties_close( doc );
		error = 1;
	}
	s = mlt_properties_get( properties, "vcodec" );
	if ( s && strcmp( s, "list" ) == 0 )
	{
		mlt_properties doc = mlt_properties_new();
		mlt_properties codecs = mlt_properties_new();
		char key[20];
		AVCodec *codec = NULL;

		mlt_properties_set_data( properties, "vcodec", codecs, 0, (mlt_destructor) mlt_properties_close, NULL );
		mlt_properties_set_data( doc, "video_codecs", codecs, 0, NULL, NULL );
		while ( ( codec = av_codec_next( codec ) ) )
			if ( codec->encode2 && codec->type == AVMEDIA_TYPE_VIDEO )
			{
				snprintf( key, sizeof(key), "%d", mlt_properties_count( codecs ) );
				mlt_properties_set( codecs, key, codec->name );
			}
		s = mlt_properties_serialise_yaml( doc );
		fprintf( stdout, "%s", s );
		free( s );
		mlt_properties_close( doc );
		error = 1;
	}

	// Check that we're not already running
	if ( !error && !mlt_properties_get_int( properties, "running" ) )
	{
		// Allocate a thread
		pthread_t *thread = calloc( 1, sizeof( pthread_t ) );

		mlt_event_block( mlt_properties_get_data( properties, "property-changed event", NULL ) );

		// Apply AVOptions that are synonyms for standard mlt_consumer options
		if ( mlt_properties_get( properties, "ac" ) )
			mlt_properties_set_int( properties, "channels", mlt_properties_get_int( properties, "ac" ) );
		if ( mlt_properties_get( properties, "ar" ) )
			mlt_properties_set_int( properties, "frequency", mlt_properties_get_int( properties, "ar" ) );

		// Because Movit only reads this on the first frame,
		// we must do this after properties have been set but before first frame request.
		if ( !mlt_properties_get( properties, "color_trc") )
			color_trc_from_colorspace( properties );
	
		// Assign the thread to properties
		mlt_properties_set_data( properties, "thread", thread, sizeof( pthread_t ), free, NULL );

		// Create the thread
		pthread_create( thread, NULL, consumer_thread, consumer );

		// Set the running state
		mlt_properties_set_int( properties, "running", 1 );
	}
	return error;
}

/** Stop the consumer.
*/

static int consumer_stop( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	pthread_t *thread = mlt_properties_get_data( properties, "thread", NULL );

	// Check that we're running
	if ( thread )
	{
		// Stop the thread
		mlt_properties_set_int( properties, "running", 0 );

		// Wait for termination
		pthread_join( *thread, NULL );

		mlt_properties_set_data( properties, "thread", NULL, 0, NULL, NULL );
		mlt_event_unblock( mlt_properties_get_data( properties, "property-changed event", NULL ) );
	}

	return 0;
}

/** Determine if the consumer is stopped.
*/

static int consumer_is_stopped( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	return !mlt_properties_get_int( properties, "running" );
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

		// If option not found, see if it was prefixed with a or v (-vb)
		if ( !opt && (
			( opt_name[0] == 'v' && ( flags & AV_OPT_FLAG_VIDEO_PARAM ) ) ||
			( opt_name[0] == 'a' && ( flags & AV_OPT_FLAG_AUDIO_PARAM ) ) ) )
			opt = av_opt_find( obj, ++opt_name, NULL, flags, flags );
		// Apply option if found
		if ( opt )
			av_opt_set( obj, opt_name, mlt_properties_get_value( properties, i), 0 );
	}
}

static enum AVPixelFormat pick_pix_fmt( mlt_image_format img_fmt )
{
	switch ( img_fmt )
	{
	case mlt_image_rgb24:
		return AV_PIX_FMT_RGB24;
	case mlt_image_rgb24a:
		return AV_PIX_FMT_RGBA;
	case mlt_image_yuv420p:
		return AV_PIX_FMT_YUV420P;
	case mlt_image_yuv422p16:
		return AV_PIX_FMT_YUV422P16LE;
	default:
		return AV_PIX_FMT_YUYV422;
	}
}

static int get_mlt_audio_format( int av_sample_fmt )
{
	switch ( av_sample_fmt )
	{
	case AV_SAMPLE_FMT_U8:
		return mlt_audio_u8;
	case AV_SAMPLE_FMT_S32:
		return mlt_audio_s32le;
	case AV_SAMPLE_FMT_FLT:
		return mlt_audio_f32le;
	case AV_SAMPLE_FMT_U8P:
		return mlt_audio_u8;
	case AV_SAMPLE_FMT_S32P:
		return mlt_audio_s32le;
	case AV_SAMPLE_FMT_FLTP:
		return mlt_audio_f32le;
	default:
		return mlt_audio_s16;
	}
}

static int pick_sample_fmt( mlt_properties properties, AVCodec *codec )
{
	int sample_fmt = AV_SAMPLE_FMT_S16;
	const char *format = mlt_properties_get( properties, "mlt_audio_format" );
	const int *p = codec->sample_fmts;

	// get default av_sample_fmt from mlt_audio_format
	if ( format )
	{
		if ( !strcmp( format, "s32le" ) )
			sample_fmt = AV_SAMPLE_FMT_S32;
		else if ( !strcmp( format, "f32le" ) )
			sample_fmt = AV_SAMPLE_FMT_FLT;
		else if ( !strcmp( format, "u8" ) )
			sample_fmt = AV_SAMPLE_FMT_U8;
		else if ( !strcmp( format, "s32" ) )
			sample_fmt = AV_SAMPLE_FMT_S32P;
		else if ( !strcmp( format, "float" ) )
			sample_fmt = AV_SAMPLE_FMT_FLTP;
	}
	// check if codec supports our mlt_audio_format
	for ( ; *p != -1; p++ )
	{
		if ( *p == sample_fmt )
			return sample_fmt;
	}
	// no match - pick first one we support
	for ( p = codec->sample_fmts; *p != -1; p++ )
	{
		switch (*p)
		{
		case AV_SAMPLE_FMT_U8:
		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S32:
		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_U8P:
		case AV_SAMPLE_FMT_S16P:
		case AV_SAMPLE_FMT_S32P:
		case AV_SAMPLE_FMT_FLTP:
			return *p;
		default:
			break;
		}
	}
	mlt_log_error( properties, "audio codec sample_fmt not compatible" );

	return AV_SAMPLE_FMT_NONE;
}

static uint8_t* interleaved_to_planar( int samples, int channels, uint8_t* audio, int bytes_per_sample )
{
	uint8_t *buffer = mlt_pool_alloc( AUDIO_ENCODE_BUFFER_SIZE );
	uint8_t *p = buffer;
	int c;

	memset( buffer, 0, AUDIO_ENCODE_BUFFER_SIZE );
	for ( c = 0; c < channels; c++ )
	{
		uint8_t *q = audio + c * bytes_per_sample;
		int i = samples + 1;
		while ( --i )
		{
			memcpy( p, q, bytes_per_sample );
			p += bytes_per_sample;
			q += channels * bytes_per_sample;
		}
	}
	return buffer;
}

/** Add an audio output stream
*/

static AVStream *add_audio_stream( mlt_consumer consumer, AVFormatContext *oc, AVCodec *codec, int channels )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Create a new stream
	AVStream *st = avformat_new_stream( oc, codec );

	// If created, then initialise from properties
	if ( st != NULL ) 
	{
		AVCodecContext *c = st->codec;

		// Establish defaults from AVOptions
		avcodec_get_context_defaults3( c, codec );

		c->codec_id = codec->id;
		c->codec_type = AVMEDIA_TYPE_AUDIO;
		c->sample_fmt = pick_sample_fmt( properties, codec );

#if 0 // disabled until some audio codecs are multi-threaded
		// Setup multi-threading
		int thread_count = mlt_properties_get_int( properties, "threads" );
		if ( thread_count == 0 && getenv( "MLT_AVFORMAT_THREADS" ) )
			thread_count = atoi( getenv( "MLT_AVFORMAT_THREADS" ) );
		if ( thread_count > 1 )
			c->thread_count = thread_count;
#endif
	
		if (oc->oformat->flags & AVFMT_GLOBALHEADER) 
			c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		
		// Allow the user to override the audio fourcc
		if ( mlt_properties_get( properties, "atag" ) )
		{
			char *tail = NULL;
			char *arg = mlt_properties_get( properties, "atag" );
			int tag = strtol( arg, &tail, 0);
			if( !tail || *tail )
				tag = arg[ 0 ] + ( arg[ 1 ] << 8 ) + ( arg[ 2 ] << 16 ) + ( arg[ 3 ] << 24 );
			c->codec_tag = tag;
		}

		// Process properties as AVOptions
		char *apre = mlt_properties_get( properties, "apre" );
		if ( apre )
		{
			mlt_properties p = mlt_properties_load( apre );
			apply_properties( c, p, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM );
			mlt_properties_close( p );
		}
		apply_properties( c, properties, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM );

		int audio_qscale = mlt_properties_get_int( properties, "aq" );
		if ( audio_qscale > QSCALE_NONE )
		{
			c->flags |= AV_CODEC_FLAG_QSCALE;
			c->global_quality = FF_QP2LAMBDA * audio_qscale;
		}

		// Set parameters controlled by MLT
		c->sample_rate = mlt_properties_get_int( properties, "frequency" );
#if LIBAVFORMAT_VERSION_INT >= ((55<<16)+(44<<8)+0)
		st->time_base = ( AVRational ){ 1, c->sample_rate };
#else
		c->time_base = ( AVRational ){ 1, c->sample_rate };
#endif
		c->channels = channels;

		if ( mlt_properties_get( properties, "alang" ) != NULL )
			av_dict_set( &oc->metadata, "language", mlt_properties_get( properties, "alang" ), 0 );
	}
	else
	{
		mlt_log_error( MLT_CONSUMER_SERVICE( consumer ), "Could not allocate a stream for audio\n" );
	}

	return st;
}

static int open_audio( mlt_properties properties, AVFormatContext *oc, AVStream *st, int audio_outbuf_size, const char *codec_name )
{
	// We will return the audio input size from here
	int audio_input_frame_size = 0;

	// Get the context
	AVCodecContext *c = st->codec;

	// Find the encoder
	AVCodec *codec;
	if ( codec_name )
		codec = avcodec_find_encoder_by_name( codec_name );
	else
		codec = avcodec_find_encoder( c->codec_id );

	// Process properties as AVOptions on the AVCodec
	if ( codec && codec->priv_class )
	{
		char *apre = mlt_properties_get( properties, "apre" );
		if ( !c->priv_data && codec->priv_data_size )
		{
			c->priv_data = av_mallocz( codec->priv_data_size );
			*(const AVClass **) c->priv_data = codec->priv_class;
//			av_opt_set_defaults( c );
		}
		if ( apre )
		{
			mlt_properties p = mlt_properties_load( apre );
			apply_properties( c->priv_data, p, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM );
			mlt_properties_close( p );
		}
		apply_properties( c->priv_data, properties, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM );
	}

	// Continue if codec found and we can open it
	if ( codec && avcodec_open2( c, codec, NULL ) >= 0 )
	{
		// ugly hack for PCM codecs (will be removed ASAP with new PCM
		// support to compute the input frame size in samples
		if ( c->frame_size <= 1 ) 
		{
			audio_input_frame_size = audio_outbuf_size / c->channels;
			switch(st->codec->codec_id) 
			{
				case AV_CODEC_ID_PCM_S16LE:
				case AV_CODEC_ID_PCM_S16BE:
				case AV_CODEC_ID_PCM_U16LE:
				case AV_CODEC_ID_PCM_U16BE:
					audio_input_frame_size >>= 1;
					break;
				case AV_CODEC_ID_PCM_S24LE:
				case AV_CODEC_ID_PCM_S24BE:
				case AV_CODEC_ID_PCM_U24LE:
				case AV_CODEC_ID_PCM_U24BE:
					audio_input_frame_size /= 3;
					break;
				case AV_CODEC_ID_PCM_S32LE:
				case AV_CODEC_ID_PCM_S32BE:
				case AV_CODEC_ID_PCM_U32LE:
				case AV_CODEC_ID_PCM_U32BE:
					audio_input_frame_size >>= 2;
					break;
				default:
					break;
			}
		} 
		else 
		{
			audio_input_frame_size = c->frame_size;
		}

		// Some formats want stream headers to be seperate (hmm)
		if ( !strcmp( oc->oformat->name, "mp4" ) ||
			 !strcmp( oc->oformat->name, "mov" ) ||
			 !strcmp( oc->oformat->name, "3gp" ) )
			c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
	else
	{
		mlt_log_warning( NULL, "%s: Unable to encode audio - disabling audio output.\n", __FILE__ );
		audio_input_frame_size = 0;
	}

	return audio_input_frame_size;
}

static void close_audio( AVFormatContext *oc, AVStream *st )
{
	if ( st && st->codec )
		avcodec_close( st->codec );
}

/** Add a video output stream 
*/

static AVStream *add_video_stream( mlt_consumer consumer, AVFormatContext *oc, AVCodec *codec )
{
 	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Create a new stream
	AVStream *st = avformat_new_stream( oc, codec );

	if ( st != NULL ) 
	{
		char *pix_fmt = mlt_properties_get( properties, "pix_fmt" );
		AVCodecContext *c = st->codec;

		// Establish defaults from AVOptions
		avcodec_get_context_defaults3( c, codec );

		c->codec_id = codec->id;
		c->codec_type = AVMEDIA_TYPE_VIDEO;
		
		// Setup multi-threading
		int thread_count = mlt_properties_get_int( properties, "threads" );
		if ( thread_count == 0 && getenv( "MLT_AVFORMAT_THREADS" ) )
			thread_count = atoi( getenv( "MLT_AVFORMAT_THREADS" ) );
		if ( thread_count > 1 )
			c->thread_count = thread_count;

		// Process properties as AVOptions
		char *vpre = mlt_properties_get( properties, "vpre" );
		if ( vpre )
		{
			mlt_properties p = mlt_properties_load( vpre );
#ifdef AVDATADIR
			if ( mlt_properties_count( p ) < 1 )
			{
				AVCodec *codec = avcodec_find_encoder( c->codec_id );
				if ( codec )
				{
					char *path = malloc( strlen(AVDATADIR) + strlen(codec->name) + strlen(vpre) + strlen(".ffpreset") + 2 );
					strcpy( path, AVDATADIR );
					strcat( path, codec->name );
					strcat( path, "-" );
					strcat( path, vpre );
					strcat( path, ".ffpreset" );
					
					mlt_properties_close( p );
					p = mlt_properties_load( path );
					if ( mlt_properties_count( p ) > 0 )
						mlt_properties_debug( p, path, stderr );
					free( path );	
				}
			}
			else
			{
				mlt_properties_debug( p, vpre, stderr );			
			}
#endif
			apply_properties( c, p, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM );
			mlt_properties_close( p );
		}
		int colorspace = mlt_properties_get_int( properties, "colorspace" );
		mlt_properties_set( properties, "colorspace", NULL );
		apply_properties( c, properties, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM );
		mlt_properties_set_int( properties, "colorspace", colorspace );

		// Set options controlled by MLT
		c->width = mlt_properties_get_int( properties, "width" );
		c->height = mlt_properties_get_int( properties, "height" );
		c->time_base.num = mlt_properties_get_int( properties, "frame_rate_den" );
		c->time_base.den = mlt_properties_get_int( properties, "frame_rate_num" );
#if LIBAVFORMAT_VERSION_INT < ((55<<16)+(44<<8)+0)
		if ( st->time_base.den == 0 )
#endif
		st->time_base = c->time_base;

		// Default to the codec's first pix_fmt if possible.
		c->pix_fmt = pix_fmt ? av_get_pix_fmt( pix_fmt ) : codec ?
			( codec->pix_fmts ? codec->pix_fmts[0] : AV_PIX_FMT_YUV422P ): AV_PIX_FMT_YUV420P;

		switch ( colorspace )
		{
		case 170:
			c->colorspace = AVCOL_SPC_SMPTE170M;
			break;
		case 240:
			c->colorspace = AVCOL_SPC_SMPTE240M;
			break;
		case 470:
			c->colorspace = AVCOL_SPC_BT470BG;
			break;
		case 601:
			c->colorspace = ( 576 % c->height ) ? AVCOL_SPC_SMPTE170M : AVCOL_SPC_BT470BG;
			break;
		case 709:
			c->colorspace = AVCOL_SPC_BT709;
			break;
		}

		if ( mlt_properties_get( properties, "aspect" ) )
		{
			// "-aspect" on ffmpeg command line is display aspect ratio
			double ar = mlt_properties_get_double( properties, "aspect" );
			c->sample_aspect_ratio = av_d2q( ar * c->height / c->width, 255 );
		}
		else
		{
			c->sample_aspect_ratio.num = mlt_properties_get_int( properties, "sample_aspect_num" );
			c->sample_aspect_ratio.den = mlt_properties_get_int( properties, "sample_aspect_den" );
		}
		st->sample_aspect_ratio = c->sample_aspect_ratio;

		if ( mlt_properties_get_double( properties, "qscale" ) > 0 )
		{
			c->flags |= AV_CODEC_FLAG_QSCALE;
			c->global_quality = FF_QP2LAMBDA * mlt_properties_get_double( properties, "qscale" );
		}

		// Allow the user to override the video fourcc
		if ( mlt_properties_get( properties, "vtag" ) )
		{
			char *tail = NULL;
			const char *arg = mlt_properties_get( properties, "vtag" );
			int tag = strtol( arg, &tail, 0);
			if( !tail || *tail )
				tag = arg[ 0 ] + ( arg[ 1 ] << 8 ) + ( arg[ 2 ] << 16 ) + ( arg[ 3 ] << 24 );
			c->codec_tag = tag;
		}

		// Some formats want stream headers to be seperate
		if ( oc->oformat->flags & AVFMT_GLOBALHEADER ) 
			c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		// Translate these standard mlt consumer properties to ffmpeg
		if ( mlt_properties_get_int( properties, "progressive" ) == 0 &&
		     mlt_properties_get_int( properties, "deinterlace" ) == 0 )
		{
			if ( ! mlt_properties_get( properties, "ildct" ) || mlt_properties_get_int( properties, "ildct" ) )
				c->flags |= AV_CODEC_FLAG_INTERLACED_DCT;
			if ( ! mlt_properties_get( properties, "ilme" ) || mlt_properties_get_int( properties, "ilme" ) )
				c->flags |= AV_CODEC_FLAG_INTERLACED_ME;
		}
		
		// parse the ratecontrol override string
		int i;
		char *rc_override = mlt_properties_get( properties, "rc_override" );
		for ( i = 0; rc_override; i++ )
		{
			int start, end, q;
			int e = sscanf( rc_override, "%d,%d,%d", &start, &end, &q );
			if ( e != 3 )
				mlt_log_warning( MLT_CONSUMER_SERVICE( consumer ), "Error parsing rc_override\n" );
			c->rc_override = av_realloc( c->rc_override, sizeof( RcOverride ) * ( i + 1 ) );
			c->rc_override[i].start_frame = start;
			c->rc_override[i].end_frame = end;
			if ( q > 0 )
			{
				c->rc_override[i].qscale = q;
				c->rc_override[i].quality_factor = 1.0;
			}
			else
			{
				c->rc_override[i].qscale = 0;
				c->rc_override[i].quality_factor = -q / 100.0;
			}
			rc_override = strchr( rc_override, '/' );
			if ( rc_override )
				rc_override++;
		}
		c->rc_override_count = i;
 		if ( !c->rc_initial_buffer_occupancy )
 			c->rc_initial_buffer_occupancy = c->rc_buffer_size * 3/4;
		c->intra_dc_precision = mlt_properties_get_int( properties, "dc" ) - 8;

		// Setup dual-pass
		i = mlt_properties_get_int( properties, "pass" );
		if ( i == 1 )
			c->flags |= AV_CODEC_FLAG_PASS1;
		else if ( i == 2 )
			c->flags |= AV_CODEC_FLAG_PASS2;
#ifdef AV_CODEC_ID_H265
		if ( codec->id != AV_CODEC_ID_H265 )
#endif
		if ( codec->id != AV_CODEC_ID_H264 && ( c->flags & ( AV_CODEC_FLAG_PASS1 | AV_CODEC_FLAG_PASS2 ) ) )
		{
			FILE *f;
			int size;
			char *logbuffer;

			if ( mlt_properties_get( properties, "passlogfile" ) ) {
				mlt_properties_from_utf8( properties, "passlogfile", "_logfilename" );
			} else {
				char logfilename[1024];
				snprintf( logfilename, sizeof(logfilename), "%s_2pass.log", mlt_properties_get( properties, "target" ) );
				mlt_properties_set( properties, "_passlogfile", logfilename );
				mlt_properties_from_utf8( properties, "_passlogfile", "_logfilename" );
			}
			const char *filename = mlt_properties_get( properties, "_logfilename" );
			if ( c->flags & AV_CODEC_FLAG_PASS1 )
			{
				f = fopen( filename, "w" );
				if ( !f )
					perror( filename );
				else
					mlt_properties_set_data( properties, "_logfile", f, 0, ( mlt_destructor )fclose, NULL );
			}
			else
			{
				/* read the log file */
				f = fopen( filename, "r" );
				if ( !f )
				{
					perror( filename );
				}
				else
				{
					fseek( f, 0, SEEK_END );
					size = ftell( f );
					fseek( f, 0, SEEK_SET );
					logbuffer = av_malloc( size + 1 );
					if ( !logbuffer )
						mlt_log_fatal( MLT_CONSUMER_SERVICE( consumer ), "Could not allocate log buffer\n" );
					else
					{
						if ( size >= 0 )
						{
							size = fread( logbuffer, 1, size, f );
							logbuffer[size] = '\0';
							c->stats_in = logbuffer;
						}
					}
					fclose( f );
				}
			}
		}
	}
	else
	{
		mlt_log_error( MLT_CONSUMER_SERVICE( consumer ), "Could not allocate a stream for video\n" );
	}
 
	return st;
}

static AVFrame *alloc_picture( int pix_fmt, int width, int height )
{
	// Allocate a frame
#if LIBAVCODEC_VERSION_INT >= ((55<<16)+(45<<8)+0)
	AVFrame *picture = av_frame_alloc();
#else
	AVFrame *picture = avcodec_alloc_frame();
#endif

	// Determine size of the 
	int size = avpicture_get_size(pix_fmt, width, height);

	// Allocate the picture buf
	uint8_t *picture_buf = av_malloc(size);

	// If we have both, then fill the image
	if ( picture != NULL && picture_buf != NULL )
	{
		// Fill the frame with the allocated buffer
		avpicture_fill( (AVPicture *)picture, picture_buf, pix_fmt, width, height);
		picture->format = pix_fmt;
		picture->width = width;
		picture->height = height;
	}
	else
	{
		// Something failed - clean up what we can
	 	av_free( picture );
	 	av_free( picture_buf );
	 	picture = NULL;
	}

	return picture;
}

static int open_video( mlt_properties properties, AVFormatContext *oc, AVStream *st, const char *codec_name )
{
	// Get the codec
	AVCodecContext *video_enc = st->codec;

	// find the video encoder
	AVCodec *codec;
	if ( codec_name )
		codec = avcodec_find_encoder_by_name( codec_name );
	else
		codec = avcodec_find_encoder( video_enc->codec_id );

	// Process properties as AVOptions on the AVCodec
	if ( codec && codec->priv_class )
	{
		char *vpre = mlt_properties_get( properties, "vpre" );
		if ( !video_enc->priv_data && codec->priv_data_size )
		{
			video_enc->priv_data = av_mallocz( codec->priv_data_size );
			*(const AVClass **) video_enc->priv_data = codec->priv_class;
//			av_opt_set_defaults( video_enc );
		}
		if ( vpre )
		{
			mlt_properties p = mlt_properties_load( vpre );
			apply_properties( video_enc->priv_data, p, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM );
			mlt_properties_close( p );
		}
		apply_properties( video_enc->priv_data, properties, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM );
	}

	if( codec && codec->pix_fmts )
	{
		const enum AVPixelFormat *p = codec->pix_fmts;
		for( ; *p!=-1; p++ )
		{
			if( *p == video_enc->pix_fmt )
				break;
		}
		if( *p == -1 )
			video_enc->pix_fmt = codec->pix_fmts[ 0 ];
	}

	int result = codec && avcodec_open2( video_enc, codec, NULL ) >= 0;
	
	return result;
}

void close_video(AVFormatContext *oc, AVStream *st)
{
	if ( st && st->codec )
	{
		av_freep( &st->codec->stats_in );
		avcodec_close(st->codec);
	}
}

static inline long time_difference( struct timeval *time1 )
{
	struct timeval time2;
	gettimeofday( &time2, NULL );
	return time2.tv_sec * 1000000 + time2.tv_usec - time1->tv_sec * 1000000 - time1->tv_usec;
}

static int mlt_write(void *h, uint8_t *buf, int size)
{
	mlt_properties properties = (mlt_properties) h;
	mlt_events_fire( properties, "avformat-write", buf, &size, NULL );
	return 0;
}

static void write_transmitter( mlt_listener listener, mlt_properties owner, mlt_service service, void **args )
{
	int *p_size = (int*) args[1];
	listener( owner, service, (uint8_t*) args[0], *p_size );
}

/** The main thread - the argument is simply the consumer.
*/

static void *consumer_thread( void *arg )
{
	// Map the argument to the object
	mlt_consumer consumer = arg;

	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Get the terminate on pause property
	int terminate_on_pause = mlt_properties_get_int( properties, "terminate_on_pause" );
	int terminated = 0;

	// Determine if feed is slow (for realtime stuff)
	int real_time_output = mlt_properties_get_int( properties, "real_time" );

	// Time structures
	struct timeval ante;

	// Get the frame rate
	double fps = mlt_properties_get_double( properties, "fps" );

	// Get width and height
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );
	int img_width = width;
	int img_height = height;

	// Get default audio properties
	int channels = mlt_properties_get_int( properties, "channels" );
	int total_channels = channels;
	int frequency = mlt_properties_get_int( properties, "frequency" );
	void *pcm = NULL;
	int samples = 0;

	// AVFormat audio buffer and frame size
	int audio_outbuf_size = AUDIO_BUFFER_SIZE;
	uint8_t *audio_outbuf = av_malloc( audio_outbuf_size );
	int audio_input_nb_samples = 0;

	// AVFormat video buffer and frame count
	int frame_count = 0;
	int video_outbuf_size = VIDEO_BUFFER_SIZE;
	uint8_t *video_outbuf = av_malloc( video_outbuf_size );

	// Used for the frame properties
	mlt_frame frame = NULL;
	mlt_properties frame_properties = NULL;

	// Get the queues
	mlt_deque queue = mlt_properties_get_data( properties, "frame_queue", NULL );
	sample_fifo fifo = mlt_properties_get_data( properties, "sample_fifo", NULL );

	// For receiving images from an mlt_frame
	uint8_t *image;
	mlt_image_format img_fmt = mlt_image_yuv422;

	// Need two av pictures for converting
	AVFrame *converted_avframe = NULL;
	AVFrame *audio_avframe = NULL;

	// For receiving audio samples back from the fifo
	uint8_t *audio_buf_1 = av_malloc( AUDIO_ENCODE_BUFFER_SIZE );
	uint8_t *audio_buf_2 = NULL;
	int count = 0;

	// Allocate the context
	AVFormatContext *oc = NULL;

	// Streams
	AVStream *video_st = NULL;
	AVStream *audio_st[ MAX_AUDIO_STREAMS ];

	// Time stamps
	double audio_pts = 0;
	double video_pts = 0;

	// Frames dispatched
	long int frames = 0;
	long int total_time = 0;

	// Determine the format
	AVOutputFormat *fmt = NULL;
	mlt_properties_from_utf8( properties, "target", "_target" );
	const char *filename = mlt_properties_get( properties, "_target" );
	char *format = mlt_properties_get( properties, "f" );
	char *vcodec = mlt_properties_get( properties, "vcodec" );
	char *acodec = mlt_properties_get( properties, "acodec" );
	AVCodec *audio_codec = NULL;
	AVCodec *video_codec = NULL;
	
	// Used to store and override codec ids
	int audio_codec_id;
	int video_codec_id;

	// Misc
	char key[27];
	mlt_properties frame_meta_properties = mlt_properties_new();
	int error_count = 0;
	int64_t sample_count[ MAX_AUDIO_STREAMS ];
	int header_written = 0;

	// Initialize audio_st
	int i = MAX_AUDIO_STREAMS;
	while ( i-- )
	{
		audio_st[i] = NULL;
		sample_count[i] = 0;
	}

	// Check for user selected format first
	if ( format != NULL )
		fmt = av_guess_format( format, NULL, NULL );

	// Otherwise check on the filename
	if ( fmt == NULL && filename != NULL )
		fmt = av_guess_format( NULL, filename, NULL );

	// Otherwise default to mpeg
	if ( fmt == NULL )
		fmt = av_guess_format( "mpeg", NULL, NULL );

	// We need a filename - default to stdout?
	if ( filename == NULL || !strcmp( filename, "" ) )
		filename = "pipe:";

#if defined(FFUDIV) && LIBAVUTIL_VERSION_INT >= ((53<<16)+(2<<8)+0)
	avformat_alloc_output_context2( &oc, fmt, format, filename );
#else
	oc = avformat_alloc_context( );
	oc->oformat = fmt;
	snprintf( oc->filename, sizeof(oc->filename), "%s", filename );

	if ( oc->oformat && oc->oformat->priv_class && !oc->priv_data && oc->oformat->priv_data_size ) {
		oc->priv_data = av_mallocz( oc->oformat->priv_data_size );
		if ( oc->priv_data ) {
			*(const AVClass**)oc->priv_data = oc->oformat->priv_class;
			av_opt_set_defaults( oc->priv_data );
		}
	}
#endif

	// Get the codec ids selected
	audio_codec_id = fmt->audio_codec;
	video_codec_id = fmt->video_codec;

	// Check for audio codec overides
	if ( ( acodec && strcmp( acodec, "none" ) == 0 ) || mlt_properties_get_int( properties, "an" ) )
		audio_codec_id = AV_CODEC_ID_NONE;
	else if ( acodec )
	{
		audio_codec = avcodec_find_encoder_by_name( acodec );
		if ( audio_codec )
		{
			audio_codec_id = audio_codec->id;
			if ( audio_codec_id == AV_CODEC_ID_AC3 && avcodec_find_encoder_by_name( "ac3_fixed" ) )
			{
				mlt_properties_set( properties, "_acodec", "ac3_fixed" );
				acodec = mlt_properties_get( properties, "_acodec" );
				audio_codec = avcodec_find_encoder_by_name( acodec );
			}
			else if ( !strcmp( acodec, "aac" ) || !strcmp( acodec, "vorbis" ) )
			{
				mlt_properties_set( properties, "astrict", "experimental" );
			}
		}
		else
		{
			audio_codec_id = AV_CODEC_ID_NONE;
			mlt_log_warning( MLT_CONSUMER_SERVICE( consumer ), "audio codec %s unrecognised - ignoring\n", acodec );
		}
	}
	else
	{
		audio_codec = avcodec_find_encoder( audio_codec_id );
	}

	// Check for video codec overides
	if ( ( vcodec && strcmp( vcodec, "none" ) == 0 ) || mlt_properties_get_int( properties, "vn" ) )
		video_codec_id = AV_CODEC_ID_NONE;
	else if ( vcodec )
	{
		video_codec = avcodec_find_encoder_by_name( vcodec );
		if ( video_codec )
		{
			video_codec_id = video_codec->id;
		}
		else
		{
			video_codec_id = AV_CODEC_ID_NONE;
			mlt_log_warning( MLT_CONSUMER_SERVICE( consumer ), "video codec %s unrecognised - ignoring\n", vcodec );
		}
	}
	else
	{
		video_codec = avcodec_find_encoder( video_codec_id );
	}

	// Write metadata
	for ( i = 0; i < mlt_properties_count( properties ); i++ )
	{
		char *name = mlt_properties_get_name( properties, i );
		if ( name && !strncmp( name, "meta.attr.", 10 ) )
		{
			char *key = strdup( name + 10 );
			char *markup = strrchr( key, '.' );
			if ( markup && !strcmp( markup, ".markup") )
			{
				markup[0] = '\0';
				if ( !strstr( key, ".stream." ) )
					av_dict_set( &oc->metadata, key, mlt_properties_get_value( properties, i ), 0 );
			}
			free( key );
		}
	}

	// Add audio and video streams
	if ( video_codec_id != AV_CODEC_ID_NONE )
	{
		if ( ( video_st = add_video_stream( consumer, oc, video_codec ) ) )
		{
			const char* img_fmt_name = mlt_properties_get( properties, "mlt_image_format" );
			if ( img_fmt_name )
			{
				// Set the mlt_image_format from explicit property.
				mlt_image_format f = mlt_image_format_id( img_fmt_name );
				if ( mlt_image_invalid != f )
					img_fmt = f;
			}
			else
			{
				// Set the mlt_image_format from the selected pix_fmt.
				const char *pix_fmt_name = av_get_pix_fmt_name( video_st->codec->pix_fmt );
				if ( !strcmp( pix_fmt_name, "rgba" ) ||
					 !strcmp( pix_fmt_name, "argb" ) ||
					 !strcmp( pix_fmt_name, "bgra" ) ) {
					mlt_properties_set( properties, "mlt_image_format", "rgb24a" );
					img_fmt = mlt_image_rgb24a;
				} else if ( strstr( pix_fmt_name, "rgb" ) ||
							strstr( pix_fmt_name, "bgr" ) ) {
					mlt_properties_set( properties, "mlt_image_format", "rgb24" );
					img_fmt = mlt_image_rgb24;
				}
			}
		}
	}
	if ( audio_codec_id != AV_CODEC_ID_NONE )
	{
		int is_multi = 0;

		total_channels = 0;
		// multitrack audio
		for ( i = 0; i < MAX_AUDIO_STREAMS; i++ )
		{
			sprintf( key, "channels.%d", i );
			int j = mlt_properties_get_int( properties, key );
			if ( j )
			{
				is_multi = 1;
				total_channels += j;
				audio_st[i] = add_audio_stream( consumer, oc, audio_codec, j );
			}
		}
		// single track
		if ( !is_multi )
		{
			audio_st[0] = add_audio_stream( consumer, oc, audio_codec, channels );
			total_channels = channels;
		}
	}
	mlt_properties_set_int( properties, "channels", total_channels );

	// Audio format is determined when adding the audio stream
	mlt_audio_format aud_fmt = mlt_audio_none;
	if ( audio_st[0] )
		aud_fmt = get_mlt_audio_format( audio_st[0]->codec->sample_fmt );
	int sample_bytes = mlt_audio_format_size( aud_fmt, 1, 1 );
	sample_bytes = sample_bytes ? sample_bytes : 1; // prevent divide by zero

	// Set the parameters (even though we have none...)
	{
		if ( mlt_properties_get( properties, "muxpreload" ) && ! mlt_properties_get( properties, "preload" ) )
			mlt_properties_set_double( properties, "preload", mlt_properties_get_double( properties, "muxpreload" ) );
		oc->max_delay= ( int )( mlt_properties_get_double( properties, "muxdelay" ) * AV_TIME_BASE );

		// Process properties as AVOptions
		char *fpre = mlt_properties_get( properties, "fpre" );
		if ( fpre )
		{
			mlt_properties p = mlt_properties_load( fpre );
			apply_properties( oc, p, AV_OPT_FLAG_ENCODING_PARAM );
			if ( oc->oformat && oc->oformat->priv_class && oc->priv_data )
				apply_properties( oc->priv_data, p, AV_OPT_FLAG_ENCODING_PARAM );
			mlt_properties_close( p );
		}
		apply_properties( oc, properties, AV_OPT_FLAG_ENCODING_PARAM );
		if ( oc->oformat && oc->oformat->priv_class && oc->priv_data )
			apply_properties( oc->priv_data, properties, AV_OPT_FLAG_ENCODING_PARAM );

		if ( video_st && !open_video( properties, oc, video_st, vcodec? vcodec : NULL ) )
			video_st = NULL;
		for ( i = 0; i < MAX_AUDIO_STREAMS && audio_st[i]; i++ )
		{
			audio_input_nb_samples = open_audio( properties, oc, audio_st[i], audio_outbuf_size,
				acodec? acodec : NULL );
			if ( !audio_input_nb_samples )
			{
				// Remove the audio stream from the output context
				int j;
				for ( j = 0; j < oc->nb_streams; j++ )
				{
					if ( oc->streams[j] == audio_st[i] )
						av_freep( &oc->streams[j] );
				}
				--oc->nb_streams;
				audio_st[i] = NULL;
			}
		}

		// Setup custom I/O if redirecting
		if ( mlt_properties_get_int( properties, "redirect" ) )
		{
			int buffer_size = 32768;
			unsigned char *buffer = av_malloc( buffer_size );
			AVIOContext* io = avio_alloc_context( buffer, buffer_size, 1, properties, NULL, mlt_write, NULL );
			if ( buffer && io )
			{
				oc->pb = io;
				oc->flags |= AVFMT_FLAG_CUSTOM_IO;
				mlt_properties_set_data( properties, "avio_buffer", buffer, buffer_size, av_free, NULL );
				mlt_properties_set_data( properties, "avio_context", io, 0, av_free, NULL );
				mlt_events_register( properties, "avformat-write", (mlt_transmitter) write_transmitter );
			}
			else
			{
				av_free( buffer );
				mlt_log_error( MLT_CONSUMER_SERVICE(consumer), "failed to setup output redirection\n" );
			}
		}
		// Open the output file, if needed
		else if ( !( fmt->flags & AVFMT_NOFILE ) )
		{
			if ( avio_open( &oc->pb, filename, AVIO_FLAG_WRITE ) < 0 )
			{
				mlt_log_error( MLT_CONSUMER_SERVICE( consumer ), "Could not open '%s'\n", filename );
				mlt_events_fire( properties, "consumer-fatal-error", NULL );
				goto on_fatal_error;
			}
		}
	
	}

	// Last check - need at least one stream
	if ( !audio_st[0] && !video_st )
	{
		mlt_events_fire( properties, "consumer-fatal-error", NULL );
		goto on_fatal_error;
	}

	// Allocate picture
	if ( video_st )
		converted_avframe = alloc_picture( video_st->codec->pix_fmt, width, height );

	// Allocate audio AVFrame
	if ( audio_st[0] )
	{
#if LIBAVCODEC_VERSION_INT >= ((55<<16)+(45<<8)+0)
		audio_avframe = av_frame_alloc();
#else
		audio_avframe = avcodec_alloc_frame();
#endif
		if ( audio_avframe ) {
			AVCodecContext *c = audio_st[0]->codec;
			audio_avframe->format = c->sample_fmt;
			audio_avframe->nb_samples = audio_input_nb_samples;
			audio_avframe->channel_layout = c->channel_layout;
		} else {
			mlt_log_error( MLT_CONSUMER_SERVICE(consumer), "failed to allocate audio AVFrame\n" );
			mlt_events_fire( properties, "consumer-fatal-error", NULL );
			goto on_fatal_error;
		}
	}

	// Get the starting time (can ignore the times above)
	gettimeofday( &ante, NULL );

	// Loop while running
	while( mlt_properties_get_int( properties, "running" ) &&
	       ( !terminated || ( video_st && mlt_deque_count( queue ) ) ) )
	{
		if ( !frame )
			frame = mlt_consumer_rt_frame( consumer );

		// Check that we have a frame to work with
		if ( frame != NULL )
		{
			// Default audio args
			frame_properties = MLT_FRAME_PROPERTIES( frame );

			// Write the stream header.
			if ( !header_written )
			{
				// set timecode from first frame if not been set from metadata
				if( !mlt_properties_get( properties, "timecode" ) )
				{
					char *vitc = mlt_properties_get( frame_properties, "meta.attr.vitc.markup" );
					if ( vitc && vitc[0] )
					{
						mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "timecode=[%s]\n", vitc );
						av_dict_set( &oc->metadata, "timecode", vitc, 0 );

						if ( video_st )
							av_dict_set( &video_st->metadata, "timecode", vitc, 0 );
					};
				};

				if ( avformat_write_header( oc, NULL ) < 0 )
				{
					mlt_log_error( MLT_CONSUMER_SERVICE( consumer ), "Could not write header '%s'\n", filename );
					mlt_events_fire( properties, "consumer-fatal-error", NULL );
					goto on_fatal_error;
				}

				header_written = 1;
			}

			// Increment frames dispatched
			frames ++;

			// Check for the terminated condition
			terminated = terminate_on_pause && mlt_properties_get_double( frame_properties, "_speed" ) == 0.0;

			// Get audio and append to the fifo
			if ( !terminated && audio_st[0] )
			{
				samples = mlt_sample_calculator( fps, frequency, count ++ );
				channels = total_channels;
				mlt_frame_get_audio( frame, &pcm, &aud_fmt, &frequency, &channels, &samples );

				// Save the audio channel remap properties for later
				mlt_properties_pass( frame_meta_properties, frame_properties, "meta.map.audio." );

				// Create the fifo if we don't have one
				if ( fifo == NULL )
				{
					fifo = sample_fifo_init( frequency, channels );
					mlt_properties_set_data( properties, "sample_fifo", fifo, 0, ( mlt_destructor )sample_fifo_close, NULL );
				}
				if ( pcm )
				{
					// Silence if not normal forward speed
					if ( mlt_properties_get_double( frame_properties, "_speed" ) != 1.0 )
						memset( pcm, 0, samples * channels * sample_bytes );

					// Append the samples
					sample_fifo_append( fifo, pcm, samples * channels * sample_bytes );
					total_time += ( samples * 1000000 ) / frequency;
				}
				if ( !video_st )
					mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
			}

			// Encode the image
			if ( !terminated && video_st )
				mlt_deque_push_back( queue, frame );
			else
				mlt_frame_close( frame );
			frame = NULL;
		}

		// While we have stuff to process, process...
		while ( 1 )
		{
			// Write interleaved audio and video frames
			if ( !video_st || ( video_st && audio_st[0] && audio_pts < video_pts ) )
			{
				// Write audio
				if ( ( video_st && terminated ) || ( channels * audio_input_nb_samples ) < sample_fifo_used( fifo ) / sample_bytes )
				{
					int j = 0; // channel offset into interleaved source buffer
					int n = FFMIN( FFMIN( channels * audio_input_nb_samples, sample_fifo_used( fifo ) / sample_bytes ), AUDIO_ENCODE_BUFFER_SIZE );

					// Get the audio samples
					if ( n > 0 )
					{
						sample_fifo_fetch( fifo, audio_buf_1, n * sample_bytes );
					}
					else if ( audio_codec_id == AV_CODEC_ID_VORBIS && terminated )
					{
						// This prevents an infinite loop when some versions of vorbis do not
						// increment pts when encoding silence.
						audio_pts = video_pts;
						break;
					}
					else
					{
						memset( audio_buf_1, 0, AUDIO_ENCODE_BUFFER_SIZE );
					}
					samples = n / channels;

					// For each output stream
					for ( i = 0; i < MAX_AUDIO_STREAMS && audio_st[i] && j < total_channels; i++ )
					{
						AVStream *stream = audio_st[i];
						AVCodecContext *codec = stream->codec;
						AVPacket pkt;

						av_init_packet( &pkt );
						pkt.data = audio_outbuf;
						pkt.size = audio_outbuf_size;

						// Optimized for single track and no channel remap
						if ( !audio_st[1] && !mlt_properties_count( frame_meta_properties ) )
						{
							void* p = audio_buf_1;
							if ( codec->sample_fmt == AV_SAMPLE_FMT_FLTP )
								p = interleaved_to_planar( samples, channels, p, sizeof( float ) );
							else if ( codec->sample_fmt == AV_SAMPLE_FMT_S16P )
								p = interleaved_to_planar( samples, channels, p, sizeof( int16_t ) );
							else if ( codec->sample_fmt == AV_SAMPLE_FMT_S32P )
								p = interleaved_to_planar( samples, channels, p, sizeof( int32_t ) );
							else if ( codec->sample_fmt == AV_SAMPLE_FMT_U8P )
								p = interleaved_to_planar( samples, channels, p, sizeof( uint8_t ) );
							audio_avframe->nb_samples = FFMAX( samples, audio_input_nb_samples );
#if LIBAVCODEC_VERSION_MAJOR >= 55
							audio_avframe->pts = sample_count[i];
#endif
							sample_count[i] += audio_avframe->nb_samples;
							avcodec_fill_audio_frame( audio_avframe, codec->channels, codec->sample_fmt,
								(const uint8_t*) p, AUDIO_ENCODE_BUFFER_SIZE, 0 );
							int got_packet = 0;
							int ret = avcodec_encode_audio2( codec, &pkt, audio_avframe, &got_packet );
							if ( ret < 0 )
								pkt.size = ret;
							else if ( !got_packet )
								pkt.size = 0;

							if ( p != audio_buf_1 )
								mlt_pool_release( p );
						}
						else
						{
							// Extract the audio channels according to channel mapping
							int dest_offset = 0; // channel offset into interleaved dest buffer

							// Get the number of channels for this stream
							sprintf( key, "channels.%d", i );
							int current_channels = mlt_properties_get_int( properties, key );

							// Clear the destination audio buffer.
							if ( !audio_buf_2 )
								audio_buf_2 = av_mallocz( AUDIO_ENCODE_BUFFER_SIZE );
							else
								memset( audio_buf_2, 0, AUDIO_ENCODE_BUFFER_SIZE );

							// For each output channel
							while ( dest_offset < current_channels && j < total_channels )
							{
								int map_start = -1, map_channels = 0;
								int source_offset = 0;
								int k;

								// Look for a mapping that starts at j
								for ( k = 0; k < (MAX_AUDIO_STREAMS * 2) && map_start != j; k++ )
								{
									sprintf( key, "%d.channels", k );
									map_channels = mlt_properties_get_int( frame_meta_properties, key );
									sprintf( key, "%d.start", k );
									if ( mlt_properties_get( frame_meta_properties, key ) )
										map_start = mlt_properties_get_int( frame_meta_properties, key );
									if ( map_start != j )
										source_offset += map_channels;
								}

								// If no mapping
								if ( map_start != j )
								{
									map_channels = current_channels;
									source_offset = j;
								}

								// Copy samples if source offset valid
								if ( source_offset < channels )
								{
									// Interleave the audio buffer with the # channels for this stream/mapping.
									for ( k = 0; k < map_channels; k++, j++, source_offset++, dest_offset++ )
									{
										void *src = audio_buf_1 + source_offset * sample_bytes;
										void *dest = audio_buf_2 + dest_offset * sample_bytes;
										int s = samples + 1;

										while ( --s ) {
											memcpy( dest, src, sample_bytes );
											dest += current_channels * sample_bytes;
											src += channels * sample_bytes;
										}
									}
								}
								// Otherwise silence
								else
								{
									j += current_channels;
									dest_offset += current_channels;
								}
							}
							audio_avframe->nb_samples = FFMAX( samples, audio_input_nb_samples );
#if LIBAVCODEC_VERSION_MAJOR >= 55
							audio_avframe->pts = sample_count[i];
							sample_count[i] += audio_avframe->nb_samples;
#endif
							avcodec_fill_audio_frame( audio_avframe, codec->channels, codec->sample_fmt,
								(const uint8_t*) audio_buf_2, AUDIO_ENCODE_BUFFER_SIZE, 0 );
							int got_packet = 0;
							int ret = avcodec_encode_audio2( codec, &pkt, audio_avframe, &got_packet );
							if ( ret < 0 )
								pkt.size = ret;
							else if ( !got_packet )
								pkt.size = 0;
						}

						if ( pkt.size > 0 )
						{
							// Write the compressed frame in the media file
							if ( pkt.pts != AV_NOPTS_VALUE )
								pkt.pts = av_rescale_q( pkt.pts, codec->time_base, stream->time_base );
#if LIBAVFORMAT_VERSION_INT >= ((55<<16)+(44<<8)+0)
							if ( pkt.dts != AV_NOPTS_VALUE )
								pkt.dts = av_rescale_q( pkt.dts, codec->time_base, stream->time_base );
							if ( pkt.duration > 0 )
								pkt.duration = av_rescale_q( pkt.duration, codec->time_base, stream->time_base );
#endif
							pkt.stream_index = stream->index;
							if ( av_interleaved_write_frame( oc, &pkt ) )
							{
								mlt_log_fatal( MLT_CONSUMER_SERVICE( consumer ), "error writing audio frame\n" );
								mlt_events_fire( properties, "consumer-fatal-error", NULL );
								goto on_fatal_error;
							}
							error_count = 0;
							mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "audio stream %d pkt pts %"PRId64" frame_size %d\n",
								stream->index, pkt.pts, codec->frame_size );
						}
						else if ( pkt.size < 0 )
						{
							mlt_log_warning( MLT_CONSUMER_SERVICE( consumer ), "error with audio encode %d\n", frame_count );
							if ( ++error_count > 2 )
								goto on_fatal_error;
						}

						if ( i == 0 )
						{
#if LIBAVFORMAT_VERSION_INT >= ((55<<16)+(44<<8)+0)
							audio_pts = (double) sample_count[0] * av_q2d( stream->codec->time_base );
#else
							audio_pts = (double) sample_count[0] * av_q2d( stream->time_base );
#endif
						}
					}
				}
				else
				{
					break;
				}
			}
			else if ( video_st )
			{
				// Write video
				if ( mlt_deque_count( queue ) )
				{
					int ret = 0;
					AVCodecContext *c = video_st->codec;

					frame = mlt_deque_pop_front( queue );
					frame_properties = MLT_FRAME_PROPERTIES( frame );

					if ( mlt_properties_get_int( frame_properties, "rendered" ) )
					{
						AVFrame video_avframe;
						mlt_frame_get_image( frame, &image, &img_fmt, &img_width, &img_height, 0 );

						mlt_image_format_planes( img_fmt, width, height, image, video_avframe.data, video_avframe.linesize );

						// Do the colour space conversion
						int flags = SWS_BICUBIC;
						struct SwsContext *context = sws_getContext( width, height, pick_pix_fmt( img_fmt ),
							width, height, c->pix_fmt, flags, NULL, NULL, NULL);
						sws_scale( context, (const uint8_t* const*) video_avframe.data, video_avframe.linesize, 0, height,
							converted_avframe->data, converted_avframe->linesize);
						sws_freeContext( context );

						mlt_events_fire( properties, "consumer-frame-show", frame, NULL );

						// Apply the alpha if applicable
						if ( !mlt_properties_get( properties, "mlt_image_format" ) ||
						     strcmp( mlt_properties_get( properties, "mlt_image_format" ), "rgb24a" ) )
						if ( c->pix_fmt == AV_PIX_FMT_RGBA ||
						     c->pix_fmt == AV_PIX_FMT_ARGB ||
						     c->pix_fmt == AV_PIX_FMT_BGRA )
						{
							uint8_t *p;
							uint8_t *alpha = mlt_frame_get_alpha_mask( frame );
							register int n;

							for ( i = 0; i < height; i ++ )
							{
								n = ( width + 7 ) / 8;
								p = converted_avframe->data[ 0 ] + i * converted_avframe->linesize[ 0 ] + 3;

								switch( width % 8 )
								{
									case 0:	do { *p = *alpha++; p += 4;
									case 7:		 *p = *alpha++; p += 4;
									case 6:		 *p = *alpha++; p += 4;
									case 5:		 *p = *alpha++; p += 4;
									case 4:		 *p = *alpha++; p += 4;
									case 3:		 *p = *alpha++; p += 4;
									case 2:		 *p = *alpha++; p += 4;
									case 1:		 *p = *alpha++; p += 4;
											}
											while( --n );
								}
							}
						}
					}

					if (oc->oformat->flags & AVFMT_RAWPICTURE) 
					{
	 					// raw video case. The API will change slightly in the near future for that
						AVPacket pkt;
						av_init_packet(&pkt);

						// Set frame interlace hints
						if ( mlt_properties_get_int( frame_properties, "progressive" ) )
							c->field_order = AV_FIELD_PROGRESSIVE;
						else
							c->field_order = (mlt_properties_get_int( frame_properties, "top_field_first" )) ? AV_FIELD_TB : AV_FIELD_BT;
						pkt.flags |= AV_PKT_FLAG_KEY;
						pkt.stream_index = video_st->index;
						pkt.data = (uint8_t *)converted_avframe;
						pkt.size = sizeof(AVPicture);

						ret = av_write_frame(oc, &pkt);
					} 
					else 
					{
						AVPacket pkt;
						av_init_packet( &pkt );
						if ( c->codec->id == AV_CODEC_ID_RAWVIDEO ) {
							pkt.data = NULL;
							pkt.size = 0;
						} else {
							pkt.data = video_outbuf;
							pkt.size = video_outbuf_size;
						}

						// Set the quality
						converted_avframe->quality = c->global_quality;
						converted_avframe->pts = frame_count;

						// Set frame interlace hints
						converted_avframe->interlaced_frame = !mlt_properties_get_int( frame_properties, "progressive" );
						converted_avframe->top_field_first = mlt_properties_get_int( frame_properties, "top_field_first" );
						if ( mlt_properties_get_int( frame_properties, "progressive" ) )
							c->field_order = AV_FIELD_PROGRESSIVE;
						else if ( c->codec_id == AV_CODEC_ID_MJPEG )
							c->field_order = (mlt_properties_get_int( frame_properties, "top_field_first" )) ? AV_FIELD_TT : AV_FIELD_BB;
						else
							c->field_order = (mlt_properties_get_int( frame_properties, "top_field_first" )) ? AV_FIELD_TB : AV_FIELD_BT;

	 					// Encode the image
#if LIBAVCODEC_VERSION_MAJOR >= 55
						int got_packet;
						ret = avcodec_encode_video2( c, &pkt, converted_avframe, &got_packet );
						if ( ret < 0 )
							pkt.size = ret;
						else if ( !got_packet )
							pkt.size = 0;
#else
	 					pkt.size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, converted_avframe );
	 					pkt.pts = c->coded_frame? c->coded_frame->pts : AV_NOPTS_VALUE;
						if ( c->coded_frame && c->coded_frame->key_frame )
							pkt.flags |= AV_PKT_FLAG_KEY;
#endif

	 					// If zero size, it means the image was buffered
						if ( pkt.size > 0 )
						{
							if ( pkt.pts != AV_NOPTS_VALUE )
								pkt.pts = av_rescale_q( pkt.pts, c->time_base, video_st->time_base );
#if LIBAVCODEC_VERSION_MAJOR >= 55
							if ( pkt.dts != AV_NOPTS_VALUE )
								pkt.dts = av_rescale_q( pkt.dts, c->time_base, video_st->time_base );
#endif
							pkt.stream_index = video_st->index;

							// write the compressed frame in the media file
							ret = av_interleaved_write_frame(oc, &pkt);
							mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), " frame_size %d\n", c->frame_size );
							
							// Dual pass logging
							if ( mlt_properties_get_data( properties, "_logfile", NULL ) && c->stats_out )
								fprintf( mlt_properties_get_data( properties, "_logfile", NULL ), "%s", c->stats_out );

							error_count = 0;
	 					} 
						else if ( pkt.size < 0 )
						{
							mlt_log_warning( MLT_CONSUMER_SERVICE( consumer ), "error with video encode %d\n", frame_count );
							if ( ++error_count > 2 )
								goto on_fatal_error;
							ret = 0;
						}
 					}
					frame_count++;
#if LIBAVFORMAT_VERSION_INT >= ((55<<16)+(44<<8)+0)
					video_pts = (double) frame_count * av_q2d( video_st->codec->time_base );
#else
					video_pts = (double) frame_count * av_q2d( video_st->time_base );
#endif
					if ( ret )
					{
						mlt_log_fatal( MLT_CONSUMER_SERVICE( consumer ), "error writing video frame\n" );
						mlt_events_fire( properties, "consumer-fatal-error", NULL );
						goto on_fatal_error;
					}
					mlt_frame_close( frame );
					frame = NULL;
				}
				else
				{
					break;
				}
			}
			if ( audio_st[0] )
				mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "audio pts %f ", audio_pts );
			if ( video_st )
				mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "video pts %f ", video_pts );
			mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "\n" );
		}

		if ( real_time_output == 1 && frames % 2 == 0 )
		{
			long passed = time_difference( &ante );
			if ( fifo != NULL )
			{
				long pending = ( ( ( long )sample_fifo_used( fifo ) / sample_bytes * 1000 ) / frequency ) * 1000;
				passed -= pending;
			}
			if ( passed < total_time )
			{
				long total = ( total_time - passed );
				struct timespec t = { total / 1000000, ( total % 1000000 ) * 1000 };
				nanosleep( &t, NULL );
			}
		}
	}

	// Flush the encoder buffers
	if ( real_time_output <= 0 )
	{
		// Flush audio fifo
		// TODO: flush all audio streams
		if ( audio_st[0] && audio_st[0]->codec->frame_size > 1 ) for (;;)
		{
			AVCodecContext *c = audio_st[0]->codec;
			AVPacket pkt;
			av_init_packet( &pkt );
			pkt.data = audio_outbuf;
			pkt.size = 0;

			if ( fifo && sample_fifo_used( fifo ) > 0 )
			{
				// Drain the MLT FIFO
				int samples = FFMIN( FFMIN( channels * audio_input_nb_samples, sample_fifo_used( fifo ) / sample_bytes ), AUDIO_ENCODE_BUFFER_SIZE );
				sample_fifo_fetch( fifo, audio_buf_1, samples * sample_bytes );
				void* p = audio_buf_1;
				if ( c->sample_fmt == AV_SAMPLE_FMT_FLTP )
					p = interleaved_to_planar( audio_input_nb_samples, channels, p, sizeof( float ) );
				else if ( c->sample_fmt == AV_SAMPLE_FMT_S16P )
					p = interleaved_to_planar( audio_input_nb_samples, channels, p, sizeof( int16_t ) );
				else if ( c->sample_fmt == AV_SAMPLE_FMT_S32P )
					p = interleaved_to_planar( audio_input_nb_samples, channels, p, sizeof( int32_t ) );
				else if ( c->sample_fmt == AV_SAMPLE_FMT_U8P )
					p = interleaved_to_planar( audio_input_nb_samples, channels, p, sizeof( uint8_t ) );
				pkt.size = audio_outbuf_size;
				audio_avframe->nb_samples = FFMAX( samples / channels, audio_input_nb_samples );
#if LIBAVCODEC_VERSION_MAJOR >= 55
				audio_avframe->pts = sample_count[0];
				sample_count[0] += audio_avframe->nb_samples;
#endif
				avcodec_fill_audio_frame( audio_avframe, c->channels, c->sample_fmt,
					(const uint8_t*) p, AUDIO_ENCODE_BUFFER_SIZE, 0 );
				int got_packet = 0;
				int ret = avcodec_encode_audio2( c, &pkt, audio_avframe, &got_packet );
				if ( ret < 0 )
					pkt.size = ret;
				else if ( !got_packet )
					pkt.size = 0;
				if ( p != audio_buf_1 )
					mlt_pool_release( p );
				mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "flushing audio size %d\n", pkt.size );
			}
			else
			{
				// Drain the codec
				if ( pkt.size <= 0 ) {
					pkt.size = audio_outbuf_size;
					int got_packet = 0;
					int ret = avcodec_encode_audio2( c, &pkt, NULL, &got_packet );
					if ( ret < 0 )
						pkt.size = ret;
					else if ( !got_packet )
						pkt.size = 0;
				}
				mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "flushing audio size %d\n", pkt.size );
				if ( pkt.size <= 0 )
					break;
			}

			// Write the compressed frame in the media file
			if ( pkt.pts != AV_NOPTS_VALUE )
				pkt.pts = av_rescale_q( pkt.pts, c->time_base, audio_st[0]->time_base );
#if LIBAVCODEC_VERSION_MAJOR >= 55
			if ( pkt.dts != AV_NOPTS_VALUE )
				pkt.dts = av_rescale_q( pkt.dts, c->time_base, audio_st[0]->time_base );
			if ( pkt.duration > 0 )
				pkt.duration = av_rescale_q( pkt.duration, c->time_base, audio_st[0]->time_base );
#endif
			pkt.stream_index = audio_st[0]->index;
			if ( av_interleaved_write_frame( oc, &pkt ) != 0 )
			{
				mlt_log_warning( MLT_CONSUMER_SERVICE( consumer ), "error writing flushed audio frame\n" );
				break;
			}
		}

		// Flush video
		if ( video_st && !( oc->oformat->flags & AVFMT_RAWPICTURE ) ) for (;;)
		{
			AVCodecContext *c = video_st->codec;
			AVPacket pkt;
			av_init_packet( &pkt );
			if ( c->codec->id == AV_CODEC_ID_RAWVIDEO ) {
				pkt.data = NULL;
				pkt.size = 0;
			} else {
				pkt.data = video_outbuf;
				pkt.size = video_outbuf_size;
			}

			// Encode the image
#if LIBAVCODEC_VERSION_MAJOR >= 55
			int got_packet = 0;
			int ret = avcodec_encode_video2( c, &pkt, NULL, &got_packet );
			if ( ret < 0 )
				pkt.size = ret;
			else if ( !got_packet )
				pkt.size = 0;
#else
			pkt.size = avcodec_encode_video( c, video_outbuf, video_outbuf_size, NULL );
			pkt.pts = c->coded_frame? c->coded_frame->pts : AV_NOPTS_VALUE;
			if( c->coded_frame && c->coded_frame->key_frame )
				pkt.flags |= AV_PKT_FLAG_KEY;
#endif
			mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "flushing video size %d\n", pkt.size );
			if ( pkt.size < 0 )
				break;
			// Dual pass logging
			if ( mlt_properties_get_data( properties, "_logfile", NULL ) && c->stats_out )
				fprintf( mlt_properties_get_data( properties, "_logfile", NULL ), "%s", c->stats_out );
			if ( !pkt.size )
				break;

			if ( pkt.pts != AV_NOPTS_VALUE )
				pkt.pts = av_rescale_q( pkt.pts, c->time_base, video_st->time_base );
#if LIBAVCODEC_VERSION_MAJOR >= 55
			if ( pkt.dts != AV_NOPTS_VALUE )
				pkt.dts = av_rescale_q( pkt.dts, c->time_base, video_st->time_base );
#endif
			pkt.stream_index = video_st->index;

			// write the compressed frame in the media file
			if ( av_interleaved_write_frame( oc, &pkt ) != 0 )
			{
				mlt_log_fatal( MLT_CONSUMER_SERVICE(consumer), "error writing flushed video frame\n" );
				mlt_events_fire( properties, "consumer-fatal-error", NULL );
				goto on_fatal_error;
			}
		}
	}

on_fatal_error:

	if ( frame )
		mlt_frame_close( frame );

	// Write the trailer, if any
	if ( frames )
		av_write_trailer( oc );

	// close each codec
	if ( video_st )
		close_video(oc, video_st);
	for ( i = 0; i < MAX_AUDIO_STREAMS && audio_st[i]; i++ )
		close_audio( oc, audio_st[i] );

	// Free the streams
	for ( i = 0; i < oc->nb_streams; i++ )
		av_freep( &oc->streams[i] );

	// Close the output file
	if ( !( fmt->flags & AVFMT_NOFILE ) &&
		!mlt_properties_get_int( properties, "redirect" ) )
	{
		if ( oc->pb  ) avio_close( oc->pb );
	}

	// Clean up input and output frames
	if ( converted_avframe )
		av_free( converted_avframe->data[0] );
	av_free( converted_avframe );
	av_free( video_outbuf );
	av_free( audio_avframe );
	av_free( audio_buf_1 );
	av_free( audio_buf_2 );

	// Free the stream
	av_free( oc );

	// Just in case we terminated on pause
	mlt_consumer_stopped( consumer );
	mlt_properties_close( frame_meta_properties );

	if ( mlt_properties_get_int( properties, "pass" ) > 1 )
	{
		// Remove the dual pass log file
		if ( mlt_properties_get( properties, "_logfilename" ) )
			remove( mlt_properties_get( properties, "_logfilename" ) );

		// Remove the x264 dual pass logs
		char *cwd = getcwd( NULL, 0 );
		const char *file = "x264_2pass.log";
		char *full = malloc( strlen( cwd ) + strlen( file ) + 2 );
		sprintf( full, "%s/%s", cwd, file );
		remove( full );
		free( full );
		file = "x264_2pass.log.temp";
		full = malloc( strlen( cwd ) + strlen( file ) + 2 );
		sprintf( full, "%s/%s", cwd, file );
		remove( full );
		free( full );
		file = "x264_2pass.log.mbtree";
		full = malloc( strlen( cwd ) + strlen( file ) + 2 );
		sprintf( full, "%s/%s", cwd, file );
		remove( full );
		free( full );
		free( cwd );
		remove( "x264_2pass.log.temp" );

		// Recent versions of libavcodec/x264 support passlogfile and need cleanup if specified.
		if ( !mlt_properties_get( properties, "_logfilename" ) &&
		      mlt_properties_get( properties, "passlogfile" ) )
		{
			mlt_properties_get( properties, "passlogfile" );
			mlt_properties_from_utf8( properties, "passlogfile", "_passlogfile" );
			file = mlt_properties_get( properties, "_passlogfile" );
			remove( file );
			full = malloc( strlen( file ) + strlen( ".mbtree" ) + 1 );
			sprintf( full, "%s.mbtree", file );
			remove( full );
			free( full );
		}
	}

	while ( ( frame = mlt_deque_pop_back( queue ) ) )
		mlt_frame_close( frame );

	return NULL;
}

/** Close the consumer.
*/

static void consumer_close( mlt_consumer consumer )
{
	// Stop the consumer
	mlt_consumer_stop( consumer );

	// Close the parent
	mlt_consumer_close( consumer );

	// Free the memory
	free( consumer );
}
