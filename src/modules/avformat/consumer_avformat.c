/*
 * consumer_avformat.c -- an encoder based on avformat
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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
#ifdef SWSCALE
#include <libswscale/swscale.h>
#endif
#if LIBAVUTIL_VERSION_INT >= ((50<<16)+(8<<8)+0)
#include <libavutil/pixdesc.h>
#endif

#if LIBAVUTIL_VERSION_INT < (50<<16)
#define PIX_FMT_RGB32 PIX_FMT_RGBA32
#define PIX_FMT_YUYV422 PIX_FMT_YUV422
#endif

#if LIBAVCODEC_VERSION_MAJOR > 52
#include <libavutil/opt.h>
#define CODEC_TYPE_VIDEO      AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_AUDIO      AVMEDIA_TYPE_AUDIO
#define PKT_FLAG_KEY AV_PKT_FLAG_KEY
#else
#include <libavcodec/opt.h>
#endif

#define MAX_AUDIO_STREAMS (8)
#define AUDIO_ENCODE_BUFFER_SIZE (48000 * 2 * MAX_AUDIO_STREAMS)
#define AUDIO_BUFFER_SIZE (1024 * 42)
#define VIDEO_BUFFER_SIZE (2048 * 1024)

void avformat_lock( );
void avformat_unlock( );

//
// This structure should be extended and made globally available in mlt
//

typedef struct
{
	int16_t *buffer;
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

// sample_fifo_clear and check are temporarily aborted (not working as intended)

void sample_fifo_clear( sample_fifo fifo, double time )
{
	int words = ( float )( time - fifo->time ) * fifo->frequency * fifo->channels;
	if ( ( int )( ( float )time * 100 ) < ( int )( ( float )fifo->time * 100 ) && fifo->used > words && words > 0 )
	{
		memmove( fifo->buffer, &fifo->buffer[ words ], ( fifo->used - words ) * sizeof( int16_t ) );
		fifo->used -= words;
		fifo->time = time;
	}
	else if ( ( int )( ( float )time * 100 ) != ( int )( ( float )fifo->time * 100 ) )
	{
		fifo->used = 0;
		fifo->time = time;
	}
}

void sample_fifo_check( sample_fifo fifo, double time )
{
	if ( fifo->used == 0 )
	{
		if ( ( int )( ( float )time * 100 ) < ( int )( ( float )fifo->time * 100 ) )
			fifo->time = time;
	}
}

void sample_fifo_append( sample_fifo fifo, int16_t *samples, int count )
{
	if ( ( fifo->size - fifo->used ) < count )
	{
		fifo->size += count * 5;
		fifo->buffer = realloc( fifo->buffer, fifo->size * sizeof( int16_t ) );
	}

	memcpy( &fifo->buffer[ fifo->used ], samples, count * sizeof( int16_t ) );
	fifo->used += count;
}

int sample_fifo_used( sample_fifo fifo )
{
	return fifo->used;
}

int sample_fifo_fetch( sample_fifo fifo, int16_t *samples, int count )
{
	if ( count > fifo->used )
		count = fifo->used;

	memcpy( samples, fifo->buffer, count * sizeof( int16_t ) );
	fifo->used -= count;
	memmove( fifo->buffer, &fifo->buffer[ count ], fifo->used * sizeof( int16_t ) );

	fifo->time += ( double )count / fifo->channels / fifo->frequency;

	return count;
}

void sample_fifo_close( sample_fifo fifo )
{
	free( fifo->buffer );
	free( fifo );
}

// Forward references.
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
	}

	// Return consumer
	return consumer;
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
		fprintf( stderr, "%s", mlt_properties_serialise_yaml( doc ) );
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
			if ( codec->encode && codec->type == CODEC_TYPE_AUDIO )
			{
				snprintf( key, sizeof(key), "%d", mlt_properties_count( codecs ) );
				mlt_properties_set( codecs, key, codec->name );
			}
		fprintf( stderr, "%s", mlt_properties_serialise_yaml( doc ) );
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
			if ( codec->encode && codec->type == CODEC_TYPE_VIDEO )
			{
				snprintf( key, sizeof(key), "%d", mlt_properties_count( codecs ) );
				mlt_properties_set( codecs, key, codec->name );
			}
		fprintf( stderr, "%s", mlt_properties_serialise_yaml( doc ) );
		mlt_properties_close( doc );
		error = 1;
	}

	// Check that we're not already running
	if ( !error && !mlt_properties_get_int( properties, "running" ) )
	{
		// Allocate a thread
		pthread_t *thread = calloc( 1, sizeof( pthread_t ) );

		// Get the width and height
		int width = mlt_properties_get_int( properties, "width" );
		int height = mlt_properties_get_int( properties, "height" );

		// Obtain the size property
		char *size = mlt_properties_get( properties, "s" );

		// Interpret it
		if ( size != NULL )
		{
			int tw, th;
			if ( sscanf( size, "%dx%d", &tw, &th ) == 2 && tw > 0 && th > 0 )
			{
				width = tw;
				height = th;
			}
			else
			{
				mlt_log_warning( MLT_CONSUMER_SERVICE( consumer ), "Invalid size property %s - ignoring.\n", size );
			}
		}
		
		// Now ensure we honour the multiple of two requested by libavformat
		width = ( width / 2 ) * 2;
		height = ( height / 2 ) * 2;
		mlt_properties_set_int( properties, "width", width );
		mlt_properties_set_int( properties, "height", height );

		// We need to set these on the profile as well because the s property is
		// an alias to mlt properties that correspond to profile settings.
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( consumer ) );
		if ( profile )
		{
			profile->width = width;
			profile->height = height;
		}

		// Handle the ffmpeg command line "-r" property for frame rate
		if ( mlt_properties_get( properties, "r" ) )
		{
			double frame_rate = mlt_properties_get_double( properties, "r" );
			AVRational rational = av_d2q( frame_rate, 255 );
			mlt_properties_set_int( properties, "frame_rate_num", rational.num );
			mlt_properties_set_int( properties, "frame_rate_den", rational.den );
			if ( profile )
			{
				profile->frame_rate_num = rational.num;
				profile->frame_rate_den = rational.den;
				mlt_properties_set_double( properties, "fps", mlt_profile_fps( profile ) );
			}
		}
		
		// Apply AVOptions that are synonyms for standard mlt_consumer options
		if ( mlt_properties_get( properties, "ac" ) )
			mlt_properties_set_int( properties, "channels", mlt_properties_get_int( properties, "ac" ) );
		if ( mlt_properties_get( properties, "ar" ) )
			mlt_properties_set_int( properties, "frequency", mlt_properties_get_int( properties, "ar" ) );

		// Assign the thread to properties
		mlt_properties_set_data( properties, "thread", thread, sizeof( pthread_t ), free, NULL );

		// Set the running state
		mlt_properties_set_int( properties, "running", 1 );

		// Create the thread
		pthread_create( thread, NULL, consumer_thread, consumer );
	}
	return error;
}

/** Stop the consumer.
*/

