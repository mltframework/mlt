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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_geometry.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a<b?b:a)

#define SIGMOD_STEPS 1000
#define POSITION_VALUE(p,s,e) (s+((double)(e-s)*p ))
//static double pow2[SIGMOD_STEPS];
static float geometry_to_float(char *val, mlt_position pos )
{
    float ret=0.0;
    struct mlt_geometry_item_s item;

	mlt_geometry geom=mlt_geometry_init();
    mlt_geometry_parse(geom,val,-1,-1,-1);
    mlt_geometry_fetch(geom,&item , pos );
    ret=item.x;
    mlt_geometry_close(geom);

    return ret;
}

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	
	mlt_filter filter = mlt_frame_pop_service( this );
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );

	if ( error == 0 && *image )
	{
		mlt_position in = mlt_filter_get_in( filter );
		//mlt_position out = mlt_filter_get_out( filter );
		mlt_position time = mlt_frame_get_position( this );
		
		float smooth, radius, cx, cy, opac;
        mlt_position pos = time - in;
        mlt_properties filter_props = MLT_FILTER_PROPERTIES( filter ) ;
		smooth = geometry_to_float ( mlt_properties_get( filter_props , "smooth" ) , pos );
        radius = geometry_to_float ( mlt_properties_get( filter_props , "radius" ) , pos );
		cx = geometry_to_float ( mlt_properties_get( filter_props , "x" ) , pos );
		cy = geometry_to_float ( mlt_properties_get( filter_props , "y" ) , pos );
		opac = geometry_to_float ( mlt_properties_get( filter_props , "opacity" ) , pos );
		
		int video_width = *width;
		int video_height = *height;
		
		int x,y;
		int w2=cx,h2=cy;
		double delta=1.0;
		double max_opac=opac/100.0;

		for (y=0;y<video_height;y++){
			int h2_pow2=pow(y-h2,2.0);
			for (x=0;x<video_width;x++){
				uint8_t *pix=(*image+y*video_width*2+x*2);
				int dx=sqrt(h2_pow2+pow(x-w2,2.0));
				
				if (radius-smooth>dx){  //center, make not darker
					continue;
				}
				else if (radius+smooth<=dx){//max dark after smooth area
					delta=0.0;
				}else{
					//double sigx=5.0-10.0*(double)(dx-radius+smooth)/(2.0*smooth);//smooth >10 inner area, <-10 in dark area
					//delta=pow2[((int)((sigx+10.0)*SIGMOD_STEPS/20.0))];//sigmoidal
					delta = ((double)(radius+smooth-dx)/(2.0*smooth));//linear
				}
				delta=MAX(max_opac,delta);
				*pix=(double)(*pix)*delta;
				*(pix+1)=((double)(*(pix+1)-127.0)*delta)+127.0;
			}
		}
		// short a, short b, short c, short d
		// a= gray val pix 1
		// b: +=blue, -=yellow
		// c: =gray pix 2
		// d: +=red,-=green
	}
	
	return error;
}

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	
	mlt_frame_push_service( frame, this );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}


mlt_filter filter_vignette_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	//int i=0;
	if ( this != NULL )
	{
		/*
		for (i=-SIGMOD_STEPS/2;i<SIGMOD_STEPS/2;i++){
			pow2[i+SIGMOD_STEPS/2]=1.0/(1.0+pow(2.0,-((double)i)/((double)SIGMOD_STEPS/20.0)));
		}
		*/
		
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "smooth", "80" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "radius", "50%" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "x", "50%" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "y", "50%" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "opacity", "0" );

		//mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "end", "" );

	}
	return this;
}


