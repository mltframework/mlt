/*
 * filter_lines.c -- lines filter
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


static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

	mlt_filter filter = mlt_frame_pop_service( this );
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );

	if ( error == 0 && *image && *format == mlt_image_yuv422 )
	{
		int h = *height;
		int w = *width;

		int width = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "width" );
		int num = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "num" );
		//int frame = mlt_properties_get_int( this, "_position" );
		char buf[1024];
		
		while (num--){
			sprintf(buf,"line%d",num);
			
			int type=rand()%2;
			int x1=rand()%w;;
			int dx=rand()%width;
			/*int xx=mlt_properties_get_int(MLT_PRODUCER_PROPERTIES(mlt_frame_get_original_producer( this ),buf);
			if (xx==0){
				mlt_properties_set_int(this,buf,x1);
				//x1=100;
			}
			x1=mlt_properties_get_int(this,buf)+5;
			*/
			int x=0,y=0,pix=0;
			for (x=-dx;x<dx;x++)
				for(y=0;y<h;y++)
					if (x+x1<w && x+x1>0){
						uint8_t* pixel=(*image+(y)*w*2+(x+x1)*2);
						switch(type){
							case 0:
								pix=(*pixel)*abs(x)/dx;
								*pixel=pix;
								break;
							case 1:
								pix=(*pixel)+((255-*pixel)*abs(x)/dx);
								*pixel=pix;
								break;
						}
							
				}
		}
	}

	return error;
}

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{

	mlt_frame_push_service( frame, this );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

mlt_filter filter_lines_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "width", "2" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "num", "5" );
	}
	return this;
}


