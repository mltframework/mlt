/*
 * consumer_avformat.c -- an encoder based on avformat
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

// Local header files
#include "consumer_avformat.h"

// mlt Header files
#include <framework/mlt_frame.h>

// System header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>

// avformat header files
#include <avformat.h>
#ifdef SWSCALE
#include <swscale.h>
#endif

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
	sample_fifo this = calloc( 1, sizeof( sample_fifo_s ) );
	this->frequency = frequency;
	this->channels = channels;
	return this;
}

// sample_fifo_clear and check are temporarily aborted (not working as intended)

void sample_fifo_clear( sample_fifo this, double time )
{
	int words = ( float )( time - this->time ) * this->frequency * this->channels;
	if ( ( int )( ( float )time * 100 ) < ( int )( ( float )this->time * 100 ) && this->used > words && words > 0 )
	{
		memmove( this->buffer, &this->buffer[ words ], ( this->used - words ) * sizeof( int16_t ) );
		this->used -= words;
		this->time = time;
	}
	else if ( ( int )( ( float )time * 100 ) != ( int )( ( float )this->time * 100 ) )
	{
		this->used = 0;
		this->time = time;
	}
}

void sample_fifo_check( sample_fifo this, double time )
{
	if ( this->used == 0 )
	{
		if ( ( int )( ( float )time * 100 ) < ( int )( ( float )this->time * 100 ) )
			this->time = time;
	}
}

void sample_fifo_append( sample_fifo this, int16_t *samples, int count )
{
	if ( ( this->size - this->used ) < count )
	{
		this->size += count * 5;
		this->buffer = realloc( this->buffer, this->size * sizeof( int16_t ) );
	}

	memcpy( &this->buffer[ this->used ], samples, count * sizeof( int16_t ) );
	this->used += count;
}

int sample_fifo_used( sample_fifo this )
{
	return this->used;
}

int sample_fifo_fetch( sample_fifo this, int16_t *samples, int count )
{
	if ( count > this->used )
		count = this->used;

	memcpy( samples, this->buffer, count * sizeof( int16_t ) );
	this->used -= count;
	memmove( this->buffer, &this->buffer[ count ], this->used * sizeof( int16_t ) );

	this->time += ( double )count / this->channels / this->frequency;

	return count;
}

void sample_fifo_close( sample_fifo this )
{
	free( this->buffer );
	free( this );
}

// Forward references.
static int consumer_start( mlt_consumer this );
static int consumer_stop( mlt_consumer this );
static int consumer_is_stopped( mlt_consumer this );
static void *consumer_thread( void *arg );
static void consumer_close( mlt_consumer this );

/** Initialise the dv consumer.
*/

