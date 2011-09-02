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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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

#include "stabilize.h"
#include "transform_image.h"


static void serialize_vectors( StabData* self, mlt_position length )
{
	mlt_geometry g = mlt_geometry_init();

	if ( g )
	{
		struct mlt_geometry_item_s item;
		mlt_position i;

		// Initialize geometry item
		item.key = item.f[0] = item.f[1] = 1;
		item.f[2] = item.f[3] = item.f[4] = 1;

		tlist* dat=self->transs;
		for ( i = 0; i < length; i++ )
		{
			// Set the geometry item
			item.frame = i;
			if (dat && dat->data){
				Transform* t=dat->data;
				item.x=t->x;
				item.y=t->y;
				item.w=t->alpha;
				item.h=t->zoom;
				/*item.x = self->pos_h[i].x;
				item.y = self->pos_h[i].y;*/
				dat=dat->next;
			}
			// Add the geometry item
			mlt_geometry_insert( g, &item );
		}

		// Put the analysis results in a property
		mlt_geometry_set_length( g, length );
		mlt_properties_set( MLT_FILTER_PROPERTIES( (mlt_filter) self->parent ), "vectors", mlt_geometry_serialise( g ) );
		mlt_geometry_close( g );
	}
}

Transform* deserialize_vectors( char *vectors, mlt_position length )
{
	mlt_geometry g = mlt_geometry_init();
	Transform* tx=NULL;
	// Parse the property as a geometry
	if ( !mlt_geometry_parse( g, vectors, length, -1, -1 ) )
	{
		struct mlt_geometry_item_s item;
		int i;
		tx=malloc(sizeof(Transform)*length);
		memset(tx,sizeof(Transform)*length,0);
		// Copy the geometry items to a vc array for interp()
		for ( i = 0; i < length; i++ )
		{
			mlt_geometry_fetch( g, &item, i );
			/*self->pos_h[i].x = item.x;
			self->pos_h[i].y = item.y;*/
			Transform t;
			t.x=item.x;
			t.y=item.y;
			t.alpha=item.w;
			t.zoom=item.h;
			t.extra=0;
			tx[i]=t;
		}
		
	}
	else
	{
		//mlt_log_warning( MLT_FILTER_SERVICE(self->parent), "failed to parse vectors\n" );
	}

	// We are done with this mlt_geometry
	if ( g ) mlt_geometry_close( g );
	return tx;
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = mlt_frame_pop_service( frame );
	//*format = mlt_image_rgb24;
	*format = mlt_image_yuv420p;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	if ( !error && *image )
	{
		StabData* self = filter->child;
		mlt_position length = mlt_filter_get_length2( filter, frame );
		int h = *height;
		int w = *width;

		// Service locks are for concurrency control
		mlt_service_lock( MLT_FILTER_SERVICE( filter ) );
		if ( !self->initialized )
		{
			// Initialize our context
			self->initialized = 1;
			self->width=w;
			self->height=h;
			self->framesize=w*h* 3;//( mlt_image_format_size ( *format, w,h , 0) ; // 3/2 =1 too small
			printf("framesize = %d %d %d\n",mlt_image_format_size ( *format, w,h , 0) , w*h*3, w*h*3/2);
			stabilize_configure(self);
		/*	self->es = es_init( w, h );
			self->pos_i = (vc*) malloc( length * sizeof(vc) );
			self->pos_h = (vc*) malloc( length * sizeof(vc) );
			self->pos_y = (vc*) malloc( h * sizeof(vc) );
			self->rs = rs_init( w, h );
			*/
		}
		char *vectors = mlt_properties_get( MLT_FILTER_PROPERTIES(filter), "vectors" );
		if ( !vectors )
		{
			// Analyse
			mlt_position pos = mlt_filter_get_position( filter, frame );
			//self->pos_i[pos] = vc_add( pos == 0 ? vc_zero() : self->pos_i[pos - 1], es_estimate( self->es, *image ) );
			stabilize_filter_video ( self, *image, (*format==mlt_image_rgb24?0:1) );


			// On last frame
			if ( pos == length - 1 )
			{
				//mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE(filter) );
				//double fps =  mlt_profile_fps( profile );

				// Filter and store the results
				//hipass( self->pos_i, self->pos_h, length, fps );
				serialize_vectors( self, length );
			}
		}
		if ( vectors )
		{
			// Apply
			TransformData* tf=mlt_properties_get_data( MLT_FILTER_PROPERTIES(filter), "_transformdata", NULL);
			if (!tf){
				tf=mlt_pool_alloc(sizeof(TransformData));
				mlt_properties_set_data( MLT_FILTER_PROPERTIES(filter), "_transformdata", tf, 0, ( mlt_destructor )mlt_properties_close, NULL );
			}
			if ( self->initialized != 2 )
			{
				// Load analysis results from property
				self->initialized = 2;
				transform_configure(tf,w,h,(*format==mlt_image_rgb24?0:1) ,*image, deserialize_vectors(  vectors, length ),length);
				
			}
			if ( self->initialized == 2 )
			{
				// Stabilize
				float pos = mlt_filter_get_position( filter, frame );
				tf->current_trans=pos;
				transform_filter_video(tf, *image,(*format==mlt_image_rgb24?0:1));
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

static void filter_close( mlt_filter parent )
{
	printf("filter close \n");
	StabData* self = parent->child;
	stabilize_stop(self);
	/*if ( self->es ) es_free( self->es );
	if ( self->pos_i ) free( self->pos_i );
	if ( self->pos_h ) free( self->pos_h );
	if ( self->pos_y ) free( self->pos_y );
	if ( self->rs ) rs_free( self->rs );
	free_lanc_kernels();*/
	free( self );
	parent->close = NULL;
	parent->child = NULL;
}

mlt_filter filter_videostab2_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	StabData* self = calloc( 1, sizeof(StabData) );
	if ( self )
	{
		mlt_filter parent = mlt_filter_new();
		parent->child = self;
		parent->close = filter_close;
		parent->process = filter_process;
		self->parent = parent;
		stabilize_init(self);
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "shutterangle", "0" ); // 0 - 180 , default 0
		//prepare_lanc_kernels();
		return parent;
	}
	return NULL;
}
