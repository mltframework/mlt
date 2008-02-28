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

#include <framework/mlt_filter.h>
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
		
		mlt_position in = mlt_filter_get_in( filter );
		mlt_position out = mlt_filter_get_out( filter );
		mlt_position time = mlt_frame_get_position( this );
		double position = ( double )( time - in ) / ( double )( out - in + 1 );
		srand(position*10000);
		
		int delta = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "delta" );
		int every = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "every" );
		
		int bdu = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "brightnessdelta_up" );
		int bdd = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "brightnessdelta_down" );
		int bevery = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "brightnessdelta_every" );
			
		int diffpic=0;
		if (delta)
			diffpic=rand()%delta*2-delta;
		
		int brightdelta=0;
		if ((bdu+bdd)!=0)
			 brightdelta=rand()%(bdu+bdd)-bdd;
		if (rand()%100>every)
			diffpic=0;
		if (rand()%100>bevery)
			brightdelta=0;
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
			//int newy=y+diffpic;
			for (x=0;x<w;x++){
					uint8_t* pic=(*image+y*w*2+x*2);
					int newy=y+diffpic;
					if (newy>0 && newy<h ){
						uint8_t oldval=*(pic+diffpic*w*2);
						/* frame around
						int randx=(x<=frameborder)?x:(x+frameborder>w)?w-x:-1;
						int randy=((newy)<=frameborder)?(newy):((newy)+frameborder>h)?h-(y+diffpic):-1;
						if (randx>=0 ){
							oldval=oldval*pow(((double)randx/(double)frameborder),1.5);
						}
						if (randy>=0 ){
							oldval=oldval*pow(((double)randy/(double)frameborder),1.5);
						}
						if (randx>=0 && randy>=0){
							//oldval=oldval*(randx*randy)/500.0;
						}
						*/
						if ( ((int) oldval + brightdelta ) >255)
							*pic=255;
						else if ( ( (int) oldval+brightdelta )  <0){
							*pic=0;
						}else
							*pic=oldval+brightdelta;
						*(pic+1)=*(pic+diffpic*w*2+1);

					}else{
						*pic=0;
						//*(pic-1)=127;	
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

mlt_filter filter_oldfilm_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "delta", "14" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "every", "20" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "brightnessdelta_up" , "20" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "brightnessdelta_down" , "30" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "brightnessdelta_every" , "70" );
	}
	return this;
}

