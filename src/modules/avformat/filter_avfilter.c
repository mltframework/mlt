/*
 * filter_avfilter.c -- provide various filters based on libavfilter
 * Copyright (C) 2016-2021 Meltytech, LLC
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

#include "common.h"

#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/samplefmt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>

#define PARAM_PREFIX "av."
#define PARAM_PREFIX_LEN (sizeof(PARAM_PREFIX) - 1)
#define MLT_SWS_FLAGS "bicubic+accurate_rnd+full_chroma_int+full_chroma_inp"

typedef struct
{
	AVFilter* avfilter;
	AVFilterContext* avbuffsink_ctx;
	AVFilterContext* avbuffsrc_ctx;
	AVFilterContext* avfilter_ctx;
	AVFilterContext* scale_ctx;
	AVFilterContext* pad_ctx;
	AVFilterGraph* avfilter_graph;
	AVFrame* avinframe;
	AVFrame* avoutframe;
	int format;
	int width;
	int height;
	int reset;
} private_data;

static void property_changed( mlt_service owner, mlt_filter filter, mlt_event_data event_data )
{
	const char *name = mlt_event_data_to_string(event_data);
	if( name && strncmp( PARAM_PREFIX, name, PARAM_PREFIX_LEN ) == 0 ) {
		private_data* pdata = (private_data*)filter->child;
		if( pdata->avfilter )
		{
			const AVOption *opt = NULL;
			while( ( opt = av_opt_next( &pdata->avfilter->priv_class, opt ) ) )
			{
				if( !strcmp( opt->name, name + PARAM_PREFIX_LEN ) )
				{
					pdata->reset = 1;
					break;
				}
			}
		}
	}
}

static int mlt_to_av_image_format( mlt_image_format format )
{
	switch( format )
	{
	case mlt_image_none:
		return AV_PIX_FMT_NONE;
	case mlt_image_rgb:
		return AV_PIX_FMT_RGB24;
	case mlt_image_rgba:
		return AV_PIX_FMT_RGBA;
	case mlt_image_yuv422:
		return AV_PIX_FMT_YUYV422;
	case mlt_image_yuv420p:
		return AV_PIX_FMT_YUV420P;
	default:
		mlt_log_error(NULL, "[filter_avfilter] Unknown image format: %d\n", format );
		return AV_PIX_FMT_NONE;
	}
}

static mlt_image_format get_supported_image_format( mlt_image_format format )
{
	switch( format )
	{
	case mlt_image_rgba:
		return mlt_image_rgba;
	case mlt_image_rgb:
		return mlt_image_rgb;
	case mlt_image_yuv420p:
		return mlt_image_yuv420p;
	default:
		mlt_log_error(NULL, "[filter_avfilter] Unknown image format requested: %d\n", format );
	case mlt_image_none:
	case mlt_image_yuv422:
	case mlt_image_glsl:
	case mlt_image_glsl_texture:
		return mlt_image_yuv422;
	}
}

static void set_avfilter_options( mlt_filter filter, double scale)
{
	private_data* pdata = (private_data*)filter->child;
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
	int i;
	int count = mlt_properties_count( filter_properties );
	mlt_properties scale_map = mlt_properties_get_data(filter_properties, "_resolution_scale", NULL);

	for( i = 0; i < count; i++ )
	{
		const char *param_name = mlt_properties_get_name( filter_properties, i );
		if( param_name && strncmp( PARAM_PREFIX, param_name, PARAM_PREFIX_LEN ) == 0 )
		{
			const AVOption *opt = av_opt_find( pdata->avfilter_ctx->priv, param_name + PARAM_PREFIX_LEN, 0, 0, 0 );
			const char* value = mlt_properties_get_value( filter_properties, i );
			if( opt && value )
			{
				if (scale != 1.0) {
					double scale2 = mlt_properties_get_double(scale_map, opt->name);
					if (scale2 != 0.0) {
						double x = mlt_properties_get_double(filter_properties, param_name);
						x *= scale * scale2;
						mlt_properties_set_double(filter_properties, "_avfilter_temp", x);
						value = mlt_properties_get(filter_properties, "_avfilter_temp");
					}
				}
				av_opt_set( pdata->avfilter_ctx->priv, opt->name, value, 0 );
			}
		}
	}
}

static void init_audio_filtergraph( mlt_filter filter, mlt_audio_format format, int frequency, int channels )
{
	private_data* pdata = (private_data*)filter->child;
	AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
	AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
	int sample_fmts[] = { -1, -1 };
	int sample_rates[] = { -1, -1 };
	int channel_counts[] = { -1, -1 };
	int64_t channel_layouts[] = { -1, -1 };
	char channel_layout_str[64];
	int ret;

	pdata->format = format;

	// Set up formats
	sample_fmts[0] = mlt_to_av_sample_format( format );
	sample_rates[0] = frequency;
	channel_counts[0] = channels;
	channel_layouts[0] = av_get_default_channel_layout( channels );
	av_get_channel_layout_string( channel_layout_str, sizeof(channel_layout_str), 0, channel_layouts[0]);

	// Destroy the current filter graph
	avfilter_graph_free( &pdata->avfilter_graph );

	// Create the new filter graph
	pdata->avfilter_graph = avfilter_graph_alloc();
	if( !pdata->avfilter_graph ) {
		mlt_log_error( filter, "Cannot create filter graph\n" );
		goto fail;
	}

	// Set thread count if supported.
	if ( pdata->avfilter->flags & AVFILTER_FLAG_SLICE_THREADS ) {
		av_opt_set_int( pdata->avfilter_graph, "threads",
			FFMAX( 0, mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "av.threads" ) ), 0 );
	}

	// Initialize the buffer source filter context
	pdata->avbuffsrc_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, abuffersrc, "in");
	if( !pdata->avbuffsrc_ctx ) {
		mlt_log_error( filter, "Cannot create audio buffer source\n" );
		goto fail;
	}
	ret = av_opt_set_int( pdata->avbuffsrc_ctx, "sample_rate", sample_rates[0], AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set src sample rate %d\n", sample_rates[0] );
		goto fail;
	}
	ret = av_opt_set_int( pdata->avbuffsrc_ctx, "sample_fmt", sample_fmts[0], AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set src sample format %d\n", sample_fmts[0] );
		goto fail;
	}
	ret = av_opt_set_int( pdata->avbuffsrc_ctx, "channels", channel_counts[0], AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set src channels %d\n", channel_counts[0] );
		goto fail;
	}
	ret = av_opt_set( pdata->avbuffsrc_ctx, "channel_layout", channel_layout_str, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set src channel layout %s\n", channel_layout_str );
		goto fail;
	}
	ret = avfilter_init_str( pdata->avbuffsrc_ctx, NULL );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot init buffer source\n" );
		goto fail;
	}

	// Initialize the buffer sink filter context
	pdata->avbuffsink_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, abuffersink, "out");
	if( !pdata->avbuffsink_ctx ) {
		mlt_log_error( filter, "Cannot create audio buffer sink\n" );
		goto fail;
	}
	ret = av_opt_set_int_list( pdata->avbuffsink_ctx, "sample_fmts", sample_fmts, -1, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set sink sample formats\n" );
		goto fail;
	}
	ret = av_opt_set_int_list( pdata->avbuffsink_ctx, "sample_rates", sample_rates, -1, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set sink sample rates\n" );
		goto fail;
	}
	ret = av_opt_set_int_list( pdata->avbuffsink_ctx, "channel_counts", channel_counts, -1, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set sink channel counts\n" );
		goto fail;
	}
	ret = av_opt_set_int_list( pdata->avbuffsink_ctx, "channel_layouts", channel_layouts, -1, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set sink channel_layouts\n" );
		goto fail;
	}
	ret = avfilter_init_str(  pdata->avbuffsink_ctx, NULL );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot init buffer sink\n" );
		goto fail;
	}

	// Initialize the filter context
	pdata->avfilter_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, pdata->avfilter, pdata->avfilter->name );
	if( !pdata->avfilter_ctx ) {
		mlt_log_error( filter, "Cannot create audio filter\n" );
		goto fail;
	}
	set_avfilter_options( filter, 1.0 );
	ret = avfilter_init_str(  pdata->avfilter_ctx, NULL );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot init filter\n" );
		goto fail;
	}

	// Connect the filters
	ret = avfilter_link( pdata->avbuffsrc_ctx, 0, pdata->avfilter_ctx, 0 );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot link src to filter\n" );
		goto fail;
	}
	ret = avfilter_link( pdata->avfilter_ctx, 0, pdata->avbuffsink_ctx, 0 );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot link filter to sink\n" );
		goto fail;
	}

	// Configure the graph.
	ret = avfilter_graph_config( pdata->avfilter_graph, NULL );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot configure the filter graph\n" );
		goto fail;
	}

	return;

fail:
	avfilter_graph_free( &pdata->avfilter_graph );
}


static void init_image_filtergraph( mlt_filter filter, mlt_image_format format, int width, int height, double resolution_scale )
{
	private_data* pdata = (private_data*)filter->child;
	mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
	AVFilter *buffersrc  = avfilter_get_by_name("buffer");
	AVFilter *buffersink = avfilter_get_by_name("buffersink");
	AVFilter *scale = avfilter_get_by_name("scale");
	AVFilter *pad = avfilter_get_by_name("pad");
	mlt_properties p = mlt_properties_new();
	enum AVPixelFormat pixel_fmts[] = { -1, -1 };
	AVRational sar = (AVRational){ profile->sample_aspect_num, profile->frame_rate_den };
	AVRational timebase = (AVRational){ profile->frame_rate_den, profile->frame_rate_num };
	AVRational framerate = (AVRational){ profile->frame_rate_num, profile->frame_rate_den };
	int ret;

	pdata->format = format;
	pdata->width = width;
	pdata->height = height;

	// Set up formats
	pixel_fmts[0] = mlt_to_av_image_format( format );

	// Destroy the current filter graph
	avfilter_graph_free( &pdata->avfilter_graph );

	// Create the new filter graph
	pdata->avfilter_graph = avfilter_graph_alloc();
	if( !pdata->avfilter_graph ) {
		mlt_log_error( filter, "Cannot create filter graph\n" );
		goto fail;
	}
	pdata->avfilter_graph->scale_sws_opts = av_strdup("flags=" MLT_SWS_FLAGS);

	// Set thread count if supported.
	if ( pdata->avfilter->flags & AVFILTER_FLAG_SLICE_THREADS ) {
		av_opt_set_int( pdata->avfilter_graph, "threads",
			FFMAX( 0, mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "av.threads" ) ), 0 );
	}

	// Initialize the buffer source filter context
	pdata->avbuffsrc_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, buffersrc, "in");
	if( !pdata->avbuffsrc_ctx ) {
		mlt_log_error( filter, "Cannot create image buffer source\n" );
		goto fail;
	}
	ret = av_opt_set_int( pdata->avbuffsrc_ctx, "width", width, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set src width %d\n", width );
		goto fail;
	}
	ret = av_opt_set_int( pdata->avbuffsrc_ctx, "height", height, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set src height %d\n", height );
		goto fail;
	}
	ret = av_opt_set_pixel_fmt( pdata->avbuffsrc_ctx, "pix_fmt", pixel_fmts[0], AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set src pixel format %d\n", pixel_fmts[0] );
		goto fail;
	}
	ret = av_opt_set_q( pdata->avbuffsrc_ctx, "sar", sar, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set src sar %d/%d\n", sar.num, sar.den );
		goto fail;
	}
	ret = av_opt_set_q( pdata->avbuffsrc_ctx, "time_base", timebase, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set src time_base %d/%d\n", timebase.num, timebase.den );
		goto fail;
	}
	ret = av_opt_set_q( pdata->avbuffsrc_ctx, "frame_rate", framerate, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set src frame_rate %d/%d\n", framerate.num, framerate.den );
		goto fail;
	}
	ret = avfilter_init_str( pdata->avbuffsrc_ctx, NULL );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot init buffer source\n" );
		goto fail;
	}

	// Initialize the buffer sink filter context
	pdata->avbuffsink_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, buffersink, "out");
	if( !pdata->avbuffsink_ctx ) {
		mlt_log_error( filter, "Cannot create image buffer sink\n" );
		goto fail;
	}
	ret = av_opt_set_int_list( pdata->avbuffsink_ctx, "pix_fmts", pixel_fmts, -1, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set sink pixel formats\n" );
		goto fail;
	}
	ret = avfilter_init_str(  pdata->avbuffsink_ctx, NULL );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot init buffer sink\n" );
		goto fail;
	}

	// Initialize the filter context
	pdata->avfilter_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, pdata->avfilter, pdata->avfilter->name );
	if( !pdata->avfilter_ctx ) {
		mlt_log_error( filter, "Cannot create video filter\n" );
		goto fail;
	}
	set_avfilter_options( filter, resolution_scale );

	if ( !strcmp( "lut3d", pdata->avfilter->name ) ) {
#if defined(__GLIBC__) || defined(__APPLE__) || (__FreeBSD__)
		// LUT data files use period for the decimal point regardless of LC_NUMERIC.
		locale_t posix_locale = newlocale( LC_NUMERIC_MASK, "POSIX", NULL );
		// Get the current locale and switch to POSIX local.
		locale_t orig_locale  = uselocale( posix_locale );
		// Initialize the filter.
		ret = avfilter_init_str(  pdata->avfilter_ctx, NULL );
		// Restore the original locale.
		uselocale( orig_locale );
		freelocale( posix_locale );
#else
		// Get the current locale and switch to POSIX local.
		char *orig_localename = strdup( setlocale( LC_NUMERIC, NULL ) );
		setlocale( LC_NUMERIC, "C" );
		// Initialize the filter.
		ret = avfilter_init_str(  pdata->avfilter_ctx, NULL );
		// Restore the original locale.
		setlocale( LC_NUMERIC, orig_localename );
		free( orig_localename );
#endif
	} else {
		ret = avfilter_init_str(  pdata->avfilter_ctx, NULL );
	}
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot init scale filter: %s\n", av_err2str(ret) );
		goto fail;
	}

	// scale=w=1280:h=720:force_original_aspect_ratio=decrease, pad=w=1280:h=720:x=(ow-iw)/2:y=(oh-ih)/2

	// Initialize the scale filter context
	pdata->scale_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, scale, "scale");
	if( !pdata->scale_ctx ) {
		mlt_log_error( filter, "Cannot create scale filer\n" );
		goto fail;
	}
	mlt_properties_set_int( p, "w", width );
	mlt_properties_set_int( p, "h", height );
	const AVOption *opt = av_opt_find( pdata->scale_ctx->priv, "w", 0, 0, 0 );
	if ( opt ) {
		ret = av_opt_set( pdata->scale_ctx->priv, opt->name, mlt_properties_get(p, "w"), 0 );
		if ( ret < 0 ) {
			mlt_log_error( filter, "Cannot set scale width\n" );
			goto fail;
		}
	}
	opt = av_opt_find( pdata->scale_ctx->priv, "h", 0, 0, 0 );
	if ( opt ) {
		ret = av_opt_set( pdata->scale_ctx->priv, opt->name, mlt_properties_get(p, "h"), 0 );
		if ( ret < 0 ) {
			mlt_log_error( filter, "Cannot set scale height\n" );
			goto fail;
		}
	}
	ret = av_opt_set_int( pdata->scale_ctx, "force_original_aspect_ratio", 1, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot set scale force_original_aspect_ratio\n" );
		goto fail;
	}
	opt = av_opt_find( pdata->scale_ctx->priv, "flags", 0, 0, 0 );
	if ( opt ) {
		ret = av_opt_set( pdata->scale_ctx->priv, opt->name, MLT_SWS_FLAGS, 0 );
		if ( ret < 0 ) {
			mlt_log_error( filter, "Cannot set scale flags\n" );
			goto fail;
		}
	}
	ret = avfilter_init_str(  pdata->scale_ctx, NULL );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot init scale filter\n" );
		goto fail;
	}

	// Initialize the padding filter context
	pdata->pad_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, pad, "pad");
	if( !pdata->pad_ctx ) {
		mlt_log_error( filter, "Cannot create pad filter\n" );
		goto fail;
	}
	opt = av_opt_find( pdata->pad_ctx->priv, "w", 0, 0, 0 );
	if ( opt ) {
		ret = av_opt_set( pdata->pad_ctx->priv, opt->name, mlt_properties_get(p, "w"), 0 );
		if ( ret < 0 ) {
			mlt_log_error( filter, "Cannot set pad width\n" );
			goto fail;
		}
	}
	opt = av_opt_find( pdata->pad_ctx->priv, "h", 0, 0, 0 );
	if ( opt ) {
		ret = av_opt_set( pdata->pad_ctx->priv, opt->name, mlt_properties_get(p, "h"), 0 );
		if ( ret < 0 ) {
			mlt_log_error( filter, "Cannot pad scale height\n" );
			goto fail;
		}
	}
	opt = av_opt_find( pdata->pad_ctx->priv, "x", 0, 0, 0 );
	if ( opt ) {
		ret = av_opt_set( pdata->pad_ctx->priv, opt->name, "(ow-iw)/2", 0 );
		if ( ret < 0 ) {
			mlt_log_error( filter, "Cannot set pad x\n" );
			goto fail;
		}
	}
	opt = av_opt_find( pdata->pad_ctx->priv, "y", 0, 0, 0 );
	if ( opt ) {
		ret = av_opt_set( pdata->pad_ctx->priv, opt->name, "(oh-ih)/2", 0 );
		if ( ret < 0 ) {
			mlt_log_error( filter, "Cannot set pad y\n" );
			goto fail;
		}
	}
	ret = avfilter_init_str(  pdata->pad_ctx, NULL );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot init pad filter\n" );
		goto fail;
	}

	// Connect the filters
	ret = avfilter_link( pdata->avbuffsrc_ctx, 0, pdata->avfilter_ctx, 0 );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot link src to filter\n" );
		goto fail;
	}
	ret = avfilter_link( pdata->avfilter_ctx, 0, pdata->scale_ctx, 0 );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot link filter to scale\n" );
		goto fail;
	}
	ret = avfilter_link( pdata->scale_ctx, 0, pdata->pad_ctx, 0 );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot link scale to pad\n" );
		goto fail;
	}
	ret = avfilter_link( pdata->pad_ctx, 0, pdata->avbuffsink_ctx, 0 );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot link pad to sink\n" );
		goto fail;
	}

	// Configure the graph.
	ret = avfilter_graph_config( pdata->avfilter_graph, NULL );
	if( ret < 0 ) {
		mlt_log_error( filter, "Cannot configure the filter graph\n" );
		goto fail;
	}

	return;

fail:
	mlt_properties_close( p );
	avfilter_graph_free( &pdata->avfilter_graph );
}

mlt_position get_position(mlt_filter filter, mlt_frame frame)
{
	mlt_position position = mlt_frame_get_position(frame);
	const char* pos_type = mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "position");
	if (pos_type) {
		if (!strcmp("filter", pos_type)) {
			position = mlt_filter_get_position(filter, frame);
		} else if (!strcmp("source", pos_type)) {
			position = mlt_frame_original_position(frame);
		} else if (!strcmp("producer", pos_type)) {
			mlt_producer producer = mlt_properties_get_data(MLT_FILTER_PROPERTIES(filter), "service", NULL);
			if (producer)
				position = mlt_producer_position(producer);
		}
	} else {
		private_data* pdata = (private_data*)filter->child;
		if (!strcmp("subtitles", pdata->avfilter->name))
			position = mlt_frame_original_position(frame);
	}
	return position;
}

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_filter filter = mlt_frame_pop_audio( frame );
	private_data* pdata = (private_data*)filter->child;
	double fps = mlt_profile_fps( mlt_service_profile(MLT_FILTER_SERVICE(filter)) );
	int64_t samplepos = mlt_audio_calculate_samples_to_position( fps, *frequency, get_position(filter, frame) );
	int bufsize = 0;
	int ret;

	// Get the producer's audio
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	bufsize = mlt_audio_format_size( *format, *samples, *channels );

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	if( pdata->reset || pdata->format != *format )
	{
		init_audio_filtergraph( filter, *format, *frequency, *channels );
		pdata->reset = 0;
	}

	if( pdata->avfilter_graph )
	{
		// Set up the input frame
		mlt_channel_layout layout = mlt_get_channel_layout_or_default( mlt_properties_get( MLT_FRAME_PROPERTIES(frame), "channel_layout" ) , *channels );
		pdata->avinframe->sample_rate = *frequency;
		pdata->avinframe->format = mlt_to_av_sample_format( *format );
		pdata->avinframe->channel_layout = mlt_to_av_channel_layout( layout );
		pdata->avinframe->channels = *channels;
		pdata->avinframe->nb_samples = *samples;
		pdata->avinframe->pts = samplepos;
		ret = av_frame_get_buffer( pdata->avinframe, 1 );
		if( ret < 0 ) {
			mlt_log_error( filter, "Cannot get in frame buffer\n" );
		}

		if( av_sample_fmt_is_planar( pdata->avinframe->format ) )
		{
			int i = 0;
			int stride = bufsize / *channels;
			for( i = 0; i < * channels; i++ )
			{
				memcpy( pdata->avinframe->extended_data[i],
						(uint8_t*)*buffer + stride * i,
						stride );
			}
		}
		else
		{
			memcpy( pdata->avinframe->extended_data[0],
					(uint8_t*)*buffer,
					bufsize );
		}

		// Run the frame through the filter graph
		ret = av_buffersrc_add_frame( pdata->avbuffsrc_ctx, pdata->avinframe );
		if( ret < 0 ) {
			mlt_log_error( filter, "Cannot add frame to buffer source\n" );
		}
		ret = av_buffersink_get_frame( pdata->avbuffsink_ctx, pdata->avoutframe );
		if( ret < 0 ) {
			mlt_log_error( filter, "Cannot get frame from buffer sink\n" );
		}

		// Sanity check the output frame
		if( *channels != pdata->avoutframe->channels ||
			*samples != pdata->avoutframe->nb_samples ||
			*frequency != pdata->avoutframe->sample_rate )
		{
			mlt_log_error( filter, "Unexpected return format\n" );
			goto exit;
		}

		// Copy the filter output into the original buffer
		if( av_sample_fmt_is_planar( pdata->avoutframe->format ) )
		{
			int stride = bufsize / *channels;
			int i = 0;
			for( i = 0; i < * channels; i++ )
			{
				memcpy( (uint8_t*)*buffer + stride * i,
						pdata->avoutframe->extended_data[i],
						stride );
			}
		}
		else
		{
			memcpy( (uint8_t*)*buffer,
					pdata->avoutframe->extended_data[0],
					bufsize );
		}
	}

exit:
	av_frame_unref( pdata->avinframe );
	av_frame_unref( pdata->avoutframe );
	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
	return 0;
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = mlt_frame_pop_service( frame );
	private_data* pdata = (private_data*)filter->child;
	int64_t pos = get_position( filter, frame );
	mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
	int ret;

	mlt_log_debug(MLT_FILTER_SERVICE(filter), "position %"PRId64"\n", pos);
	if (mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "_yuv_only")) {
		*format = mlt_image_yuv422;
	} else {
		*format = get_supported_image_format(*format);
	}

	mlt_frame_get_image( frame, image, format, width, height, 0 );

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	if( pdata->reset || pdata->format != *format || pdata->width != *width || pdata->height != *height )
	{
		double scale = mlt_profile_scale_width(profile, *width);
		init_image_filtergraph( filter, *format, *width, *height, scale );
		pdata->reset = 0;
	}

	if( pdata->avfilter_graph )
	{
		pdata->avinframe->width = *width;
		pdata->avinframe->height = *height;
		pdata->avinframe->format = mlt_to_av_image_format( *format );
		pdata->avinframe->sample_aspect_ratio = (AVRational) {
			profile->sample_aspect_num, profile->frame_rate_den };
		pdata->avinframe->pts = pos;
		pdata->avinframe->interlaced_frame = !mlt_properties_get_int( frame_properties, "progressive" );
		pdata->avinframe->top_field_first = mlt_properties_get_int( frame_properties, "top_field_first" );
		pdata->avinframe->color_primaries = mlt_properties_get_int( frame_properties, "color_primaries" );
		pdata->avinframe->color_trc = mlt_properties_get_int( frame_properties, "color_trc" );
		av_frame_set_color_range( pdata->avinframe,
			mlt_properties_get_int( frame_properties, "full_luma" )? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG );

		switch (mlt_properties_get_int( frame_properties, "colorspace" ))
		{
		case 240:
			av_frame_set_colorspace( pdata->avinframe, AVCOL_SPC_SMPTE240M );
			break;
		case 601:
			av_frame_set_colorspace( pdata->avinframe, AVCOL_SPC_BT470BG );
			break;
		case 709:
			av_frame_set_colorspace( pdata->avinframe, AVCOL_SPC_BT709 );
			break;
		case 2020:
			av_frame_set_colorspace( pdata->avinframe, AVCOL_SPC_BT2020_NCL );
			break;
		case 2021:
			av_frame_set_colorspace( pdata->avinframe, AVCOL_SPC_BT2020_CL );
			break;
		}

		ret = av_frame_get_buffer( pdata->avinframe, 1 );
		if( ret < 0 ) {
			mlt_log_error( filter, "Cannot get in frame buffer\n" );
		}

		// Set up the input frame
		if( *format == mlt_image_yuv420p )
		{
			int i = 0;
			int p = 0;
			int widths[3] = { *width, *width / 2, *width / 2 };
			int heights[3] = { *height, *height / 2, *height / 2 };
			uint8_t* src = *image;
			for( p = 0; p < 3; p ++ )
			{
				uint8_t* dst = pdata->avinframe->data[p];
				for( i = 0; i < heights[p]; i ++ )
				{
					memcpy( dst, src, widths[p] );
					src += widths[p];
					dst += pdata->avinframe->linesize[p];
				}
			}
		}
		else
		{
			int i;
			uint8_t* src = *image;
			uint8_t* dst = pdata->avinframe->data[0];
			int stride = mlt_image_format_size( *format, *width, 0, NULL );
			for( i = 0; i < *height; i ++ )
			{
				memcpy( dst, src, stride );
				src += stride;
				dst += pdata->avinframe->linesize[0];
			}
		}

		// Run the frame through the filter graph
		ret = av_buffersrc_add_frame( pdata->avbuffsrc_ctx, pdata->avinframe );
		if( ret < 0 ) {
			mlt_log_error( filter, "Cannot add frame to buffer source\n" );
		}
		ret = av_buffersink_get_frame( pdata->avbuffsink_ctx, pdata->avoutframe );
		if( ret < 0 ) {
			mlt_log_error( filter, "Cannot get frame from buffer sink\n" );
		}

		// Sanity check the output frame
		if( *width != pdata->avoutframe->width ||
			*height != pdata->avoutframe->height )
		{
			mlt_log_error( filter, "Unexpected return format\n" );
			goto exit;
		}

		// Copy the filter output into the original buffer
		if( *format == mlt_image_yuv420p )
		{
			int i = 0;
			int p = 0;
			int widths[3] = { *width, *width / 2, *width / 2 };
			int heights[3] = { *height, *height / 2, *height / 2 };
			uint8_t* dst = *image;
			for ( p = 0; p < 3; p ++ )
			{
				uint8_t* src = pdata->avoutframe->data[p];
				for ( i = 0; i < heights[p]; i ++ )
				{
					memcpy( dst, src, widths[p] );
					dst += widths[p];
					src += pdata->avoutframe->linesize[p];
				}
			}
		}
		else
		{
			int i;
			uint8_t* dst = *image;
			uint8_t* src = pdata->avoutframe->data[0];
			int stride = mlt_image_format_size( *format, *width, 0, NULL );
			for( i = 0; i < *height; i ++ )
			{
				memcpy( dst, src, stride );
				dst += stride;
				src += pdata->avoutframe->linesize[0];
			}
		}
	}

exit:
	av_frame_unref( pdata->avinframe );
	av_frame_unref( pdata->avoutframe );
	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	private_data* pdata = (private_data*)filter->child;

	if( avfilter_pad_get_type( pdata->avfilter->inputs, 0 ) == AVMEDIA_TYPE_VIDEO )
	{
		mlt_frame_push_service( frame, filter );
		mlt_frame_push_get_image( frame, filter_get_image );
	}
	else if( avfilter_pad_get_type( pdata->avfilter->inputs, 0 ) == AVMEDIA_TYPE_AUDIO )
	{
		mlt_frame_push_audio( frame, filter );
		mlt_frame_push_audio( frame, filter_get_audio );
	}

	return frame;
}

/** Destructor for the filter.
*/

