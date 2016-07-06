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

#include "stabilize.h"
#include "transform_image.h"

typedef struct {
	StabData* stab;
	TransformData* trans;
	int initialized;
	void* parent;
} videostab2_data;

static void serialize_vectors( videostab2_data* self, mlt_position length )
{
	mlt_geometry g = mlt_geometry_init();
	if ( g )
	{
		struct mlt_geometry_item_s item;
		mlt_position i;

		// Initialize geometry item
		item.key = item.f[0] = item.f[1] = item.f[2] = item.f[3] = 1;
		item.f[4] = 0;

		tlist* transform_data =self->stab->transs;
		for ( i = 0; i < length; i++ )
		{
			// Set the geometry item
			item.frame = i;
			if (transform_data){
				if ( transform_data->data){
					Transform* t=transform_data->data;
					item.x=t->x;
					item.y=t->y;
					item.w=t->alpha;
					item.h=t->zoom;
					transform_data=transform_data->next;
				}
			}
			// Add the geometry item
			mlt_geometry_insert( g, &item );
		}

		// Put the analysis results in a property
		mlt_geometry_set_length( g, length );
		mlt_properties_set_data( MLT_FILTER_PROPERTIES( (mlt_filter) self->parent ), "vectors", g, 0,
			(mlt_destructor) mlt_geometry_close, (mlt_serialiser) mlt_geometry_serialise );
	}
}
// scale zoom implements the factor that the vetcors must be scaled since the vector is calulated for real with, now we need it for (scaled)width
Transform* deserialize_vectors( char *vectors, mlt_position length ,float scale_zoom )
{
	mlt_geometry g = mlt_geometry_init();
	Transform* tx=NULL;
	// Parse the property as a geometry
	if ( g && !mlt_geometry_parse( g, vectors, length, -1, -1 ) )
	{
		struct mlt_geometry_item_s item;
		int i;
		tx=calloc(1,sizeof(Transform)*length);
		// Copy the geometry items to a vc array for interp()
		for ( i = 0; i < length; i++ )
		{
			mlt_geometry_fetch( g, &item, i );
			Transform t;
			t.x=scale_zoom*item.x;
			t.y=scale_zoom*item.y;
			t.alpha=item.w;
			t.zoom=scale_zoom*item.h;
			t.extra=0;
			tx[i]=t;
		}

	}
	else
	{
		//mlt_log_warning( NULL, "failed to parse vectors\n" );
	}

	// We are done with this mlt_geometry
	if ( g ) mlt_geometry_close( g );
	return tx;
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = mlt_frame_pop_service( frame );
	char *vectors = mlt_properties_get( MLT_FILTER_PROPERTIES(filter), "vectors" );
	*format = mlt_image_yuv422;
	if (vectors)
		*format= mlt_image_rgb24;
	mlt_properties_set_int( MLT_FRAME_PROPERTIES(frame), "consumer_deinterlace", 1 );
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	if ( !error && *image )
	{
		videostab2_data* data = filter->child;
		if ( data==NULL ) { // big error, abort
			return 1;
		}
		mlt_position length = mlt_filter_get_length2( filter, frame );
		int h = *height;
		int w = *width;

		// Service locks are for concurrency control
		mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

		// Handle signal from app to re-init data
		if ( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter) , "refresh" ) )
		{
			mlt_properties_set( MLT_FILTER_PROPERTIES(filter) , "refresh", NULL );
			data->initialized = 0;
		}

		if ( !vectors) {
			if ( !data->initialized )
			{
				// Initialize our context
				data->initialized = 1;
				data->stab->width=w;
				data->stab->height=h;
				if (*format==mlt_image_yuv420p) data->stab->framesize=w*h* 3/2;//( mlt_image_format_size ( *format, w,h , 0) ; // 3/2 =1 too small
				if (*format==mlt_image_yuv422) data->stab->framesize=w*h;
				data->stab->shakiness = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter) , "shakiness" );
				data->stab->accuracy = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter) , "accuracy" );
				data->stab->stepsize = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter) , "stepsize" );
				data->stab->algo = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter) , "algo" );
				data->stab->show = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter) , "show" );
				data->stab->contrast_threshold = mlt_properties_get_double( MLT_FILTER_PROPERTIES(filter) , "mincontrast" );
				stabilize_configure(data->stab);
			}
				// Analyse
				mlt_position pos = mlt_filter_get_position( filter, frame );
				stabilize_filter_video ( data->stab , *image, *format );

				// On last frame
				if ( pos == length - 1 )
				{
					serialize_vectors( data , length );
				}
		}
		else
		{
			if ( data->initialized!=1  )
			{
				char *interps = mlt_properties_get( MLT_FRAME_PROPERTIES( frame ), "rescale.interp" );

				if ( data->initialized != 2 )
				{
					// Load analysis results from property
					data->initialized = 2;

					int interp = 2; // default to bilinear
					float scale_zoom=1.0;
					if ( *width != mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "meta.media.width" ) )
						scale_zoom = (float) *width / (float) mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "meta.media.width" );
					if ( strcmp( interps, "nearest" ) == 0 || strcmp( interps, "neighbor" ) == 0 )
						interp = 0;
					else if ( strcmp( interps, "tiles" ) == 0 || strcmp( interps, "fast_bilinear" ) == 0 )
						interp = 1;

					data->trans->interpoltype = interp;
					data->trans->smoothing = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "smoothing" );
					data->trans->maxshift = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "maxshift" );
					data->trans->maxangle = mlt_properties_get_double( MLT_FILTER_PROPERTIES(filter), "maxangle" );
					data->trans->crop = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "crop" );
					data->trans->invert = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "invert" );
					data->trans->relative = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "relative" );
					data->trans->zoom = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "zoom" );
					data->trans->optzoom = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "optzoom" );
					data->trans->sharpen = mlt_properties_get_double( MLT_FILTER_PROPERTIES(filter), "sharpen" );

					transform_configure(data->trans,w,h,*format ,*image, deserialize_vectors(  vectors, length , scale_zoom ),length);

				}
				if ( data->initialized == 2 )
				{
					// Stabilize
					float pos = mlt_filter_get_position( filter, frame );
					data->trans->current_trans=pos;
					transform_filter_video(data->trans, *image, *format );

				}
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
	videostab2_data* data = parent->child;
	if (data){
		if (data->stab) stabilize_stop(data->stab);
		if (data->trans){
			free(data->trans->src);
			free (data->trans);
		}
		free( data );
	}
	parent->close = NULL;
	parent->child = NULL;
}

mlt_filter filter_videostab2_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	videostab2_data* data= calloc( 1, sizeof(videostab2_data));
	if ( data )
	{
		data->stab = calloc( 1, sizeof(StabData) );
		if ( !data->stab )
		{
			free( data );
			return NULL;
		}

		data->trans = calloc( 1, sizeof (TransformData) ) ;
		if ( !data->trans )
		{
			free( data->stab );
			free( data );
			return NULL;
		}

		mlt_filter parent = mlt_filter_new();
		if ( !parent )
		{
			free( data->trans );
			free( data->stab );
			free( data );
			return NULL;
		}

		parent->child = data;
		parent->close = filter_close;
		parent->process = filter_process;
		data->parent = parent;
		//properties for stabilize
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "shakiness", "4" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "accuracy", "4" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "stepsize", "6" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "algo", "1" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "mincontrast", "0.3" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "show", "0" );

		//properties for transform
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "smoothing", "10" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "maxshift", "-1" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "maxangle", "-1" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "crop", "0" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "invert", "0" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "relative", "1" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "zoom", "0" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "optzoom", "1" );
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "sharpen", "0.8" );
		return parent;
	}
	return NULL;
}