static int consumer_stop( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Check that we're running
	if ( mlt_properties_get_int( properties, "running" ) )
	{
		// Get the thread
		pthread_t *thread = mlt_properties_get_data( properties, "thread", NULL );

		// Stop the thread
		mlt_properties_set_int( properties, "running", 0 );

		// Wait for termination
		pthread_join( *thread, NULL );
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

static void apply_properties( void *obj, mlt_properties properties, int flags, int alloc )
{
	int i;
	int count = mlt_properties_count( properties );
	for ( i = 0; i < count; i++ )
	{
		const char *opt_name = mlt_properties_get_name( properties, i );
		const AVOption *opt = av_find_opt( obj, opt_name, NULL, flags, flags );

		if ( opt != NULL )
#if LIBAVCODEC_VERSION_INT >= ((52<<16)+(7<<8)+0)
			av_set_string3( obj, opt_name, mlt_properties_get_value( properties, i), alloc, NULL );
#elif LIBAVCODEC_VERSION_INT >= ((51<<16)+(59<<8)+0)
			av_set_string2( obj, opt_name, mlt_properties_get_value( properties, i), alloc );
#else
			av_set_string( obj, opt_name, mlt_properties_get_value( properties, i) );
#endif
	}
}

/** Add an audio output stream
*/

static AVStream *add_audio_stream( mlt_consumer consumer, AVFormatContext *oc, int codec_id, int channels )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Create a new stream
	AVStream *st = av_new_stream( oc, oc->nb_streams );

	// If created, then initialise from properties
	if ( st != NULL ) 
	{
		AVCodecContext *c = st->codec;

		// Establish defaults from AVOptions
		avcodec_get_context_defaults2( c, CODEC_TYPE_AUDIO );

		c->codec_id = codec_id;
		c->codec_type = CODEC_TYPE_AUDIO;
		c->sample_fmt = SAMPLE_FMT_S16;

#if 0 // disabled until some audio codecs are multi-threaded
		// Setup multi-threading
		int thread_count = mlt_properties_get_int( properties, "threads" );
		if ( thread_count == 0 && getenv( "MLT_AVFORMAT_THREADS" ) )
			thread_count = atoi( getenv( "MLT_AVFORMAT_THREADS" ) );
		if ( thread_count > 1 )
			c->thread_count = thread_count;
#endif
	
		if (oc->oformat->flags & AVFMT_GLOBALHEADER) 
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
		
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
			apply_properties( c, p, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM, 1 );
			mlt_properties_close( p );
		}
		apply_properties( c, properties, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM, 0 );

		int audio_qscale = mlt_properties_get_int( properties, "aq" );
        if ( audio_qscale > QSCALE_NONE )
		{
			c->flags |= CODEC_FLAG_QSCALE;
			c->global_quality = st->quality = FF_QP2LAMBDA * audio_qscale;
		}

		// Set parameters controlled by MLT
		c->sample_rate = mlt_properties_get_int( properties, "frequency" );
		c->time_base = ( AVRational ){ 1, c->sample_rate };
		c->channels = channels;

		if ( mlt_properties_get( properties, "alang" ) != NULL )
#if LIBAVFORMAT_VERSION_INT >= ((52<<16)+(43<<8)+0)
			av_metadata_set2( &oc->metadata, "language", mlt_properties_get( properties, "alang" ), 0 );
#else

			strncpy( st->language, mlt_properties_get( properties, "alang" ), sizeof( st->language ) );
#endif
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

#if LIBAVCODEC_VERSION_MAJOR > 52
	// Process properties as AVOptions on the AVCodec
	if ( codec && codec->priv_class && c->priv_data )
	{
		char *apre = mlt_properties_get( properties, "apre" );
		if ( apre )
		{
			mlt_properties p = mlt_properties_load( apre );
			apply_properties( c->priv_data, p, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM, 1 );
			mlt_properties_close( p );
		}
		apply_properties( c->priv_data, properties, AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_ENCODING_PARAM, 0 );
	}
#endif

	avformat_lock();
	
	// Continue if codec found and we can open it
	if ( codec != NULL && avcodec_open( c, codec ) >= 0 )
	{
		// ugly hack for PCM codecs (will be removed ASAP with new PCM
		// support to compute the input frame size in samples
		if ( c->frame_size <= 1 ) 
		{
			audio_input_frame_size = audio_outbuf_size / c->channels;
			switch(st->codec->codec_id) 
			{
				case CODEC_ID_PCM_S16LE:
				case CODEC_ID_PCM_S16BE:
				case CODEC_ID_PCM_U16LE:
				case CODEC_ID_PCM_U16BE:
					audio_input_frame_size >>= 1;
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
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	else
	{
		mlt_log_warning( NULL, "%s: Unable to encode audio - disabling audio output.\n", __FILE__ );
	}
	
	avformat_unlock();

	return audio_input_frame_size;
}

static void close_audio( AVFormatContext *oc, AVStream *st )
{
	if ( st && st->codec )
	{
		avformat_lock();
		avcodec_close( st->codec );
		avformat_unlock();
	}
}

/** Add a video output stream 
*/

static AVStream *add_video_stream( mlt_consumer consumer, AVFormatContext *oc, int codec_id )
{
 	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Create a new stream
	AVStream *st = av_new_stream( oc, oc->nb_streams );

	if ( st != NULL ) 
	{
		char *pix_fmt = mlt_properties_get( properties, "pix_fmt" );
		AVCodecContext *c = st->codec;

		// Establish defaults from AVOptions
		avcodec_get_context_defaults2( c, CODEC_TYPE_VIDEO );

		c->codec_id = codec_id;
		c->codec_type = CODEC_TYPE_VIDEO;
		
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
					mlt_properties_debug( p, path, stderr );
					free( path );	
				}
			}
			else
			{
				mlt_properties_debug( p, vpre, stderr );			
			}
#endif
			apply_properties( c, p, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM, 1 );
			mlt_properties_close( p );
		}
		int colorspace = mlt_properties_get_int( properties, "colorspace" );
		mlt_properties_set( properties, "colorspace", NULL );
		apply_properties( c, properties, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM, 0 );
		mlt_properties_set_int( properties, "colorspace", colorspace );

		// Set options controlled by MLT
		c->width = mlt_properties_get_int( properties, "width" );
		c->height = mlt_properties_get_int( properties, "height" );
		c->time_base.num = mlt_properties_get_int( properties, "frame_rate_den" );
		c->time_base.den = mlt_properties_get_int( properties, "frame_rate_num" );
		if ( st->time_base.den == 0 )
			st->time_base = c->time_base;
#if LIBAVUTIL_VERSION_INT >= ((50<<16)+(8<<8)+0)
		c->pix_fmt = pix_fmt ? av_get_pix_fmt( pix_fmt ) : PIX_FMT_YUV420P;
#else
		c->pix_fmt = pix_fmt ? avcodec_get_pix_fmt( pix_fmt ) : PIX_FMT_YUV420P;
#endif
		
#if LIBAVCODEC_VERSION_INT > ((52<<16)+(28<<8)+0)
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
#endif

		if ( mlt_properties_get( properties, "aspect" ) )
		{
			// "-aspect" on ffmpeg command line is display aspect ratio
			double ar = mlt_properties_get_double( properties, "aspect" );
			AVRational rational = av_d2q( ar, 255 );

			// Update the profile and properties as well since this is an alias 
			// for mlt properties that correspond to profile settings
			mlt_properties_set_int( properties, "display_aspect_num", rational.num );
			mlt_properties_set_int( properties, "display_aspect_den", rational.den );
			mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( consumer ) );
			if ( profile )
			{
				profile->display_aspect_num = rational.num;
				profile->display_aspect_den = rational.den;
				mlt_properties_set_double( properties, "display_ratio", mlt_profile_dar( profile ) );
			}

			// Now compute the sample aspect ratio
			rational = av_d2q( ar * c->height / c->width, 255 );
			c->sample_aspect_ratio = rational;
			// Update the profile and properties as well since this is an alias 
			// for mlt properties that correspond to profile settings
			mlt_properties_set_int( properties, "sample_aspect_num", rational.num );
			mlt_properties_set_int( properties, "sample_aspect_den", rational.den );
			if ( profile )
			{
				profile->sample_aspect_num = rational.num;
				profile->sample_aspect_den = rational.den;
				mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile ) );
			}
		}
		else
		{
			c->sample_aspect_ratio.num = mlt_properties_get_int( properties, "sample_aspect_num" );
			c->sample_aspect_ratio.den = mlt_properties_get_int( properties, "sample_aspect_den" );
		}
