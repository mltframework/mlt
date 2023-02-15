/*
 * link_avdeinterlace.c
 * Copyright (C) 2023 Meltytech, LLC
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "common.h"

#include <framework/mlt_link.h>
#include <framework/mlt_log.h>
#include <framework/mlt_frame.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>

#include <stdlib.h>

// Private Types
#define FRAME_CACHE_SIZE 2

typedef struct
{
	// Used by get_frame
	mlt_frame frame_cache[FRAME_CACHE_SIZE];
	mlt_filter rescale_filter;
	mlt_filter resize_filter;
	// Used by image
	mlt_position expected_frame;
	mlt_position continuity_frame;
	AVFilterContext* avbuffsink_ctx;
	AVFilterContext* avbuffsrc_ctx;
	AVFilterGraph* avfilter_graph;
	AVFrame* avinframe;
	AVFrame* avoutframe;
	mlt_deinterlace_method method;
	int informat;
	int inwidth;
	int inheight;
	int outformat;
	int scan_mode_detected;
	// Used by get_frame and get_image
	int deinterlace_required;
} private_data;

static void stash_consumer_properties( mlt_frame frame )
{
	// Skip the deinterlacer and scaler normalizers to get original frames
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	mlt_properties_set( frame_properties, "_save.consumer.rescale", mlt_properties_get( frame_properties, "consumer.rescale" ) );
	mlt_properties_set( frame_properties, "consumer.rescale", "none" );
	mlt_properties_set( frame_properties, "_save.consumer.deinterlacer", mlt_properties_get( frame_properties, "consumer.deinterlacer" ) );
	mlt_properties_set( frame_properties, "consumer.deinterlacer", "none" );
	mlt_properties_set( frame_properties, "_save.consumer.progressive", mlt_properties_get( frame_properties, "consumer.progressive" ) );
	mlt_properties_clear( frame_properties, "consumer.progressive" );
}

static void apply_consumer_properties( mlt_frame frame )
{
	// Restore the consumer requests
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	mlt_properties_set( frame_properties, "consumer.rescale", mlt_properties_get( frame_properties, "_save.consumer.rescale" ) );
	mlt_properties_clear( frame_properties, "_save.consumer.rescale" );
	mlt_properties_set( frame_properties, "consumer.deinterlacer", mlt_properties_get( frame_properties, "_save.consumer.deinterlacer" ) );
	mlt_properties_clear( frame_properties, "_save.consumer.deinterlacer" );
	mlt_properties_set( frame_properties, "consumer.progressive", mlt_properties_get( frame_properties, "_save.consumer.progressive" ) );
	mlt_properties_clear( frame_properties, "_save.consumer.progressive" );
}

static void init_image_filtergraph( mlt_link self,AVRational sar )
{
	private_data* pdata = (private_data*)self->child;
	mlt_profile profile = mlt_service_profile(MLT_LINK_SERVICE(self));
	const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
	const AVFilter *buffersink = avfilter_get_by_name("buffersink");
	enum AVPixelFormat in_pixel_fmts[] = { -1, -1 };
	enum AVPixelFormat out_pixel_fmts[] = { -1, -1 };
	AVRational timebase = (AVRational){ profile->frame_rate_den, profile->frame_rate_num };
	AVRational framerate = (AVRational){ profile->frame_rate_num, profile->frame_rate_den };
	AVFilterContext* prev_ctx = NULL;
	AVFilterContext* avfilter_ctx = NULL;
	int ret;

	if ( !pdata->avinframe )
		pdata->avinframe = av_frame_alloc();
	if ( !pdata->avoutframe )
		pdata->avoutframe = av_frame_alloc();

	// Set up formats
	in_pixel_fmts[0] = mlt_to_av_image_format( pdata->informat );
	out_pixel_fmts[0] = mlt_to_av_image_format( pdata->outformat );

	// Destroy the current filter graph
	avfilter_graph_free( &pdata->avfilter_graph );

	// Create the new filter graph
	pdata->avfilter_graph = avfilter_graph_alloc();
	if( !pdata->avfilter_graph ) {
		mlt_log_error( self, "Cannot create filter graph\n" );
		goto fail;
	}
	pdata->avfilter_graph->scale_sws_opts = av_strdup("flags=" MLT_SWS_FLAGS);

	// Initialize the buffer source filter context
	pdata->avbuffsrc_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, buffersrc, "in");
	if( !pdata->avbuffsrc_ctx ) {
		mlt_log_error( self, "Cannot create image buffer source\n" );
		goto fail;
	}
	ret = av_opt_set_int( pdata->avbuffsrc_ctx, "width", pdata->inwidth, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( self, "Cannot set src width %d\n", pdata->inwidth );
		goto fail;
	}
	ret = av_opt_set_int( pdata->avbuffsrc_ctx, "height", pdata->inheight, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( self, "Cannot set src height %d\n", pdata->inheight );
		goto fail;
	}
	ret = av_opt_set_pixel_fmt( pdata->avbuffsrc_ctx, "pix_fmt", in_pixel_fmts[0], AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( self, "Cannot set src pixel format %d\n", in_pixel_fmts[0] );
		goto fail;
	}
	ret = av_opt_set_q( pdata->avbuffsrc_ctx, "sar", sar, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( self, "Cannot set src sar %d/%d\n", sar.num, sar.den );
		goto fail;
	}
	ret = av_opt_set_q( pdata->avbuffsrc_ctx, "time_base", timebase, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( self, "Cannot set src time_base %d/%d\n", timebase.num, timebase.den );
		goto fail;
	}
	ret = av_opt_set_q( pdata->avbuffsrc_ctx, "frame_rate", framerate, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( self, "Cannot set src frame_rate %d/%d\n", framerate.num, framerate.den );
		goto fail;
	}
	ret = avfilter_init_str( pdata->avbuffsrc_ctx, NULL );
	if( ret < 0 ) {
		mlt_log_error( self, "Cannot init buffer source\n" );
		goto fail;
	}
	prev_ctx = pdata->avbuffsrc_ctx;

	// Initialize the deinterlace filter context
	if ( pdata->method <= mlt_deinterlace_method_onefield )
	{
		const AVFilter* deint = (AVFilter*)avfilter_get_by_name( "field" );
		avfilter_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, deint, deint->name );
		if( !avfilter_ctx ) {
			mlt_log_error( self, "Cannot create field filter\n" );
			goto fail;
		}
		ret = avfilter_init_str( avfilter_ctx, NULL );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot init filter: %s\n", av_err2str(ret) );
			goto fail;
		}
		ret = avfilter_link( prev_ctx, 0, avfilter_ctx, 0 );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot link field filter\n" );
			goto fail;
		}
		prev_ctx = avfilter_ctx;
		const AVFilter* scale = (AVFilter*)avfilter_get_by_name( "scale" );
		avfilter_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, scale, scale->name );
		if( !avfilter_ctx ) {
			mlt_log_error( self, "Cannot create scale filter\n" );
			goto fail;
		}
		ret = avfilter_init_str( avfilter_ctx, "w=iw:h=2*ih" );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot init scale filter: %s\n", av_err2str(ret) );
			goto fail;
		}
		ret = avfilter_link( prev_ctx, 0, avfilter_ctx, 0 );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot link scale filter\n" );
			goto fail;
		}
		prev_ctx = avfilter_ctx;
	}
	else if ( pdata->method <= mlt_deinterlace_method_linearblend )
	{
		const AVFilter* deint = (AVFilter*)avfilter_get_by_name( "pp" );
		avfilter_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, deint, deint->name );
		if( !avfilter_ctx ) {
			mlt_log_error( self, "Cannot create video filter\n" );
			goto fail;
		}
		ret = avfilter_init_str( avfilter_ctx, "subfilters=lb" );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot init filter: %s\n", av_err2str(ret) );
			goto fail;
		}
		ret = avfilter_link( prev_ctx, 0, avfilter_ctx, 0 );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot link deinterlace filter\n" );
			goto fail;
		}
		prev_ctx = avfilter_ctx;
	}
	else if ( pdata->method <= mlt_deinterlace_method_yadif_nospatial )
	{
		const AVFilter* deint = (AVFilter*)avfilter_get_by_name( "yadif" );
		avfilter_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, deint, deint->name );
		if( !avfilter_ctx ) {
			mlt_log_error( self, "Cannot create yadif filter\n" );
			goto fail;
		}
		ret = avfilter_init_str( avfilter_ctx, "mode=send_frame_nospatial:parity=auto:deint=all" );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot init yadif filter: %s\n", av_err2str(ret) );
			goto fail;
		}
		ret = avfilter_link( prev_ctx, 0, avfilter_ctx, 0 );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot link yadif filter\n" );
			goto fail;
		}
		prev_ctx = avfilter_ctx;
	} else if ( pdata->method <= mlt_deinterlace_method_yadif ) {
		const AVFilter* deint = (AVFilter*)avfilter_get_by_name( "yadif" );
		avfilter_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, deint, deint->name );
		if( !avfilter_ctx ) {
			mlt_log_error( self, "Cannot create yadif filter\n" );
			goto fail;
		}
		ret = avfilter_init_str( avfilter_ctx, "mode=send_frame:parity=auto:deint=all" );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot init yadif filter: %s\n", av_err2str(ret) );
			goto fail;
		}
		ret = avfilter_link( prev_ctx, 0, avfilter_ctx, 0 );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot link yadif filter\n" );
			goto fail;
		}
		prev_ctx = avfilter_ctx;
	} else {
		const AVFilter* deint = (AVFilter*)avfilter_get_by_name( "bwdif" );
		avfilter_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, deint, deint->name );
		if( !avfilter_ctx ) {
			mlt_log_error( self, "Cannot create bwdif filter\n" );
			goto fail;
		}
		ret = avfilter_init_str( avfilter_ctx, "mode=send_frame:parity=auto:deint=all" );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot init bwdif filter: %s\n", av_err2str(ret) );
			goto fail;
		}
		ret = avfilter_link( prev_ctx, 0, avfilter_ctx, 0 );
		if( ret < 0 ) {
			mlt_log_error( self, "Cannot link bwdif filter\n" );
			goto fail;
		}
		prev_ctx = avfilter_ctx;
	}

	// Initialize the buffer sink filter context
	pdata->avbuffsink_ctx = avfilter_graph_alloc_filter( pdata->avfilter_graph, buffersink, "out");
	if( !pdata->avbuffsink_ctx ) {
		mlt_log_error( self, "Cannot create image buffer sink\n" );
		goto fail;
	}
	ret = av_opt_set_int_list( pdata->avbuffsink_ctx, "pix_fmts", out_pixel_fmts, -1, AV_OPT_SEARCH_CHILDREN );
	if( ret < 0 ) {
		mlt_log_error( self, "Cannot set sink pixel formats %d\n", out_pixel_fmts[0] );
		goto fail;
	}
	ret = avfilter_init_str(  pdata->avbuffsink_ctx, NULL );
	if( ret < 0 ) {
		mlt_log_error( self, "Cannot init buffer sink\n" );
		goto fail;
	}
	ret = avfilter_link( prev_ctx, 0, pdata->avbuffsink_ctx, 0 );
	if( ret < 0 ) {
		mlt_log_error( self, "Cannot link filter to sink\n" );
		goto fail;
	}

	// Configure the graph.
	ret = avfilter_graph_config( pdata->avfilter_graph, NULL );
	if( ret < 0 ) {
		mlt_log_error( self, "Cannot configure the filter graph\n" );
		goto fail;
	}

	return;

fail:
	avfilter_graph_free( &pdata->avfilter_graph );
}

static void link_configure( mlt_link self, mlt_profile chain_profile )
{
	private_data* pdata = (private_data*)self->child;

	// Operate at the same frame rate as the next link
	mlt_service_set_profile( MLT_LINK_SERVICE( self ), mlt_service_profile( MLT_PRODUCER_SERVICE( self->next ) ) );

	// The source may have changed so restart sending future frames.
	pdata->deinterlace_required = 1;
	pdata->scan_mode_detected = 0;
}

static int link_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	int ret = 0;
	mlt_link self = (mlt_link)mlt_frame_pop_service( frame );
	private_data* pdata = (private_data*)self->child;
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	struct mlt_image_s srcimg = {0};
	struct mlt_image_s dstimg = {0};
	mlt_deinterlace_method method = mlt_deinterlace_id( mlt_properties_get( frame_properties, "consumer.deinterlacer" ) );

	if ( !mlt_properties_get_int( frame_properties, "consumer.progressive" ) ||
		 method == mlt_deinterlace_method_none )
	{
		mlt_log_info( MLT_LINK_SERVICE(self), "Do not deinterlace - consumer is interlaced\n" );
		pdata->deinterlace_required = 0;
		return mlt_frame_get_image( frame, image, format, width, height, writable );
	}

	if ( mlt_frame_is_test_card( frame ) )
	{
		// The producer provided a test card. Do not deinterlace it.
		// Do not set deinterlace_required = 0 because the producer could provide interlaced frames later.
		mlt_log_debug( MLT_LINK_SERVICE(self), "Do not deinterlace - test frame\n" );
		return mlt_frame_get_image( frame, image, format, width, height, writable );
	}

	if ( !pdata->scan_mode_detected )
	{
		// Some producers do not set the progressive flag until get_image() has been called.
		mlt_image_format outformat = *format;
		mlt_image_set_values( &srcimg, *image, *format, *width, *height );
		stash_consumer_properties( frame );
		error = mlt_frame_get_image( frame, (uint8_t**)&srcimg.data, &srcimg.format, &srcimg.width, &srcimg.height, writable );
		apply_consumer_properties( frame );
		if ( error )
		{
			mlt_log_error( MLT_LINK_SERVICE(self), "Failed to get image\n" );
			return error;
		}
		mlt_log_info( MLT_LINK_SERVICE(self), "Requested: %dx%d (%s) | Received: %dx%d (%s)\n", *width, *height, mlt_image_format_name( *format ), srcimg.width, srcimg.height, mlt_image_format_name( srcimg.format ) );

		pdata->informat = srcimg.format;
		pdata->inwidth = srcimg.width;
		pdata->inheight = srcimg.height;
		if (outformat == mlt_image_none || outformat >= mlt_image_invalid)
			outformat = srcimg.format;
		pdata->outformat = outformat;

		int progressive_source = mlt_properties_get_int( frame_properties, "progressive" );
		pdata->scan_mode_detected = 1;
		char *scan_mode = progressive_source ? "progressive" : "interlaced";
		mlt_log_info( MLT_LINK_SERVICE(self), "Scan mode detected: %s\n", scan_mode );
		if ( progressive_source )
		{
			mlt_log_error( MLT_LINK_SERVICE(self), "Do not deinterlace - progressive source\n" );
			pdata->deinterlace_required = 0;
			mlt_image_get_values( &srcimg, (void**)image, format, width, height );
			return error;
		}
	}

	mlt_service_lock( MLT_LINK_SERVICE( self ) );

	if( !pdata->avfilter_graph ||
		pdata->method != method ||
		pdata->expected_frame != mlt_frame_get_position( frame ) )
	{
		pdata->method = method;
		pdata->continuity_frame = mlt_frame_get_position( frame );
		pdata->expected_frame = mlt_frame_get_position( frame );
		init_image_filtergraph( self, av_d2q(mlt_frame_get_aspect_ratio(frame), 1024) );
	}
	pdata->expected_frame++;

	if( pdata->avfilter_graph )
	{
		while (1)
		{
			mlt_frame src_frame = NULL;

			if ( pdata->continuity_frame == mlt_frame_get_position( frame ) ) {
				src_frame = frame;
				pdata->continuity_frame++;
			} else {
				mlt_properties unique_properties = mlt_frame_get_unique_properties( frame, MLT_LINK_SERVICE(self) );
				if ( !unique_properties )
				{
					error = 1;
					break;
				}
				char key[19];
				sprintf( key, "%d", pdata->continuity_frame );
				src_frame = (mlt_frame)mlt_properties_get_data( unique_properties, key, NULL );
				if ( !src_frame )
				{
					mlt_log_error( MLT_LINK_SERVICE(self), "Frame not found: %s\n", key );
					error = 1;
					break;
				}
				pdata->continuity_frame++;
			}

			if ( !srcimg.data ) { // Maybe already received during progressive check
				mlt_image_set_values( &srcimg, *image, *format, *width, *height );
				stash_consumer_properties( src_frame );
				error = mlt_frame_get_image( src_frame, (uint8_t**)&srcimg.data, &srcimg.format, &srcimg.width, &srcimg.height, 0 );
				apply_consumer_properties( src_frame );
				if ( error )
				{
					mlt_log_error( MLT_LINK_SERVICE(self), "Failed to get image\n" );
					break;
				}
			}

			mlt_image_to_avframe( &srcimg, src_frame, pdata->avinframe );

			// Run the frame through the filter graph
			ret = av_buffersrc_add_frame( pdata->avbuffsrc_ctx, pdata->avinframe );
			av_frame_unref( pdata->avinframe );
			if( ret < 0 ) {
				mlt_log_error( self, "Cannot add frame to buffer source\n" );
				error = 1;
				break;
			}
			ret = av_buffersink_get_frame( pdata->avbuffsink_ctx, pdata->avoutframe );
			if ( ret >= 0 )
				break;
			else if ( ret == AVERROR(EAGAIN) )
				continue;
			else if ( ret < 0 ) {
				mlt_log_error( self, "Cannot get frame from buffer sink\n" );
				error = 1;
				break;
			}
			srcimg.data = NULL;
		}

		if ( !error ) {
			// Allocate the output image
			mlt_image_set_values( &dstimg, NULL, pdata->outformat, pdata->avoutframe->width, pdata->avoutframe->height );
			mlt_image_alloc_data( &dstimg );
			avframe_to_mlt_image( pdata->avoutframe, &dstimg );
		}
		av_frame_unref( pdata->avoutframe );
	}

	mlt_service_unlock( MLT_LINK_SERVICE( self ) );
	mlt_image_get_values( &dstimg, (void**)image, format, width, height );
	mlt_frame_set_image( frame, dstimg.data, 0, dstimg.release_data );
	mlt_properties_set_int( frame_properties, "progressive", 1 );
	return 0;
}

static int link_get_frame( mlt_link self, mlt_frame_ptr frame, int index )
{
	int error = 0;
	private_data* pdata = (private_data*)self->child;
	mlt_position frame_pos = mlt_producer_position( MLT_LINK_PRODUCER(self) );

	if ( !pdata->deinterlace_required )
	{
		// As soon as get_image() detects that deinterlace is not required we stop passing
		// future frames and stop calling get_image().
		// This can not be undone. So a source can not toggle between interlaced/progressive.
		mlt_producer_seek( self->next, frame_pos );
		error = mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), frame, index );
		mlt_producer_prepare_next( MLT_LINK_PRODUCER( self ) );
		return error;
	}

	// Cycle out the first frame in the cache
	mlt_frame_close( pdata->frame_cache[0] );
	// Shift all the frames or get new
	int i = 0;
	for ( i = 0; i < FRAME_CACHE_SIZE - 1; i++ )
	{
		mlt_position pos = frame_pos + i;
		mlt_frame next_frame = pdata->frame_cache[ i + 1 ];
		if ( next_frame && mlt_frame_get_position( next_frame ) == pos )
		{
			// Shift the frame if it is correct
			pdata->frame_cache[ i ] = next_frame;
		}
		else
		{
			// Get a new frame if the next cache frame is not the needed frame
			mlt_frame_close( next_frame );
			mlt_producer_seek( self->next, pos );
			error = mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), &pdata->frame_cache[ i ], index );
			if ( error )
			{
				mlt_log_error( MLT_LINK_SERVICE(self), "Error getting frame: %d\n", (int)pos );
			}
		}
	}
	// Get the new last frame in the cache
	mlt_producer_seek( self->next, frame_pos + i );
	error = mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), &pdata->frame_cache[ i ], index );
	if ( error )
	{
		mlt_log_error( MLT_LINK_SERVICE(self), "Error getting frame: %d\n", (int)frame_pos + i );
	}

	*frame = pdata->frame_cache[0];
	mlt_properties_inc_ref( MLT_FRAME_PROPERTIES(*frame) );

	// Attach the next frames to the current frame in case they are needed for deinterlacing
	mlt_properties unique_properties = mlt_frame_unique_properties( *frame, MLT_LINK_SERVICE(self) );
	for ( int i = 1; i < FRAME_CACHE_SIZE; i++ )
	{
		char key[19];
		sprintf( key, "%d", (int)mlt_frame_get_position( pdata->frame_cache[i] ) );
		mlt_properties_inc_ref( MLT_FRAME_PROPERTIES(pdata->frame_cache[i]) );
		mlt_properties_set_data( unique_properties, key, pdata->frame_cache[i], 0, (mlt_destructor)mlt_frame_close, NULL );
	}

	mlt_frame_push_service( *frame, self );
	mlt_frame_push_get_image( *frame, link_get_image );
	mlt_producer_prepare_next( MLT_LINK_PRODUCER( self ) );

	// This link will request the native image. Add normalizers to be applied after this link.
	if ( !pdata->rescale_filter )
	{
		pdata->rescale_filter = mlt_factory_filter( mlt_service_profile( MLT_LINK_SERVICE( self ) ), "swscale", NULL );
	}
	if ( pdata->rescale_filter )
	{
		mlt_filter_process( pdata->rescale_filter, *frame );
	}
	if ( !pdata->resize_filter )
	{
		pdata->resize_filter = mlt_factory_filter( mlt_service_profile( MLT_LINK_SERVICE( self ) ), "resize", NULL );
	}
	if ( pdata->resize_filter )
	{
		mlt_filter_process( pdata->resize_filter, *frame );
	}

	return error;
}

static void link_close( mlt_link self )
{
	if ( self )
	{
		private_data* pdata = (private_data*)self->child;
		if ( pdata )
		{
			for ( int i = 0; i < FRAME_CACHE_SIZE; i++ )
			{
				mlt_frame_close( pdata->frame_cache[ i ] );
			}
			avfilter_graph_free( &pdata->avfilter_graph );
			av_frame_free( &pdata->avinframe );
			av_frame_free( &pdata->avoutframe );
			mlt_filter_close( pdata->rescale_filter );
			mlt_filter_close( pdata->resize_filter );
			free( pdata );
		}
		self->close = NULL;
		self->child = NULL;
		mlt_link_close( self );
		free( self );
	}
}

mlt_link link_avdeinterlace_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_link self = mlt_link_init();
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if ( self && pdata )
	{
		pdata->continuity_frame = -1;
		pdata->expected_frame = -1;
		pdata->deinterlace_required = 1;
		pdata->method = mlt_deinterlace_method_linearblend;
		self->child = pdata;

		// Callback registration
		self->configure = link_configure;
		self->get_frame = link_get_frame;
		self->close = link_close;
	}
	else
	{
		free( pdata );
		mlt_link_close( self );
		self = NULL;
	}

	return self;
}