static void filter_close( mlt_filter filter )
{
	private_data* pdata = (private_data*)filter->child;

	if( pdata )
	{
		avfilter_graph_free( &pdata->avfilter_graph );
		av_frame_free( &pdata->avinframe );
		av_frame_free( &pdata->avoutframe );
		free( pdata );
	}
	filter->child = NULL;
	filter->close = NULL;
	filter->parent.close = NULL;
	mlt_service_close( &filter->parent );
}

/** Constructor for the filter.
*/

mlt_filter filter_avfilter_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	avfilter_register_all();

	if( pdata && id )
	{
		id += 9; // Move past "avfilter."
		pdata->avfilter = (AVFilter*)avfilter_get_by_name( id );
	}

	if( filter && pdata && pdata->avfilter )
	{
		pdata->avbuffsink_ctx = NULL;
		pdata->avbuffsrc_ctx = NULL;
		pdata->avfilter_ctx = NULL;
		pdata->avfilter_graph = NULL;
		pdata->avinframe = av_frame_alloc();
		pdata->avoutframe = av_frame_alloc();
		pdata->format = -1;
		pdata->width = -1;
		pdata->height = -1;
		pdata->reset = 1;

		filter->close = filter_close;
		filter->process = filter_process;
		filter->child = pdata;

		mlt_events_listen( MLT_FILTER_PROPERTIES(filter), filter, "property-changed", (mlt_listener)property_changed );

		mlt_properties param_name_map = mlt_properties_get_data(mlt_global_properties(), "avfilter.resolution_scale", NULL);
		if (param_name_map) {
			// Lookup my plugin in the map
			param_name_map = mlt_properties_get_data(param_name_map, id, NULL);
			mlt_properties_set_data(MLT_FILTER_PROPERTIES(filter), "_resolution_scale", param_name_map, 0, NULL, NULL);
		}

		mlt_properties yuv_only = mlt_properties_get_data(mlt_global_properties(), "avfilter.yuv_only", NULL);
		if (yuv_only) {
			if (mlt_properties_get(yuv_only, id)) {
				mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "_yuv_only", 1);
			}
		}
	}
	else
	{
		mlt_filter_close( filter );
		free( pdata );
	}

	return filter;
}
