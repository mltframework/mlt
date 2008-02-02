/*
 * filter_dust.c -- dust filter
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

		int maxdia = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "maxdiameter" );
		int maxcount = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "maxcount" );
		int im=rand()%maxcount;
		
		while (im--){
			int type=im%2;
			int y1=rand()%h;
			int x1=rand()%w;
			int dx=rand()%maxdia;
			int dy=rand()%maxdia;
			int x=0,y=0;//,v=0;
			for (x=-dx;x<dx;x++)
				for (y=-dy;y<dy;y++){
					if (x1+x<w && x1+x>0 && y1+y<h && y1+y>0 && x!=0 && y!=0){
						//uint8_t *pix=*image+(y+y1)*w*2+(x+x1)*2;
						//v=255*(abs(x)+abs(y))/dx;
						switch(type){
							case 0:
								//v=((51*sqrt(y*y+x*x))/255);
								*(*image+(y+y1)*w*2+(x+x1)*2)=*(*image+(y+y1)*w*2+(x+x1)*2)/((51*sqrt(y*y+x*x))/255);
								/*if (v!=0)
									*pix/=v;
								else
									*pix=0;*/
								break;
							case 1:
								//v=((51*sqrt(y*y+x*x))/255);
								//v=255*(x+y)/dx;
								*(*image+(y+y1)*w*2+(x+x1)*2)=*(*image+(y+y1)*w*2+(x+x1)*2)*((51*sqrt(y*y+x*x))/255);
								/*if ((*pix*v)<255)
									*pix*=v;
								else
									*pix=255;*/
								break;
						}
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


mlt_filter filter_dust_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "maxdiameter", "10" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "maxcount", "10" );
	}
	return this;
}


