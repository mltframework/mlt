/*
 * filter_imagestab.c -- video stabilization with code from http://vstab.sourceforge.net/
 * Copyright (c) 2011 Marco Gittler <g.marco@freenet.de>
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_geometry.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <string.h>

#include "stab/vector.h"
#include "stab/utils.h"
#include "stab/estimate.h"
#include "stab/resample.h"


typedef struct {
	mlt_filter parent;
	int initialized;
	int* lanc_kernels;
	es_ctx *es;
	vc *pos_i;
	vc *pos_h;
	vc *pos_y;
	rs_ctx *rs;
} *videostab;

static void serialize_vectors( videostab self, mlt_position length )
{
	mlt_geometry g = mlt_geometry_init();

	if ( g )
	{
		struct mlt_geometry_item_s item;
		int i;

		// Initialize geometry item
		item.key = item.f[0] = item.f[1] = 1;
		item.f[2] = item.f[3] = item.f[4] = 0;

		for ( i = 0; i < length; i++ )
		{
			// Set the geometry item
			item.frame = i;
			item.x = self->pos_h[i].x;
			item.y = self->pos_h[i].y;

			// Add the geometry item
			mlt_geometry_insert( g, &item );
		}

		// Put the analysis results in a property
		mlt_geometry_set_length( g, length );
		mlt_properties_set_data( MLT_FILTER_PROPERTIES( self->parent ), "vectors", g, 0,
			(mlt_destructor) mlt_geometry_close, (mlt_serialiser) mlt_geometry_serialise );
	}
}

static void deserialize_vectors( videostab self, char *vectors, mlt_position length )
{
	mlt_geometry g = mlt_geometry_init();

	// Parse the property as a geometry
	if ( g && !mlt_geometry_parse( g, vectors, length, -1, -1 ) )
	{
		struct mlt_geometry_item_s item;
		int i;

		// Copy the geometry items to a vc array for interp()
		for ( i = 0; i < length; i++ )
		{
			mlt_geometry_fetch( g, &item, i );
			self->pos_h[i].x = item.x;
			self->pos_h[i].y = item.y;
		}
	}
	else
	{
		mlt_log_warning( MLT_FILTER_SERVICE(self->parent), "failed to parse vectors\n" );
	}

	// We are done with this mlt_geometry
	if ( g ) mlt_geometry_close( g );
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = mlt_frame_pop_service( frame );
	*format = mlt_image_rgb24;
	mlt_properties_set_int( MLT_FRAME_PROPERTIES(frame), "consumer_deinterlace", 1 );
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	if ( !error && *image )
	{
		videostab self = filter->child;
		mlt_position length = mlt_filter_get_length2( filter, frame );
		int h = *height;
		int w = *width;

		// Service locks are for concurrency control
		mlt_service_lock( MLT_FILTER_SERVICE( filter ) );
		if ( !self->initialized )
		{
			// Initialize our context
			self->initialized = 1;
			self->es = es_init( w, h );
			self->pos_i = (vc*) malloc( length * sizeof(vc) );
			self->pos_h = (vc*) malloc( length * sizeof(vc) );
			self->pos_y = (vc*) malloc( h * sizeof(vc) );
			self->rs = rs_init( w, h );
		}
		char *vectors = mlt_properties_get( MLT_FILTER_PROPERTIES(filter), "vectors" );
		if ( !vectors )
		{
			// Analyse
			int pos = (int) mlt_filter_get_position( filter, frame );
			self->pos_i[pos] = vc_add( pos == 0 ? vc_zero() : self->pos_i[pos - 1], es_estimate( self->es, *image ) );

			// On last frame
			if ( pos == length - 1 )
			{
				mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE(filter) );
				double fps =  mlt_profile_fps( profile );

				// Filter and store the results
				hipass( self->pos_i, self->pos_h, length, fps );
				serialize_vectors( self, length );
			}
		} else {
			// Apply
			if ( self->initialized != 2 )
			{
				// Load analysis results from property
				self->initialized = 2;
				deserialize_vectors( self, vectors, length );
			}
			if ( self->initialized == 2 )
			{
				// Stabilize
				float shutter_angle = mlt_properties_get_double( MLT_FRAME_PROPERTIES(frame) , "shutterangle" );
				float pos = mlt_filter_get_position( filter, frame );
				int i;

				for (i = 0; i < h; i ++)
					self->pos_y[i] = interp( self->lanc_kernels,self->pos_h, length, pos + (i - h / 2.0) * shutter_angle / (h * 360.0) );
				rs_resample( self->lanc_kernels,self->rs, *image, self->pos_y );
			}
		}
		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
	}
	return error;
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

void filter_close( mlt_filter parent )
{
	videostab self = parent->child;
	if ( self->es ) es_free( self->es );
	free( self->pos_i );
	free( self->pos_h );
	free( self->pos_y );
	if ( self->rs ) rs_free( self->rs );
	if ( self->lanc_kernels) free_lanc_kernels(self->lanc_kernels);
	free( self );
	parent->close = NULL;
	parent->child = NULL;
}

mlt_filter filter_videostab_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	videostab self = calloc( 1, sizeof(*self) );
	if ( self )
	{
		mlt_filter parent = mlt_filter_new();
		if ( !parent )
		{
			free( self );
			return NULL;
		}
		parent->child = self;
		parent->close = filter_close;
		parent->process = filter_process;
		self->parent = parent;
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "shutterangle", "0" ); // 0 - 180 , default 0
		self->lanc_kernels=prepare_lanc_kernels();
		return parent;
	}
	return NULL;
}