mlt_consumer consumer_avformat_init( char *arg )
{
	// Allocate the consumer
	mlt_consumer this = mlt_consumer_new( );

	// If memory allocated and initialises without error
	if ( this != NULL )
	{
		// Get properties from the consumer
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

		// Assign close callback
		this->close = consumer_close;

		// Interpret the argument
		if ( arg != NULL )
			mlt_properties_set( properties, "target", arg );

		// sample and frame queue
		mlt_properties_set_data( properties, "frame_queue", mlt_deque_init( ), 0, ( mlt_destructor )mlt_deque_close, NULL );

		// Set avformat defaults (all lifted from ffmpeg.c)
		mlt_properties_set_int( properties, "audio_bit_rate", 128000 );
		mlt_properties_set_int( properties, "video_bit_rate", 200 * 1000 );
		mlt_properties_set_int( properties, "video_bit_rate_tolerance", 4000 * 1000 );
		mlt_properties_set_int( properties, "gop_size", 12 );
		mlt_properties_set_int( properties, "b_frames", 0 );
		mlt_properties_set_int( properties, "mb_decision", FF_MB_DECISION_SIMPLE );
		mlt_properties_set_double( properties, "qscale", 1 );
		mlt_properties_set_int( properties, "me_method", ME_EPZS );
		mlt_properties_set_int( properties, "mb_cmp", FF_CMP_SAD );
		mlt_properties_set_int( properties, "ildct_cmp", FF_CMP_VSAD );
		mlt_properties_set_int( properties, "sub_cmp", FF_CMP_SAD );
		mlt_properties_set_int( properties, "cmp", FF_CMP_SAD );
		mlt_properties_set_int( properties, "pre_cmp", FF_CMP_SAD );
		mlt_properties_set_int( properties, "pre_me", 0 );
		mlt_properties_set_double( properties, "lumi_mask", 0 );
		mlt_properties_set_double( properties, "dark_mask", 0 );
		mlt_properties_set_double( properties, "scplx_mask", 0 );
		mlt_properties_set_double( properties, "tcplx_mask", 0 );
		mlt_properties_set_double( properties, "p_mask", 0 );
		mlt_properties_set_int( properties, "qns", 0 );
		mlt_properties_set_int( properties, "video_qmin", 2 );
		mlt_properties_set_int( properties, "video_qmax", 31 );
		mlt_properties_set_int( properties, "video_lmin", 2*FF_QP2LAMBDA );
		mlt_properties_set_int( properties, "video_lmax", 31*FF_QP2LAMBDA );
		mlt_properties_set_int( properties, "video_mb_qmin", 2 );
		mlt_properties_set_int( properties, "video_mb_qmax", 31 );
		mlt_properties_set_int( properties, "video_qdiff", 3 );
		mlt_properties_set_double( properties, "video_qblur", 0.5 );
		mlt_properties_set_double( properties, "video_qcomp", 0.5 );
		mlt_properties_set_int( properties, "video_rc_max_rate", 0 );
		mlt_properties_set_int( properties, "video_rc_min_rate", 0 );
		mlt_properties_set_int( properties, "video_rc_buffer_size", 0 );
		mlt_properties_set_double( properties, "video_rc_buffer_aggressivity", 1.0 );
		mlt_properties_set_double( properties, "video_rc_initial_cplx", 0 );
		mlt_properties_set_double( properties, "video_i_qfactor", 1.25 );
		mlt_properties_set_double( properties, "video_b_qfactor", 1.25 );
		mlt_properties_set_double( properties, "video_i_qoffset", -0.8 );
		mlt_properties_set_double( properties, "video_b_qoffset", 0 );
		mlt_properties_set_int( properties, "video_intra_quant_bias", FF_DEFAULT_QUANT_BIAS );
		mlt_properties_set_int( properties, "video_inter_quant_bias", FF_DEFAULT_QUANT_BIAS );
		mlt_properties_set_int( properties, "dct_algo", 0 );
		mlt_properties_set_int( properties, "idct_algo", 0 );
		mlt_properties_set_int( properties, "me_threshold", 0 );
		mlt_properties_set_int( properties, "mb_threshold", 0 );
		mlt_properties_set_int( properties, "intra_dc_precision", 0 );
		mlt_properties_set_int( properties, "strict", 0 );
		mlt_properties_set_int( properties, "error_rate", 0 );
		mlt_properties_set_int( properties, "noise_reduction", 0 );
		mlt_properties_set_int( properties, "sc_threshold", 0 );
		mlt_properties_set_int( properties, "me_range", 0 );
		mlt_properties_set_int( properties, "coder", 0 );
		mlt_properties_set_int( properties, "context", 0 );
		mlt_properties_set_int( properties, "predictor", 0 );
		mlt_properties_set_int( properties, "ildct", 0 );
		mlt_properties_set_int( properties, "ilme", 0 );

		// Ensure termination at end of the stream
		mlt_properties_set_int( properties, "terminate_on_pause", 1 );

		// Set up start/stop/terminated callbacks
		this->start = consumer_start;
		this->stop = consumer_stop;
		this->is_stopped = consumer_is_stopped;
	}

	// Return this
	return this;
}

/** Start the consumer.
*/

