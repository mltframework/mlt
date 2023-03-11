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
	// Used by get_frame and get_image
	int prev_next_required;
} private_data;

static void link_configure( mlt_link self, mlt_profile chain_profile )
{
	private_data* pdata = (private_data*)self->child;

	// Operate at the same frame rate as the next link
	mlt_service_set_profile( MLT_LINK_SERVICE( self ), mlt_service_profile( MLT_PRODUCER_SERVICE( self->next ) ) );
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
		 method == mlt_deinterlacer_none ||
		 mlt_frame_is_test_card( frame ) )
	{
		mlt_log_debug( MLT_LINK_SERVICE(self), "Do not deinterlace\n" );
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
		error = mlt_frame_get_image( frame, (uint8_t**)&srcimg.data, &srcimg.format, &srcimg.width, &srcimg.height, 0 );
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
			error = mlt_frame_get_image( prevframe, (uint8_t**)&previmg.data, &previmg.format, &previmg.width, &previmg.height, 0 );
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
			error = mlt_frame_get_image( nextframe, (uint8_t**)&nextimg.data, &nextimg.format, &nextimg.width, &nextimg.height, 0 );
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

	mlt_producer_seek( self->next, frame_pos );
	error = mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), frame, index );
	mlt_producer original_producer = mlt_frame_get_original_producer( *frame );
	mlt_producer_probe( original_producer );

	if ( mlt_properties_get_int( MLT_PRODUCER_PROPERTIES(original_producer), "meta.media.progressive" ) )
	{
		// Source is progressive. Deinterlace is not required;
		return error;
	}

	mlt_frame prev = NULL;
	mlt_frame next = NULL;

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

	return error;
}

static void link_close( mlt_link self )
{
	if ( self )
	{
		private_data* pdata = (private_data*)self->child;
		free( pdata );
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
