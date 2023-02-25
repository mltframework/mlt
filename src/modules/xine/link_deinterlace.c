/*
 * link_deinterlace.c
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
#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>

#include <stdlib.h>

// Private Types
typedef struct
{
	// Used by get_frame
	mlt_filter rescale_filter;
	mlt_filter resize_filter;
	// Used by image
	int scan_mode_detected;
	// Used by get_frame and get_image
	int deinterlace_required;
	int prev_next_required;
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
	struct mlt_image_s previmg = {0};
	struct mlt_image_s nextimg = {0};
	mlt_deinterlacer method = mlt_deinterlacer_id( mlt_properties_get( frame_properties, "consumer.deinterlacer" ) );

	if ( !mlt_properties_get_int( frame_properties, "consumer.progressive" ) ||
		 method == mlt_deinterlacer_none )
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

	// At this point, we know we need to deinterlace.
	method = supported_method( method );
	if ( method == mlt_deinterlacer_weave ||
		 method == mlt_deinterlacer_greedy ||
		 method >= mlt_deinterlacer_yadif_nospatial )
	{
		pdata->prev_next_required = 1;
	}

	if ( srcimg.data ) // Maybe already received during progressive check
	{
		if ( srcimg.format != mlt_image_yuv422 )
		{
			error = frame->convert_image( frame, (uint8_t**)&srcimg.data, &srcimg.format, mlt_image_yuv422 );
			if ( error )
			{
				mlt_log_error( MLT_LINK_SERVICE(self), "Failed to convert image\n" );
				return error;
			}
		}
	}
	else
	{
		mlt_image_set_values( &srcimg, NULL, mlt_image_yuv422, *width, *height );
		stash_consumer_properties( frame );
		error = mlt_frame_get_image( frame, (uint8_t**)&srcimg.data, &srcimg.format, &srcimg.width, &srcimg.height, 0 );
		apply_consumer_properties( frame );
		if ( error )
		{
			mlt_log_error( MLT_LINK_SERVICE(self), "Failed to get image\n" );
			return error;
		}
	}

	mlt_image_set_values( &dstimg, NULL, srcimg.format, srcimg.width, srcimg.height );
	mlt_image_alloc_data( &dstimg );

	if ( pdata->prev_next_required )
	{
		mlt_properties unique_properties = mlt_frame_unique_properties( frame, MLT_LINK_SERVICE(self) );

		mlt_frame prevframe = mlt_properties_get_data( unique_properties, "prev", NULL );
		if ( prevframe )
		{
			mlt_image_set_values( &previmg, NULL, mlt_image_yuv422, srcimg.width, srcimg.height );
			stash_consumer_properties( prevframe );
			error = mlt_frame_get_image( prevframe, (uint8_t**)&previmg.data, &previmg.format, &previmg.width, &previmg.height, 0 );
			apply_consumer_properties( prevframe );
			if ( error )
			{
				mlt_log_error( MLT_LINK_SERVICE(self), "Failed to get prev image\n" );
				previmg.data = NULL;
			}
		}
		mlt_frame nextframe = mlt_properties_get_data( unique_properties, "next", NULL );
		if ( nextframe )
		{
			mlt_image_set_values( &nextimg, NULL, mlt_image_yuv422, srcimg.width, srcimg.height );
			stash_consumer_properties( nextframe );
			error = mlt_frame_get_image( nextframe, (uint8_t**)&nextimg.data, &nextimg.format, &nextimg.width, &nextimg.height, 0 );
			apply_consumer_properties( nextframe );
			if ( error )
			{
				mlt_log_error( MLT_LINK_SERVICE(self), "Failed to get next image\n" );
				nextimg.data = NULL;
			}
		}
	}

	int tff = mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "top_field_first" );
	error = deinterlace_image( &dstimg, &srcimg, &previmg, &nextimg, tff, method );
	if ( error )
	{
		mlt_log_error( MLT_LINK_SERVICE(self), "Deinterlace failed\n" );
		return error;
	}

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
	mlt_frame prev = NULL;
	mlt_frame next = NULL;

	if ( !pdata->scan_mode_detected )
	{
		// If the producer is progressive, then this filter does not need to do anything.
		// This can not be undone. So a source can not toggle between interlaced/progressive.
		mlt_producer_seek( self->next, frame_pos );
		error = mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), frame, index );
		mlt_producer original_producer = mlt_frame_get_original_producer( *frame );
		mlt_producer_probe( original_producer );
		int progressive_source = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES(original_producer), "progressive" );
		pdata->scan_mode_detected = 1;
		char *scan_mode = progressive_source ? "progressive" : "interlaced";
		mlt_log_info( MLT_LINK_SERVICE(self), "Scan mode detected: %s\n", scan_mode );
		if ( progressive_source )
		{
			mlt_log_info( MLT_LINK_SERVICE(self), "Do not deinterlace - progressive source\n" );
			pdata->deinterlace_required = 0;
			mlt_producer_prepare_next( MLT_LINK_PRODUCER( self ) );
			return error;
		}
		else
		{
			// Proceed to setup the frame cache below
			mlt_frame_close(*frame);
		}
	}

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

	mlt_producer_seek( self->next, frame_pos );
	error = mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), frame, index );
	if ( error )
	{
		mlt_log_error( MLT_LINK_SERVICE(self), "Unable to get frame: %d\n", frame_pos );
		return error;
	}

	if ( pdata->prev_next_required )
	{
		mlt_properties unique_properties = mlt_frame_unique_properties( *frame, MLT_LINK_SERVICE(self) );
		mlt_producer_seek( self->next, frame_pos - 1 );
		error = mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), &prev, index );
		if ( error )
		{
			mlt_log_error( MLT_LINK_SERVICE(self), "Unable to get prev: %d\n", frame_pos );
		}
		mlt_properties_set_data( unique_properties, "prev", prev, 0, (mlt_destructor)mlt_frame_close, NULL );

		mlt_producer_seek( self->next, frame_pos + 1 );
		error = mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), &next, index );
		if ( error )
		{
			mlt_log_error( MLT_LINK_SERVICE(self), "Unable to get next: %d\n", frame_pos );
		}
		mlt_properties_set_data( unique_properties, "next", next, 0, (mlt_destructor)mlt_frame_close, NULL );
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

mlt_link link_deinterlace_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_link self = mlt_link_init();
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if ( self && pdata )
	{
		pdata->deinterlace_required = 1;
		pdata->prev_next_required = 1;
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