static int consumer_start( mlt_consumer this )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Check that we're not already running
	if ( !mlt_properties_get_int( properties, "running" ) )
	{
		// Allocate a thread
		pthread_t *thread = calloc( 1, sizeof( pthread_t ) );

		// Get the width and height
		int width = mlt_properties_get_int( properties, "width" );
		int height = mlt_properties_get_int( properties, "height" );

		// Obtain the size property
		char *size = mlt_properties_get( properties, "size" );

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
				fprintf( stderr, "consumer_avformat: Invalid size property %s - ignoring.\n", size );
			}
		}
		
		// Now ensure we honour the multiple of two requested by libavformat
		mlt_properties_set_int( properties, "width", ( width / 2 ) * 2 );
		mlt_properties_set_int( properties, "height", ( height / 2 ) * 2 );

		// Assign the thread to properties
		mlt_properties_set_data( properties, "thread", thread, sizeof( pthread_t ), free, NULL );

		// Set the running state
		mlt_properties_set_int( properties, "running", 1 );

		// Create the thread
		pthread_create( thread, NULL, consumer_thread, this );
	}
	return 0;
}

/** Stop the consumer.
*/

static int consumer_stop( mlt_consumer this )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

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

static int consumer_is_stopped( mlt_consumer this )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );
	return !mlt_properties_get_int( properties, "running" );
}

/** Add an audio output stream
*/

static AVStream *add_audio_stream( mlt_consumer this, AVFormatContext *oc, int codec_id )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Create a new stream
	AVStream *st = av_new_stream( oc, 1 );

	// If created, then initialise from properties
	if ( st != NULL ) 
	{
		AVCodecContext *c = st->codec;
		c->codec_id = codec_id;
		c->codec_type = CODEC_TYPE_AUDIO;

		// Put sample parameters
		c->bit_rate = mlt_properties_get_int( properties, "audio_bit_rate" );
		c->sample_rate = mlt_properties_get_int( properties, "frequency" );
		c->channels = mlt_properties_get_int( properties, "channels" );

    	if (oc->oformat->flags & AVFMT_GLOBALHEADER) 
        	c->flags |= CODEC_FLAG_GLOBAL_HEADER;

		// Allow the user to override the audio fourcc
		if ( mlt_properties_get( properties, "afourcc" ) )
		{
			char *tail = NULL;
			char *arg = mlt_properties_get( properties, "afourcc" );
    		int tag = strtol( arg, &tail, 0);
    		if( !tail || *tail )
        		tag = arg[ 0 ] + ( arg[ 1 ] << 8 ) + ( arg[ 2 ] << 16 ) + ( arg[ 3 ] << 24 );
			c->codec_tag = tag;
		}
	}
	else
	{
		fprintf( stderr, "Could not allocate a stream for audio\n" );
	}

	return st;
}

static int open_audio( AVFormatContext *oc, AVStream *st, int audio_outbuf_size )
{
	// We will return the audio input size from here
	int audio_input_frame_size = 0;

	// Get the context
	AVCodecContext *c = st->codec;

	// Find the encoder
	AVCodec *codec = avcodec_find_encoder( c->codec_id );

	// Continue if codec found and we can open it
	if ( codec != NULL && avcodec_open(c, codec) >= 0 )
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
		if( !strcmp( oc->oformat->name, "mp4" ) || 
			!strcmp( oc->oformat->name, "mov" ) || 
			!strcmp( oc->oformat->name, "3gp" ) )
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	else
	{
		fprintf( stderr, "Unable to encode audio - disabling audio output.\n" );
	}

	return audio_input_frame_size;
}

static void close_audio( AVFormatContext *oc, AVStream *st )
{
	avcodec_close( st->codec );
}

/** Add a video output stream 
*/

