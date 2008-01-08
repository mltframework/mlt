/*
 * filter_oldfilm.c -- oldfilm filter
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

#include "filter_oldfilm.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

	mlt_filter filter = mlt_frame_pop_service( this );
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );
	
	if (  error == 0 && *image && *format == mlt_image_yuv422 )
	{
		int h = *height;
		int w = *width;

		int x=0;
		int y=0;
		// Get u and v values
		int delta = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "delta" );
		int every = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "every" );
		int diffpic=rand()%delta*2-delta;
		if (rand()%100>every)
			diffpic=0;
		int yend,ydiff;
		if (diffpic<=0){
			y=h;
			yend=0;
			ydiff=-1;
		}else{
			y=0;
			yend=h;
			ydiff=1;
		}
		while(y!=yend){
			for (x=0;x<w;x++){
					uint8_t* pic=(*image+y*w*2+x*2);
					if (y+diffpic>0 && y+diffpic<h){
						*pic=*(pic+diffpic*w*2);
						*(pic+1)=*(pic+diffpic*w*2+1);
					}else{
						*pic=0;
						*(pic+1)=127;	
					}
			}
			y+=ydiff;
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

mlt_filter filter_oldfilm_init( char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "delta", "20" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "every", "80" );
	}
	return this;
}