#if LIBAVFORMAT_VERSION_INT >= ((52<<16)+(21<<8)+0)
		st->sample_aspect_ratio = c->sample_aspect_ratio;
#endif

		if ( mlt_properties_get_double( properties, "qscale" ) > 0 )
		{
			c->flags |= CODEC_FLAG_QSCALE;
			st->quality = FF_QP2LAMBDA * mlt_properties_get_double( properties, "qscale" );
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
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;

		// Translate these standard mlt consumer properties to ffmpeg
		if ( mlt_properties_get_int( properties, "progressive" ) == 0 &&
		     mlt_properties_get_int( properties, "deinterlace" ) == 0 )
		{
			if ( ! mlt_properties_get( properties, "ildct" ) || mlt_properties_get_int( properties, "ildct" ) )
				c->flags |= CODEC_FLAG_INTERLACED_DCT;
			if ( ! mlt_properties_get( properties, "ilme" ) || mlt_properties_get_int( properties, "ilme" ) )
				c->flags |= CODEC_FLAG_INTERLACED_ME;
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
			c->flags |= CODEC_FLAG_PASS1;
		else if ( i == 2 )
			c->flags |= CODEC_FLAG_PASS2;
		if ( codec_id != CODEC_ID_H264 && ( c->flags & ( CODEC_FLAG_PASS1 | CODEC_FLAG_PASS2 ) ) )
		{
			char logfilename[1024];
			FILE *f;
			int size;
			char *logbuffer;

			snprintf( logfilename, sizeof(logfilename), "%s_2pass.log",
				mlt_properties_get( properties, "passlogfile" ) ? mlt_properties_get( properties, "passlogfile" ) : mlt_properties_get( properties, "target" ) );
			if ( c->flags & CODEC_FLAG_PASS1 )
			{
				f = fopen( logfilename, "w" );
				if ( !f )
					perror( logfilename );
				else
					mlt_properties_set_data( properties, "_logfile", f, 0, ( mlt_destructor )fclose, NULL );
			}
			else
			{
				/* read the log file */
				f = fopen( logfilename, "r" );
				if ( !f )
				{
					perror(logfilename);
				}
				else
				{
					mlt_properties_set( properties, "_logfilename", logfilename );
					fseek( f, 0, SEEK_END );
					size = ftell( f );
					fseek( f, 0, SEEK_SET );
					logbuffer = av_malloc( size + 1 );
					if ( !logbuffer )
						mlt_log_fatal( MLT_CONSUMER_SERVICE( consumer ), "Could not allocate log buffer\n" );
					else
					{
						size = fread( logbuffer, 1, size, f );
						fclose( f );
						logbuffer[size] = '\0';
						c->stats_in = logbuffer;
						mlt_properties_set_data( properties, "_logbuffer", logbuffer, 0, ( mlt_destructor )av_free, NULL );
					}
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
	AVFrame *picture = avcodec_alloc_frame();

	// Determine size of the 
	int size = avpicture_get_size(pix_fmt, width, height);

	// Allocate the picture buf
	uint8_t *picture_buf = av_malloc(size);

	// If we have both, then fill the image
	if ( picture != NULL && picture_buf != NULL )
	{
		// Fill the frame with the allocated buffer
		avpicture_fill( (AVPicture *)picture, picture_buf, pix_fmt, width, height);
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

#if LIBAVCODEC_VERSION_MAJOR > 52
	// Process properties as AVOptions on the AVCodec
	if ( codec && codec->priv_class && video_enc->priv_data )
	{
		char *vpre = mlt_properties_get( properties, "vpre" );
		if ( vpre )
		{
			mlt_properties p = mlt_properties_load( vpre );
			apply_properties( video_enc->priv_data, p, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM, 1 );
			mlt_properties_close( p );
		}
		apply_properties( video_enc->priv_data, properties, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM, 0 );
	}
#endif

	if( codec && codec->pix_fmts )
	{
		const enum PixelFormat *p = codec->pix_fmts;
		for( ; *p!=-1; p++ )
		{
			if( *p == video_enc->pix_fmt )
				break;
		}
		if( *p == -1 )
			video_enc->pix_fmt = codec->pix_fmts[ 0 ];
	}

	// Open the codec safely
	avformat_lock();
	int result = codec != NULL && avcodec_open( video_enc, codec ) >= 0;
	avformat_unlock();
	
	return result;
}

void close_video(AVFormatContext *oc, AVStream *st)
{
	if ( st && st->codec )
	{
		avformat_lock();
		avcodec_close(st->codec);
		avformat_unlock();
	}
}

static inline long time_difference( struct timeval *time1 )
{
	struct timeval time2;
	gettimeofday( &time2, NULL );
	return time2.tv_sec * 1000000 + time2.tv_usec - time1->tv_sec * 1000000 - time1->tv_usec;
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
	mlt_audio_format aud_fmt = mlt_audio_s16;
	int channels = mlt_properties_get_int( properties, "channels" );
	int total_channels = channels;
	int frequency = mlt_properties_get_int( properties, "frequency" );
	int16_t *pcm = NULL;
	int samples = 0;

	// AVFormat audio buffer and frame size
	int audio_outbuf_size = AUDIO_BUFFER_SIZE;
	uint8_t *audio_outbuf = av_malloc( audio_outbuf_size );
	int audio_input_frame_size = 0;

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

	// Need two av pictures for converting
	AVFrame *output = NULL;
	AVFrame *input = alloc_picture( PIX_FMT_YUYV422, width, height );

	// For receiving images from an mlt_frame
	uint8_t *image;
	mlt_image_format img_fmt = mlt_image_yuv422;

	// For receiving audio samples back from the fifo
	int16_t *audio_buf_1 = av_malloc( AUDIO_ENCODE_BUFFER_SIZE );
	int16_t *audio_buf_2 = NULL;
	int count = 0;

	// Allocate the context
#if (LIBAVFORMAT_VERSION_INT >= ((52<<16)+(26<<8)+0))
	AVFormatContext *oc = avformat_alloc_context( );
#else
	AVFormatContext *oc = av_alloc_format_context( );
#endif

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
	const char *filename = mlt_properties_get( properties, "target" );
	char *format = mlt_properties_get( properties, "f" );
	char *vcodec = mlt_properties_get( properties, "vcodec" );
	char *acodec = mlt_properties_get( properties, "acodec" );
	
	// Used to store and override codec ids
	int audio_codec_id;
	int video_codec_id;

	// Misc
	char key[27];
	mlt_properties frame_meta_properties = mlt_properties_new();

	// Initialize audio_st
	int i = MAX_AUDIO_STREAMS;
	while ( i-- )
		audio_st[i] = NULL;

	// Check for user selected format first
	if ( format != NULL )
#if LIBAVFORMAT_VERSION_INT < ((52<<16)+(45<<8)+0)
		fmt = guess_format( format, NULL, NULL );
#else
		fmt = av_guess_format( format, NULL, NULL );
#endif

	// Otherwise check on the filename
	if ( fmt == NULL && filename != NULL )
#if LIBAVFORMAT_VERSION_INT < ((52<<16)+(45<<8)+0)
		fmt = guess_format( NULL, filename, NULL );
#else
		fmt = av_guess_format( NULL, filename, NULL );
#endif

	// Otherwise default to mpeg
	if ( fmt == NULL )
#if LIBAVFORMAT_VERSION_INT < ((52<<16)+(45<<8)+0)
		fmt = guess_format( "mpeg", NULL, NULL );
#else
		fmt = av_guess_format( "mpeg", NULL, NULL );
#endif

	// We need a filename - default to stdout?
	if ( filename == NULL || !strcmp( filename, "" ) )
		filename = "pipe:";

	// Get the codec ids selected
	audio_codec_id = fmt->audio_codec;
	video_codec_id = fmt->video_codec;

	// Check for audio codec overides
	if ( ( acodec && strcmp( acodec, "none" ) == 0 ) || mlt_properties_get_int( properties, "an" ) )
		audio_codec_id = CODEC_ID_NONE;
	else if ( acodec )
	{
		AVCodec *p = avcodec_find_encoder_by_name( acodec );
		if ( p != NULL )
		{
			audio_codec_id = p->id;
			if ( audio_codec_id == CODEC_ID_AC3 && avcodec_find_encoder_by_name( "ac3_fixed" ) )
			{
				mlt_properties_set( properties, "_acodec", "ac3_fixed" );
				acodec = mlt_properties_get( properties, "_acodec" );
			}
		}
		else
		{
			audio_codec_id = CODEC_ID_NONE;
			mlt_log_warning( MLT_CONSUMER_SERVICE( consumer ), "audio codec %s unrecognised - ignoring\n", acodec );
		}
	}

	// Check for video codec overides
	if ( ( vcodec && strcmp( vcodec, "none" ) == 0 ) || mlt_properties_get_int( properties, "vn" ) )
		video_codec_id = CODEC_ID_NONE;
	else if ( vcodec )
	{
		AVCodec *p = avcodec_find_encoder_by_name( vcodec );
		if ( p != NULL )
		{
			video_codec_id = p->id;
		}
		else
		{
			video_codec_id = CODEC_ID_NONE;
			mlt_log_warning( MLT_CONSUMER_SERVICE( consumer ), "video codec %s unrecognised - ignoring\n", vcodec );
		}
	}

	// Write metadata
#if LIBAVFORMAT_VERSION_INT >= ((52<<16)+(31<<8)+0)
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
#if LIBAVFORMAT_VERSION_INT >= ((52<<16)+(43<<8)+0)
					av_metadata_set2( &oc->metadata, key, mlt_properties_get_value( properties, i ), 0 );
#else
					av_metadata_set( &oc->metadata, key, mlt_properties_get_value( properties, i ) );
#endif
			}
			free( key );
		}
	}
#else
	char *tmp = NULL;
	int metavalue;

	tmp = mlt_properties_get( properties, "meta.attr.title.markup");
	if (tmp != NULL) snprintf( oc->title, sizeof(oc->title), "%s", tmp );

	tmp = mlt_properties_get( properties, "meta.attr.comment.markup");
	if (tmp != NULL) snprintf( oc->comment, sizeof(oc->comment), "%s", tmp );

	tmp = mlt_properties_get( properties, "meta.attr.author.markup");
	if (tmp != NULL) snprintf( oc->author, sizeof(oc->author), "%s", tmp );

	tmp = mlt_properties_get( properties, "meta.attr.copyright.markup");
	if (tmp != NULL) snprintf( oc->copyright, sizeof(oc->copyright), "%s", tmp );

	tmp = mlt_properties_get( properties, "meta.attr.album.markup");
	if (tmp != NULL) snprintf( oc->album, sizeof(oc->album), "%s", tmp );

	metavalue = mlt_properties_get_int( properties, "meta.attr.year.markup");
	if (metavalue != 0) oc->year = metavalue;

	metavalue = mlt_properties_get_int( properties, "meta.attr.track.markup");
	if (metavalue != 0) oc->track = metavalue;
#endif

	oc->oformat = fmt;
	snprintf( oc->filename, sizeof(oc->filename), "%s", filename );

	// Add audio and video streams
	if ( video_codec_id != CODEC_ID_NONE )
		video_st = add_video_stream( consumer, oc, video_codec_id );
	if ( audio_codec_id != CODEC_ID_NONE )
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
				audio_st[i] = add_audio_stream( consumer, oc, audio_codec_id, j );
			}
		}
		// single track
		if ( !is_multi )
		{
			audio_st[0] = add_audio_stream( consumer, oc, audio_codec_id, channels );
			total_channels = channels;
		}
	}

	// Set the parameters (even though we have none...)
	if ( av_set_parameters(oc, NULL) >= 0 ) 
	{
		oc->preload = ( int )( mlt_properties_get_double( properties, "muxpreload" ) * AV_TIME_BASE );
		oc->max_delay= ( int )( mlt_properties_get_double( properties, "muxdelay" ) * AV_TIME_BASE );

		// Process properties as AVOptions
		char *fpre = mlt_properties_get( properties, "fpre" );
		if ( fpre )
		{
			mlt_properties p = mlt_properties_load( fpre );
			apply_properties( oc, p, AV_OPT_FLAG_ENCODING_PARAM, 1 );
#if LIBAVFORMAT_VERSION_MAJOR > 52
			if ( oc->oformat && oc->oformat->priv_class && oc->priv_data )
				apply_properties( oc->priv_data, p, AV_OPT_FLAG_ENCODING_PARAM, 1 );
#endif
			mlt_properties_close( p );
		}
		apply_properties( oc, properties, AV_OPT_FLAG_ENCODING_PARAM, 0 );
#if LIBAVFORMAT_VERSION_MAJOR > 52
		if ( oc->oformat && oc->oformat->priv_class && oc->priv_data )
			apply_properties( oc->priv_data, properties, AV_OPT_FLAG_ENCODING_PARAM, 1 );
#endif

		if ( video_st && !open_video( properties, oc, video_st, vcodec? vcodec : NULL ) )
			video_st = NULL;
		for ( i = 0; i < MAX_AUDIO_STREAMS && audio_st[i]; i++ )
		{
			audio_input_frame_size = open_audio( properties, oc, audio_st[i], audio_outbuf_size,
				acodec? acodec : NULL );
			if ( !audio_input_frame_size )
				audio_st[i] = NULL;
		}

		// Open the output file, if needed
		if ( !( fmt->flags & AVFMT_NOFILE ) ) 
		{
#if LIBAVFORMAT_VERSION_MAJOR > 52
			if ( avio_open( &oc->pb, filename, AVIO_FLAG_WRITE ) < 0 )
#else
			if ( url_fopen( &oc->pb, filename, URL_WRONLY ) < 0 )
#endif
			{
				mlt_log_error( MLT_CONSUMER_SERVICE( consumer ), "Could not open '%s'\n", filename );
				mlt_properties_set_int( properties, "running", 0 );
			}
		}
	
		// Write the stream header.
		if ( mlt_properties_get_int( properties, "running" ) )
			av_write_header( oc );
	}
	else
	{
		mlt_log_error( MLT_CONSUMER_SERVICE( consumer ), "Invalid output format parameters\n" );
		mlt_properties_set_int( properties, "running", 0 );
	}

	// Allocate picture
	if ( video_st )
		output = alloc_picture( video_st->codec->pix_fmt, width, height );

	// Last check - need at least one stream
	if ( !audio_st[0] && !video_st )
		mlt_properties_set_int( properties, "running", 0 );

	// Get the starting time (can ignore the times above)
	gettimeofday( &ante, NULL );

	// Loop while running
	while( mlt_properties_get_int( properties, "running" ) &&
	       ( !terminated || ( video_st && mlt_deque_count( queue ) ) ) )
	{
		frame = mlt_consumer_rt_frame( consumer );

		// Check that we have a frame to work with
		if ( frame != NULL )
		{
			// Increment frames dispatched
			frames ++;

			// Default audio args
			frame_properties = MLT_FRAME_PROPERTIES( frame );

			// Check for the terminated condition
			terminated = terminate_on_pause && mlt_properties_get_double( frame_properties, "_speed" ) == 0.0;

			// Get audio and append to the fifo
			if ( !terminated && audio_st[0] )
			{
				samples = mlt_sample_calculator( fps, frequency, count ++ );
				mlt_frame_get_audio( frame, (void**) &pcm, &aud_fmt, &frequency, &channels, &samples );

				// Save the audio channel remap properties for later
				mlt_properties_pass( frame_meta_properties, frame_properties, "meta.map.audio." );

				// Create the fifo if we don't have one
				if ( fifo == NULL )
				{
					fifo = sample_fifo_init( frequency, channels );
					mlt_properties_set_data( properties, "sample_fifo", fifo, 0, ( mlt_destructor )sample_fifo_close, NULL );
				}

				// Silence if not normal forward speed
				if ( mlt_properties_get_double( frame_properties, "_speed" ) != 1.0 )
					memset( pcm, 0, samples * channels * 2 );

				// Append the samples
				sample_fifo_append( fifo, pcm, samples * channels );
				total_time += ( samples * 1000000 ) / frequency;

				if ( !video_st )
					mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
			}

			// Encode the image
			if ( !terminated && video_st )
				mlt_deque_push_back( queue, frame );
			else
				mlt_frame_close( frame );
		}

		// While we have stuff to process, process...
		while ( 1 )
		{
			// Write interleaved audio and video frames
			if ( !video_st || ( video_st && audio_st[0] && audio_pts < video_pts ) )
			{
				// Write audio
				if ( ( video_st && terminated ) || ( channels * audio_input_frame_size ) < sample_fifo_used( fifo ) )
				{
					int j = 0; // channel offset into interleaved source buffer
					int n = FFMIN( FFMIN( channels * audio_input_frame_size, sample_fifo_used( fifo ) ), AUDIO_ENCODE_BUFFER_SIZE );

					// Get the audio samples
					if ( n > 0 )
					{
						sample_fifo_fetch( fifo, audio_buf_1, n );
					}
					else if ( audio_codec_id == CODEC_ID_VORBIS && terminated )
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

						// Optimized for single track and no channel remap
						if ( !audio_st[1] && !mlt_properties_count( frame_meta_properties ) )
						{
							pkt.size = avcodec_encode_audio( codec, audio_outbuf, audio_outbuf_size, audio_buf_1 );
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
										int16_t *src = audio_buf_1 + source_offset;
										int16_t *dest = audio_buf_2 + dest_offset;
										int s = samples + 1;

										while ( --s ) {
											*dest = *src;
											dest += current_channels;
											src += channels;
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
							pkt.size = avcodec_encode_audio( codec, audio_outbuf, audio_outbuf_size, audio_buf_2 );
						}

						// Write the compressed frame in the media file
						if ( codec->coded_frame && codec->coded_frame->pts != AV_NOPTS_VALUE )
						{
							pkt.pts = av_rescale_q( codec->coded_frame->pts, codec->time_base, stream->time_base );
							mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "audio stream %d pkt pts %"PRId64" frame pts %"PRId64,
								stream->index, pkt.pts, codec->coded_frame->pts );
						}
						pkt.flags |= PKT_FLAG_KEY;
						pkt.stream_index = stream->index;
						pkt.data = audio_outbuf;

						if ( pkt.size > 0 )
						{
							if ( av_interleaved_write_frame( oc, &pkt ) )
							{
								mlt_log_fatal( MLT_CONSUMER_SERVICE( consumer ), "error writing audio frame\n" );
								mlt_events_fire( properties, "consumer-fatal-error", NULL );
								goto on_fatal_error;
							}
						}

						mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), " frame_size %d\n", codec->frame_size );
						if ( i == 0 )
						{
							audio_pts = (double)stream->pts.val * av_q2d( stream->time_base );
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
					int out_size, ret = 0;
					AVCodecContext *c;

					frame = mlt_deque_pop_front( queue );
					frame_properties = MLT_FRAME_PROPERTIES( frame );

					c = video_st->codec;
					
					if ( mlt_properties_get_int( frame_properties, "rendered" ) )
					{
						int i = 0;
						uint8_t *p;
						uint8_t *q;

						mlt_frame_get_image( frame, &image, &img_fmt, &img_width, &img_height, 0 );

						q = image;

						// Convert the mlt frame to an AVPicture
						for ( i = 0; i < height; i ++ )
						{
							p = input->data[ 0 ] + i * input->linesize[ 0 ];
							memcpy( p, q, width * 2 );
							q += width * 2;
						}

						// Do the colour space conversion
#ifdef SWSCALE
						int flags = SWS_BILINEAR;
#ifdef USE_MMX
						flags |= SWS_CPU_CAPS_MMX;
#endif
#ifdef USE_SSE
						flags |= SWS_CPU_CAPS_MMX2;
#endif
						struct SwsContext *context = sws_getContext( width, height, PIX_FMT_YUYV422,
							width, height, video_st->codec->pix_fmt, flags, NULL, NULL, NULL);
						sws_scale( context, (const uint8_t* const*) input->data, input->linesize, 0, height,
							output->data, output->linesize);
						sws_freeContext( context );
#else
						img_convert( ( AVPicture * )output, video_st->codec->pix_fmt, ( AVPicture * )input, PIX_FMT_YUYV422, width, height );
#endif

						mlt_events_fire( properties, "consumer-frame-show", frame, NULL );

						// Apply the alpha if applicable
						if ( video_st->codec->pix_fmt == PIX_FMT_RGB32 )
						{
							uint8_t *alpha = mlt_frame_get_alpha_mask( frame );
							register int n;

							for ( i = 0; i < height; i ++ )
							{
								n = ( width + 7 ) / 8;
								p = output->data[ 0 ] + i * output->linesize[ 0 ] + 3;

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

						pkt.flags |= PKT_FLAG_KEY;
						pkt.stream_index= video_st->index;
						pkt.data= (uint8_t *)output;
						pkt.size= sizeof(AVPicture);

						ret = av_write_frame(oc, &pkt);
						video_pts += c->frame_size;
					} 
					else 
					{
						// Set the quality
						output->quality = video_st->quality;

						// Set frame interlace hints
						output->interlaced_frame = !mlt_properties_get_int( frame_properties, "progressive" );
						output->top_field_first = mlt_properties_get_int( frame_properties, "top_field_first" );
						output->pts = frame_count;

	 					// Encode the image
	 					out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, output );

	 					// If zero size, it means the image was buffered
						if ( out_size > 0 )
						{
							AVPacket pkt;
							av_init_packet( &pkt );

							if ( c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE )
								pkt.pts= av_rescale_q( c->coded_frame->pts, c->time_base, video_st->time_base );
							mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "video pkt pts %"PRId64" frame pts %"PRId64, pkt.pts, c->coded_frame->pts );
							if( c->coded_frame && c->coded_frame->key_frame )
								pkt.flags |= PKT_FLAG_KEY;
							pkt.stream_index= video_st->index;
							pkt.data= video_outbuf;
							pkt.size= out_size;

							// write the compressed frame in the media file
							ret = av_interleaved_write_frame(oc, &pkt);
							mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), " frame_size %d\n", c->frame_size );
							video_pts = (double)video_st->pts.val * av_q2d( video_st->time_base );
							
							// Dual pass logging
							if ( mlt_properties_get_data( properties, "_logfile", NULL ) && c->stats_out )
								fprintf( mlt_properties_get_data( properties, "_logfile", NULL ), "%s", c->stats_out );
	 					} 
						else if ( out_size < 0 )
						{
							mlt_log_warning( MLT_CONSUMER_SERVICE( consumer ), "error with video encode %d\n", frame_count );
						}
 					}
 					frame_count++;
					if ( ret )
					{
						mlt_log_fatal( MLT_CONSUMER_SERVICE( consumer ), "error writing video frame\n" );
						mlt_events_fire( properties, "consumer-fatal-error", NULL );
						goto on_fatal_error;
					}
					mlt_frame_close( frame );
				}
				else
				{
					break;
				}
			}
			if ( audio_st[0] )
				mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "audio pts %"PRId64" (%f) ", audio_st[0]->pts.val, audio_pts );
			if ( video_st )
				mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "video pts %"PRId64" (%f) ", video_st->pts.val, video_pts );
			mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "\n" );
		}

		if ( real_time_output == 1 && frames % 2 == 0 )
		{
			long passed = time_difference( &ante );
			if ( fifo != NULL )
			{
				long pending = ( ( ( long )sample_fifo_used( fifo ) * 1000 ) / frequency ) * 1000;
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
			pkt.size = 0;

			if ( /*( c->capabilities & CODEC_CAP_SMALL_LAST_FRAME ) &&*/
				( channels * audio_input_frame_size < sample_fifo_used( fifo ) ) )
			{
				sample_fifo_fetch( fifo, audio_buf_1, channels * audio_input_frame_size );
				pkt.size = avcodec_encode_audio( c, audio_outbuf, audio_outbuf_size, audio_buf_1 );
			}
			if ( pkt.size <= 0 )
				pkt.size = avcodec_encode_audio( c, audio_outbuf, audio_outbuf_size, NULL );
			mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "flushing audio size %d\n", pkt.size );
			if ( pkt.size <= 0 )
				break;

			// Write the compressed frame in the media file
			if ( c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE )
				pkt.pts = av_rescale_q( c->coded_frame->pts, c->time_base, audio_st[0]->time_base );
			pkt.flags |= PKT_FLAG_KEY;
			pkt.stream_index = audio_st[0]->index;
			pkt.data = audio_outbuf;
			if ( av_interleaved_write_frame( oc, &pkt ) != 0 )
			{
				mlt_log_fatal( MLT_CONSUMER_SERVICE( consumer ), "error writing flushed audio frame\n" );
				mlt_events_fire( properties, "consumer-fatal-error", NULL );
				goto on_fatal_error;
			}
		}

		// Flush video
		if ( video_st && !( oc->oformat->flags & AVFMT_RAWPICTURE ) ) for (;;)
		{
			AVCodecContext *c = video_st->codec;
			AVPacket pkt;
			av_init_packet( &pkt );

			// Encode the image
			pkt.size = avcodec_encode_video( c, video_outbuf, video_outbuf_size, NULL );
			mlt_log_debug( MLT_CONSUMER_SERVICE( consumer ), "flushing video size %d\n", pkt.size );
			if ( pkt.size <= 0 )
				break;

			if ( c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE )
				pkt.pts= av_rescale_q( c->coded_frame->pts, c->time_base, video_st->time_base );
			if( c->coded_frame && c->coded_frame->key_frame )
				pkt.flags |= PKT_FLAG_KEY;
			pkt.stream_index = video_st->index;
			pkt.data = video_outbuf;

			// write the compressed frame in the media file
			if ( av_interleaved_write_frame( oc, &pkt ) != 0 )
			{
				mlt_log_fatal( MLT_CONSUMER_SERVICE(consumer), "error writing flushed video frame\n" );
				mlt_events_fire( properties, "consumer-fatal-error", NULL );
				goto on_fatal_error;
			}
			// Dual pass logging
			if ( mlt_properties_get_data( properties, "_logfile", NULL ) && c->stats_out )
				fprintf( mlt_properties_get_data( properties, "_logfile", NULL ), "%s", c->stats_out );
		}
	}

on_fatal_error:
	
	// Write the trailer, if any
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
	if ( !( fmt->flags & AVFMT_NOFILE ) )
#if LIBAVFORMAT_VERSION_MAJOR > 52
		avio_close( oc->pb );
#elif LIBAVFORMAT_VERSION_INT >= ((52<<16)+(0<<8)+0)
		url_fclose( oc->pb );
#else
		url_fclose( &oc->pb );
#endif

	// Clean up input and output frames
	if ( output )
		av_free( output->data[0] );
	av_free( output );
	av_free( input->data[0] );
	av_free( input );
	av_free( video_outbuf );
	av_free( audio_buf_1 );
	av_free( audio_buf_2 );

	// Free the stream
	av_free( oc );

	// Just in case we terminated on pause
	mlt_properties_set_int( properties, "running", 0 );

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
	}

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
