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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a<b?b:a)

#define SIGMOD_STEPS 1000
#define POSITION_VALUE(p,s,e) (s+((double)(e-s)*p ))
static double pow2[SIGMOD_STEPS];

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	
	mlt_filter filter = mlt_frame_pop_service( this );
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );
	
	if ( error == 0 && *image && *format == mlt_image_yuv422 )
	{
		int smooth_s=80,radius_s=50,x_s=50,y_s=50,opac_s=0;
		int smooth_e=80,radius_e=50,x_e=50,y_e=50,opac_e=0;
		
		sscanf(mlt_properties_get(MLT_FILTER_PROPERTIES( filter ), "start"  ), "%d:%d,%dx%d,%d",&smooth_s,&radius_s,&x_s,&y_s,&opac_s);
		if (mlt_properties_get(MLT_FILTER_PROPERTIES( filter ), "end"  ) ){
			sscanf(mlt_properties_get(MLT_FILTER_PROPERTIES( filter ), "end"  ), "%d:%d,%dx%d,%d",&smooth_e,&radius_e,&x_e,&y_e,&opac_e);
		}else{
			smooth_e=smooth_s;
			radius_e=radius_s;
			x_e=x_s;
			y_e=y_s;
			opac_e=opac_s;
		}
		mlt_position in = mlt_filter_get_in( filter );
		mlt_position out = mlt_filter_get_out( filter );
		mlt_position time = mlt_frame_get_position( this );
		double position = ( double )( time -in ) / ( double )( out - in + 1 ) /100.0;
		
		double smooth = POSITION_VALUE (position, smooth_s, smooth_e) ;
		double radius = POSITION_VALUE (position, radius_s, radius_e)/100.0* *width;
		
		double cx = POSITION_VALUE (position, x_s, x_e ) * *width/100;
		double cy = POSITION_VALUE (position, y_s, y_e ) * *height/100; 
		double opac= POSITION_VALUE(position, opac_s, opac_e) ;
		
		int video_width = *width;
		int video_height = *height;
		
		int x,y;
		int w2=cx,h2=cy;
		double delta=0.0;
		double max_opac=255.0;
		
		if (opac>0.0)
			max_opac=100.0/opac;
		
		for (y=0;y<video_height;y++){
			int h2_pow2=pow(y-h2,2.0);
			for (x=0;x<video_width;x++){
				uint8_t *pix=(*image+y*video_width*2+x*2);
				int dx=sqrt(h2_pow2+pow(x-w2,2.0));
				double sigx=(double)(-dx+radius-smooth)/smooth;
				
				if (smooth>0.001 && sigx>-10.0 && sigx<10.0){
					delta=pow2[((int)((sigx+10)*SIGMOD_STEPS/20))];
				}else if (smooth>0.001 && sigx>10.0){
					continue;
				}else{
					delta=255.0;
				}
				if ( max_opac<delta){
						delta=max_opac;
				}
				if (delta!=0.0){
					*pix=(double)(*pix)/delta;
					*(pix+1)=((double)(*(pix+1)-127.0)/delta)+127.0;
				}else{
					*pix=*(pix+1)=0;
				}
				
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
	int i=0;
	if ( this != NULL )
	{
		for (i=-SIGMOD_STEPS/2;i<SIGMOD_STEPS/2;i++){
			pow2[i+SIGMOD_STEPS/2]=1.0+pow(2.0,-((double)i)/((double)SIGMOD_STEPS/20.0));
		}
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "start", "80:50:50x50:0" );
		//mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "end", "" );

	}
	return this;
}


