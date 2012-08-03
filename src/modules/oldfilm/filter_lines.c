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
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );

	if ( error == 0 && *image )
	{
		int h = *height;
		int w = *width;

		int line_width = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "line_width" );
		int num = mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "num" );
		double maxdarker= (double)mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "darker" ) ;
		double maxlighter=(double)mlt_properties_get_int( MLT_FILTER_PROPERTIES( filter ), "lighter" ) ;
		//int frame = mlt_properties_get_int( this, "_position" );
		char buf[256];
		char typebuf[256];
		
		if ( line_width < 1 )
			return 0;

		double position = mlt_filter_get_progress( filter, this );
		srand(position*10000);
		
		mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

		while (num--){
			int type=(rand()%3)+1;
			int x1=(double)w*rand()/RAND_MAX;
			int dx=rand()%line_width;
			int x=0,y=0;
			int ystart=rand()%h;
			int yend=rand()%h;
			
			sprintf(buf,"line%d",num);
			sprintf(typebuf,"typeline%d",num);
			maxlighter+=rand()%30-15;
			maxdarker+=rand()%30-15;
			
			if (mlt_properties_get_int(MLT_FILTER_PROPERTIES( filter ),buf)==0){
				mlt_properties_set_int(MLT_FILTER_PROPERTIES( filter ),buf,x1);
			}
			
			if (mlt_properties_get_int(MLT_FILTER_PROPERTIES( filter ),typebuf)==0 ){
				mlt_properties_set_int(MLT_FILTER_PROPERTIES( filter ),typebuf,type);
			}

			
			x1=mlt_properties_get_int(MLT_FILTER_PROPERTIES( filter ),buf);
			type=mlt_properties_get_int(MLT_FILTER_PROPERTIES( filter ),typebuf);
			if (position!=mlt_properties_get_double(MLT_FILTER_PROPERTIES( filter ),"last_oldfilm_line_pos")){
				x1+=(rand()%11-5);
			}
		
			if (yend<ystart){
				yend=h;
			}
			
			for (x = -dx ; x < dx && dx != 0 ; x++ )
				for(y=ystart;y<yend;y++)
					if (x+x1<w && x+x1>0){
						uint8_t* pixel=(*image+(y)*w*2+(x+x1)*2);
						double diff=1.0-fabs(x)/dx;
						switch(type){
							case 1: //blackline
								*pixel-=((double)*pixel*diff*maxdarker/100.0);
								break;
							case 2: //whiteline
								*pixel+=((255.0-(double)*pixel)*diff*maxlighter/100.0);
								break;
							case 3: //greenline
								*(pixel+1)-=((*(pixel+1))*diff*maxlighter/100.0);
							break;
						}
							
				}
			mlt_properties_set_int(MLT_FILTER_PROPERTIES( filter ),buf,x1);
		}
		mlt_properties_set_double(MLT_FILTER_PROPERTIES( filter ),"last_oldfilm_line_pos",position);
		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
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
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "line_width", 2 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "num", 5 );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "darker" , 40 ) ;
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "lighter" , 40 ) ;
	}
	return this;
}


