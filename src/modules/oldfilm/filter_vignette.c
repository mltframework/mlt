/*
 * filter_vignette.c -- vignette filter
 * Copyright (c) 2007 Marco Gittler <g.marco@freenet.de>
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_geometry.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SIGMOD_STEPS 1000
#define POSITION_VALUE(p,s,e) (s+((double)(e-s)*p ))

static float geometry_to_float( char *val, mlt_position pos )
{
	float ret=0.0;
	struct mlt_geometry_item_s item;

	mlt_geometry geom = mlt_geometry_init();
	mlt_geometry_parse( geom, val, -1, -1, -1);
	mlt_geometry_fetch( geom, &item, pos );
	ret = item.x;
	mlt_geometry_close( geom );

	return ret;
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = mlt_frame_pop_service( frame );
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	if ( error == 0 && *image )
	{
		float smooth, radius, cx, cy, opac;
		mlt_properties filter_props = MLT_FILTER_PROPERTIES( filter ) ;
		mlt_position pos = mlt_filter_get_position( filter, frame );

		smooth = geometry_to_float( mlt_properties_get( filter_props, "smooth" ), pos ) * 100.0 ;
		radius = geometry_to_float( mlt_properties_get( filter_props, "radius" ), pos ) * *width;
		cx = geometry_to_float( mlt_properties_get( filter_props, "x" ), pos ) * *width;
		cy = geometry_to_float( mlt_properties_get( filter_props, "y" ), pos ) * *height;
		opac = geometry_to_float( mlt_properties_get( filter_props, "opacity" ), pos );
		int mode = mlt_properties_get_int( filter_props , "mode" );

		int video_width = *width;
		int video_height = *height;
		
		int x, y;
		int w2 = cx, h2 = cy;
		double delta = 1.0;
		double max_opac = opac;
		for ( y=0; y < video_height; y++ )
		{
			int h2_pow2 = pow( y - h2, 2.0 );
			for ( x = 0; x < video_width; x++ )
			{
				uint8_t *pix = ( *image + y * video_width * 2 + x * 2 );
				int dx = sqrt( h2_pow2 + pow( x - w2, 2.0 ));
				
				if (radius-smooth > dx) //center, make not darker
				{
					continue;
				}
				else if ( radius + smooth <= dx ) //max dark after smooth area
				{
					delta = 0.0;
				}
				else
				{
					// linear pos from of opacity (from 0 to 1)
					delta = (double) ( radius + smooth - dx ) / ( 2.0 * smooth );
					if ( mode == 1 )
					{
						//make cos for smoother non linear fade
						delta = (double) pow( cos((( 1.0 - delta ) * 3.14159 / 2.0 )), 3.0 );
					}
				}
				delta = MAX( max_opac, delta );
				*pix = (double) (*pix) * delta;
				*(pix+1) = ((double)(*(pix+1) - 127.0 ) * delta ) + 127.0;
			}
		}
	}

	return error;
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

mlt_filter filter_vignette_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );

	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set_double( MLT_FILTER_PROPERTIES( filter ), "smooth", 0.8 );
		mlt_properties_set_double( MLT_FILTER_PROPERTIES( filter ), "radius", 0.5 );
		mlt_properties_set_double( MLT_FILTER_PROPERTIES( filter ), "x", 0.5 );
		mlt_properties_set_double( MLT_FILTER_PROPERTIES( filter ), "y", 0.5 );
		mlt_properties_set_double( MLT_FILTER_PROPERTIES( filter ), "opacity", 0.0 );
		mlt_properties_set_double( MLT_FILTER_PROPERTIES( filter ), "mode", 0 );
	}
	return filter;
}