static AVStream *add_video_stream( mlt_consumer this, AVFormatContext *oc, int codec_id )
{
 	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Create a new stream
	AVStream *st = av_new_stream( oc, 0 );

	if ( st != NULL ) 
	{
		char *pix_fmt = mlt_properties_get( properties, "pix_fmt" );
		double ar = mlt_properties_get_double( properties, "display_ratio" );
		AVCodecContext *c = st->codec;
		c->codec_id = codec_id;
		c->codec_type = CODEC_TYPE_VIDEO;

		// put sample parameters
		c->bit_rate = mlt_properties_get_int( properties, "video_bit_rate" );
		c->bit_rate_tolerance = mlt_properties_get_int( properties, "video_bit_rate_tolerance" );
		c->width = mlt_properties_get_int( properties, "width" );
		c->height = mlt_properties_get_int( properties, "height" );
		c->time_base.num = mlt_properties_get_int( properties, "frame_rate_den" );
		c->time_base.den = mlt_properties_get_int( properties, "frame_rate_num" );
		c->gop_size = mlt_properties_get_int( properties, "gop_size" );
		c->pix_fmt = pix_fmt ? avcodec_get_pix_fmt( pix_fmt ) : PIX_FMT_YUV420P;

		if ( mlt_properties_get_int( properties, "b_frames" ) )
		{
	 		c->max_b_frames = mlt_properties_get_int( properties, "b_frames" );
			c->b_frame_strategy = 0;
			c->b_quant_factor = 2.0;
		}

	 	c->mb_decision = mlt_properties_get_int( properties, "mb_decision" );
		c->sample_aspect_ratio = av_d2q( ar * c->height / c->width , 255);
		c->mb_cmp = mlt_properties_get_int( properties, "mb_cmp" );
		c->ildct_cmp = mlt_properties_get_int( properties, "ildct_cmp" );
		c->me_sub_cmp = mlt_properties_get_int( properties, "sub_cmp" );
		c->me_cmp = mlt_properties_get_int( properties, "cmp" );
		c->me_pre_cmp = mlt_properties_get_int( properties, "pre_cmp" );
		c->pre_me = mlt_properties_get_int( properties, "pre_me" );
		c->lumi_masking = mlt_properties_get_double( properties, "lumi_mask" );
		c->dark_masking = mlt_properties_get_double( properties, "dark_mask" );
		c->spatial_cplx_masking = mlt_properties_get_double( properties, "scplx_mask" );
		c->temporal_cplx_masking = mlt_properties_get_double( properties, "tcplx_mask" );
		c->p_masking = mlt_properties_get_double( properties, "p_mask" );
		c->quantizer_noise_shaping= mlt_properties_get_int( properties, "qns" );
		c->qmin = mlt_properties_get_int( properties, "video_qmin" );
		c->qmax = mlt_properties_get_int( properties, "video_qmax" );
		c->lmin = mlt_properties_get_int( properties, "video_lmin" );
		c->lmax = mlt_properties_get_int( properties, "video_lmax" );
		c->mb_qmin = mlt_properties_get_int( properties, "video_mb_qmin" );
		c->mb_qmax = mlt_properties_get_int( properties, "video_mb_qmax" );
		c->max_qdiff = mlt_properties_get_int( properties, "video_qdiff" );
		c->qblur = mlt_properties_get_double( properties, "video_qblur" );
		c->qcompress = mlt_properties_get_double( properties, "video_qcomp" );

		if ( mlt_properties_get_double( properties, "qscale" ) > 0 )
		{
			c->flags |= CODEC_FLAG_QSCALE;
			st->quality = FF_QP2LAMBDA * mlt_properties_get_double( properties, "qscale" );
		}

		// Allow the user to override the video fourcc
		if ( mlt_properties_get( properties, "vfourcc" ) )
		{
			char *tail = NULL;
			const char *arg = mlt_properties_get( properties, "vfourcc" );
    		int tag = strtol( arg, &tail, 0);
    		if( !tail || *tail )
        		tag = arg[ 0 ] + ( arg[ 1 ] << 8 ) + ( arg[ 2 ] << 16 ) + ( arg[ 3 ] << 24 );
			c->codec_tag = tag;
		}

		// Some formats want stream headers to be seperate
		if ( oc->oformat->flags & AVFMT_GLOBALHEADER ) 
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;

		c->rc_max_rate = mlt_properties_get_int( properties, "video_rc_max_rate" );
		c->rc_min_rate = mlt_properties_get_int( properties, "video_rc_min_rate" );
		c->rc_buffer_size = mlt_properties_get_int( properties, "video_rc_buffer_size" );
		c->rc_buffer_aggressivity= mlt_properties_get_double( properties, "video_rc_buffer_aggressivity" );
		c->rc_initial_cplx= mlt_properties_get_double( properties, "video_rc_initial_cplx" );
		c->i_quant_factor = mlt_properties_get_double( properties, "video_i_qfactor" );
		c->b_quant_factor = mlt_properties_get_double( properties, "video_b_qfactor" );
		c->i_quant_offset = mlt_properties_get_double( properties, "video_i_qoffset" );
		c->b_quant_offset = mlt_properties_get_double( properties, "video_b_qoffset" );
		c->intra_quant_bias = mlt_properties_get_int( properties, "video_intra_quant_bias" );
		c->inter_quant_bias = mlt_properties_get_int( properties, "video_inter_quant_bias" );
		c->dct_algo = mlt_properties_get_int( properties, "dct_algo" );
		c->idct_algo = mlt_properties_get_int( properties, "idct_algo" );
		c->me_threshold= mlt_properties_get_int( properties, "me_threshold" );
		c->mb_threshold= mlt_properties_get_int( properties, "mb_threshold" );
		c->intra_dc_precision= mlt_properties_get_int( properties, "intra_dc_precision" );
		c->strict_std_compliance = mlt_properties_get_int( properties, "strict" );
		c->error_rate = mlt_properties_get_int( properties, "error_rate" );
		c->noise_reduction= mlt_properties_get_int( properties, "noise_reduction" );
		c->scenechange_threshold= mlt_properties_get_int( properties, "sc_threshold" );
		c->me_range = mlt_properties_get_int( properties, "me_range" );
		c->coder_type= mlt_properties_get_int( properties, "coder" );
		c->context_model= mlt_properties_get_int( properties, "context" );
		c->prediction_method= mlt_properties_get_int( properties, "predictor" );
		c->me_method = mlt_properties_get_int( properties, "me_method" );
		if ( mlt_properties_get_int( properties, "progressive" ) == 0 &&
		     mlt_properties_get_int( properties, "deinterlace" ) == 0 )
		{
			if ( mlt_properties_get_int( properties, "ildct" ) )
				c->flags |= CODEC_FLAG_INTERLACED_DCT;
			if ( mlt_properties_get_int( properties, "ilme" ) )
				c->flags |= CODEC_FLAG_INTERLACED_ME;
		}
 	}
	else
	{
		fprintf( stderr, "Could not allocate a stream for video\n" );
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
	
static int open_video(AVFormatContext *oc, AVStream *st)
{
	// Get the codec
	AVCodecContext *video_enc = st->codec;

	// find the video encoder
	AVCodec *codec = avcodec_find_encoder( video_enc->codec_id );

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
	return codec != NULL && avcodec_open( video_enc, codec ) >= 0;
}

void close_video(AVFormatContext *oc, AVStream *st)
{
	avcodec_close(st->codec);
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
	mlt_consumer this = arg;

	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Get the terminate on pause property
	int terminate_on_pause = mlt_properties_get_int( properties, "terminate_on_pause" );
	int terminated = 0;

	// Determine if feed is slow (for realtime stuff)
	int real_time_output = mlt_properties_get_int( properties, "real_time" );

	// Time structures
	struct timeval ante;

	// Get the frame rate
	int fps = mlt_properties_get_double( properties, "fps" );

	// Get width and height
	int width = mlt_properties_get_int( properties, "width" );
	int height = mlt_properties_get_int( properties, "height" );
	int img_width = width;
	int img_height = height;

	// Get default audio properties
	mlt_audio_format aud_fmt = mlt_audio_pcm;
	int channels = mlt_properties_get_int( properties, "channels" );
	int frequency = mlt_properties_get_int( properties, "frequency" );
	int16_t *pcm = NULL;
	int samples = 0;

	// AVFormat audio buffer and frame size
 	int audio_outbuf_size = 10000;
 	uint8_t *audio_outbuf = av_malloc( audio_outbuf_size );
	int audio_input_frame_size = 0;

	// AVFormat video buffer and frame count
	int frame_count = 0;
	int video_outbuf_size = ( 1024 * 1024 );
	uint8_t *video_outbuf = av_malloc( video_outbuf_size );

	// Used for the frame properties
	mlt_frame frame = NULL;
	mlt_properties frame_properties = NULL;

	// Get the queues
	mlt_deque queue = mlt_properties_get_data( properties, "frame_queue", NULL );
	sample_fifo fifo = mlt_properties_get_data( properties, "sample_fifo", NULL );

	// Need two av pictures for converting
	AVFrame *output = NULL;
	AVFrame *input = alloc_picture( PIX_FMT_YUV422, width, height );

	// For receiving images from an mlt_frame
	uint8_t *image;
	mlt_image_format img_fmt = mlt_image_yuv422;

	// For receiving audio samples back from the fifo
	int16_t *buffer = av_malloc( 48000 * 2 );
	int count = 0;

	// Allocate the context
	AVFormatContext *oc = av_alloc_format_context( );

	// Streams
	AVStream *audio_st = NULL;
	AVStream *video_st = NULL;

	// Time stamps
	double audio_pts = 0;
	double video_pts = 0;

	// Loop variable
	int i;

	// Frames despatched
	long int frames = 0;
	long int total_time = 0;

	// Determine the format
	AVOutputFormat *fmt = NULL;
	char *filename = mlt_properties_get( properties, "target" );
	char *format = mlt_properties_get( properties, "format" );
	char *vcodec = mlt_properties_get( properties, "vcodec" );
	char *acodec = mlt_properties_get( properties, "acodec" );

	// Used to store and override codec ids
	int audio_codec_id;
	int video_codec_id;

	// Check for user selected format first
	if ( format != NULL )
		fmt = guess_format( format, NULL, NULL );

	// Otherwise check on the filename
	if ( fmt == NULL && filename != NULL )
		fmt = guess_format( NULL, filename, NULL );

	// Otherwise default to mpeg
	if ( fmt == NULL )
		fmt = guess_format( "mpeg", NULL, NULL );

	// We need a filename - default to stdout?
	if ( filename == NULL || !strcmp( filename, "" ) )
		filename = "pipe:";

	// Get the codec ids selected
	audio_codec_id = fmt->audio_codec;
	video_codec_id = fmt->video_codec;

	// Check for audio codec overides
	if ( acodec != NULL )
	{
		AVCodec *p = first_avcodec;
		while( p != NULL ) 
		{
			if ( !strcmp( p->name, acodec ) && p->type == CODEC_TYPE_AUDIO )
				break;
			p = p->next;
		}
		if ( p != NULL )
			audio_codec_id = p->id;
		else
			fprintf( stderr, "consumer_avcodec: audio codec %s unrecognised - ignoring\n", acodec );
	}

	// Check for video codec overides
	if ( vcodec != NULL )
	{
		AVCodec *p = first_avcodec;
		while( p != NULL ) 
		{
			if ( !strcmp( p->name, vcodec ) && p->type == CODEC_TYPE_VIDEO )
				break;
			p = p->next;
		}
		if ( p != NULL )
			video_codec_id = p->id;
		else
			fprintf( stderr, "consumer_avcodec: video codec %s unrecognised - ignoring\n", vcodec );
	}

	// Update the output context

	// Write metadata
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

	oc->oformat = fmt;
	snprintf( oc->filename, sizeof(oc->filename), "%s", filename );

	// Add audio and video streams 
	if ( fmt->video_codec != CODEC_ID_NONE )
		video_st = add_video_stream( this, oc, video_codec_id );
	if ( fmt->audio_codec != CODEC_ID_NONE )
		audio_st = add_audio_stream( this, oc, audio_codec_id );

	// Set the parameters (even though we have none...)
	if ( av_set_parameters(oc, NULL) >= 0 ) 
	{
		if ( video_st && !open_video( oc, video_st ) )
			video_st = NULL;
		if ( audio_st )
			audio_input_frame_size = open_audio( oc, audio_st, audio_outbuf_size );

		// Open the output file, if needed
		if ( !( fmt->flags & AVFMT_NOFILE ) ) 
		{
			if (url_fopen(&oc->pb, filename, URL_WRONLY) < 0) 
			{
				fprintf(stderr, "Could not open '%s'\n", filename);
				mlt_properties_set_int( properties, "running", 0 );
			}
		}
	
		// Write the stream header, if any
		if ( mlt_properties_get_int( properties, "running" ) )
			av_write_header( oc );
	}
	else
	{
		fprintf(stderr, "Invalid output format parameters\n");
		mlt_properties_set_int( properties, "running", 0 );
	}

	// Allocate picture
	if ( video_st )
		output = alloc_picture( video_st->codec->pix_fmt, width, height );

	// Last check - need at least one stream
	if ( audio_st == NULL && video_st == NULL )
		mlt_properties_set_int( properties, "running", 0 );

	// Get the starting time (can ignore the times above)
	gettimeofday( &ante, NULL );

	// Loop while running
	while( mlt_properties_get_int( properties, "running" ) && !terminated )
	{
		// Get the frame
		frame = mlt_consumer_rt_frame( this );

		// Check that we have a frame to work with
		if ( frame != NULL )
		{
			// Increment frames despatched
			frames ++;

			// Default audio args
			frame_properties = MLT_FRAME_PROPERTIES( frame );

			// Check for the terminated condition
			terminated = terminate_on_pause && mlt_properties_get_double( frame_properties, "_speed" ) == 0.0;

			// Get audio and append to the fifo
			if ( !terminated && audio_st )
			{
				samples = mlt_sample_calculator( fps, frequency, count ++ );
				mlt_frame_get_audio( frame, &pcm, &aud_fmt, &frequency, &channels, &samples );

				// Create the fifo if we don't have one
				if ( fifo == NULL )
				{
					fifo = sample_fifo_init( frequency, channels );
					mlt_properties_set_data( properties, "sample_fifo", fifo, 0, ( mlt_destructor )sample_fifo_close, NULL );
				}

				if ( mlt_properties_get_double( frame_properties, "_speed" ) != 1.0 )
					memset( pcm, 0, samples * channels * 2 );

				// Append the samples
				sample_fifo_append( fifo, pcm, samples * channels );
				total_time += ( samples * 1000000 ) / frequency;
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
			if (audio_st)
				audio_pts = (double)audio_st->pts.val * audio_st->time_base.num / audio_st->time_base.den;
			else
				audio_pts = 0.0;
        
			if (video_st)
				video_pts = (double)video_st->pts.val * video_st->time_base.num / video_st->time_base.den;
			else
				video_pts = 0.0;

 			// Write interleaved audio and video frames
 			if ( !video_st || ( video_st && audio_st && audio_pts < video_pts ) )
			{
				if ( channels * audio_input_frame_size < sample_fifo_used( fifo ) )
				{
 					AVCodecContext *c;
					AVPacket pkt;
					av_init_packet( &pkt );

					c = audio_st->codec;

					sample_fifo_fetch( fifo, buffer, channels * audio_input_frame_size );

					pkt.size = avcodec_encode_audio( c, audio_outbuf, audio_outbuf_size, buffer );
 					// Write the compressed frame in the media file
					if ( c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE )
						pkt.pts = av_rescale_q( c->coded_frame->pts, c->time_base, audio_st->time_base );
					pkt.flags |= PKT_FLAG_KEY;
					pkt.stream_index= audio_st->index;
					pkt.data= audio_outbuf;

					if ( pkt.size )
 						if ( av_interleaved_write_frame( oc, &pkt ) != 0) 
 							fprintf(stderr, "Error while writing audio frame\n");

					audio_pts += c->frame_size;
				}
				else
				{
					break;
				}
			}
 			else if ( video_st )
			{
				if ( mlt_deque_count( queue ) )
				{
 					int out_size, ret;
 					AVCodecContext *c;

					frame = mlt_deque_pop_front( queue );
					frame_properties = MLT_FRAME_PROPERTIES( frame );

					c = video_st->codec;
 					
					if ( mlt_properties_get_int( frame_properties, "rendered" ) )
					{
						int i = 0;
						int j = 0;
						uint8_t *p;
						uint8_t *q;

						mlt_events_fire( properties, "consumer-frame-show", frame, NULL );

						mlt_frame_get_image( frame, &image, &img_fmt, &img_width, &img_height, 0 );

						q = image;

						// Convert the mlt frame to an AVPicture
						for ( i = 0; i < height; i ++ )
						{
							p = input->data[ 0 ] + i * input->linesize[ 0 ];
							j = width;
							while( j -- )
							{
								*p ++ = *q ++;
								*p ++ = *q ++;
							}
						}

						// Do the colour space conversion
#ifdef SWSCALE
						struct SwsContext *context = sws_getContext( width, height, PIX_FMT_YUV422,
							width, height, video_st->codec->pix_fmt, SWS_FAST_BILINEAR, NULL, NULL, NULL);
						sws_scale( context, input->data, input->linesize, 0, height,
							output->data, output->linesize);
						sws_freeContext( context );
#else
						img_convert( ( AVPicture * )output, video_st->codec->pix_fmt, ( AVPicture * )input, PIX_FMT_YUV422, width, height );
#endif

						// Apply the alpha if applicable
						if ( video_st->codec->pix_fmt == PIX_FMT_RGBA32 )
						{
							uint8_t *alpha = mlt_frame_get_alpha_mask( frame );
							register int n;

							for ( i = 0; i < height; i ++ )
							{
								n = ( width + 7 ) / 8;
								p = output->data[ 0 ] + i * output->linesize[ 0 ];

								#ifndef __DARWIN__
								p += 3;
								#endif

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

	 					// Encode the image
	 					out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, output );

	 					// If zero size, it means the image was buffered
	 					if (out_size > 0) 
						{
							AVPacket pkt;
							av_init_packet( &pkt );

							if ( c->coded_frame && c->coded_frame->pts != AV_NOPTS_VALUE )
								pkt.pts= av_rescale_q( c->coded_frame->pts, c->time_base, video_st->time_base );
							if( c->coded_frame && c->coded_frame->key_frame )
								pkt.flags |= PKT_FLAG_KEY;
							pkt.stream_index= video_st->index;
							pkt.data= video_outbuf;
							pkt.size= out_size;

             				// write the compressed frame in the media file
							ret = av_interleaved_write_frame(oc, &pkt);
							video_pts += c->frame_size;
	 					} 
						else
						{
							fprintf( stderr, "Error with video encode\n" );
						}
 					}
 					frame_count++;
					mlt_frame_close( frame );
				}
				else
				{
					break;
				}
			}
		}

		if ( real_time_output && frames % 12 == 0 )
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

	// close each codec 
	if (video_st)
		close_video(oc, video_st);
	if (audio_st)
		close_audio(oc, audio_st);

	// Write the trailer, if any
	av_write_trailer(oc);

	// Free the streams
	for(i = 0; i < oc->nb_streams; i++)
		av_freep(&oc->streams[i]);

	// Close the output file
	if (!(fmt->flags & AVFMT_NOFILE))
		url_fclose(&oc->pb);

	// Clean up input and output frames
	if ( output )
		av_free( output->data[0] );
	av_free( output );
	av_free( input->data[0] );
	av_free( input );
	av_free( video_outbuf );
	av_free( buffer );

	// Free the stream
	av_free(oc);

	// Just in case we terminated on pause
	mlt_properties_set_int( properties, "running", 0 );

	mlt_consumer_stopped( this );

	return NULL;
}

/** Close the consumer.
*/

static void consumer_close( mlt_consumer this )
{
	// Stop the consumer
	mlt_consumer_stop( this );

	// Close the parent
	mlt_consumer_close( this );

	// Free the memory
	free( this );
}
