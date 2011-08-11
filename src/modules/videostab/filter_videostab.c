/*
 * filter_imagestab.c -- video stabilization with code from http://vstab.sourceforge.net/
 * Copyright (c) 2011 Marco Gittler <g.marco@freenet.de>
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
#include <framework/mlt_producer.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)

#include "stab/vector.h"
#include "stab/utils.h"
#include "stab/estimate.h"
#include "stab/resample.h" 

int opt_shutter_angle = 0;

int nf, i, nc, nr;
int tfs, fps;

vc *pos_i=NULL, *pos_h=NULL, *pos_y=NULL;

es_ctx *es=NULL;
rs_ctx *rs=NULL;
int initialized=0;


int load_or_generate_pos_y(mlt_frame this,int  *h,int *w,int tfs, int fps2){

	int i=0;
	mlt_producer producer = mlt_frame_get_original_producer(this);
	mlt_properties prod_props= MLT_PRODUCER_PROPERTIES ( producer );
	//mlt_properties_debug(prod_prosp,"prod",NULL);
	mlt_image_format format = mlt_image_rgb24;
	for (i=0;i< mlt_properties_get_int( prod_props, "length" );i++){
		mlt_producer_seek(producer,i);
		mlt_frame frame;
		mlt_service_get_frame( mlt_producer_service(producer), &frame, 0 );
		int size = mlt_image_format_size( mlt_image_rgb24, w, h, NULL );
		uint8_t *buffer= NULL;
		int error = mlt_frame_get_image( frame, &buffer, &format, w, h, 1 );

		pos_i[i] = vc_add( i > 0 ? pos_i[i - 1] : vc_set(0.0, 0.0), es_estimate(es, buffer));
	} 
	hipass(pos_i, pos_h, tfs, fps2);

}

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

	mlt_filter filter = mlt_frame_pop_service( this );
	*format = mlt_image_rgb24;
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );
	int in,out,length;
	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
	//mlt_producer producer = mlt_properties_get_data( MLT_FILTER_PROPERTIES( filter ), "producer", NULL );
	 mlt_producer producer = mlt_frame_get_original_producer(this);
	
	mlt_properties prod_props= MLT_PRODUCER_PROPERTIES ( producer );
	 mlt_position pos = mlt_filter_get_position( filter, this );


	if ( error == 0 && *image )
	{
		int h = *height;
		int w = *width;
		
		double position = mlt_filter_get_progress( filter, this );
		
	mlt_properties pro=prod_props;
	in=mlt_properties_get_int( pro, "in" );
	out=mlt_properties_get_int( pro, "out" );
	length=mlt_properties_get_int( pro, "length" );
		if (!initialized){
			int opt_shutter_angle=100;
			int i1=0;
			tfs = length;// mv_in.header->TotalFrames;
			fps =  mlt_profile_fps( profile ); //1000000 / 20000;//mv_in.header->MicroSecPerFrame;

			pos_i = (vc *)malloc(tfs * sizeof(vc));
			pos_h = (vc *)malloc(tfs * sizeof(vc));

			pos_y = (vc *)malloc(h * sizeof(vc));
			es = es_init(w, h);
			rs = rs_init(w, h);
			initialized=1;

			if ( mlt_properties_get_int (MLT_FILTER_PROPERTIES( filter ), "calculated" )!=1 )
				load_or_generate_pos_y(this,&h,&w,tfs,fps/2);
			 mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "calculated", "1" );

		}
		if (initialized>=1){
			for (i = 0; i < h; i ++) {
				pos_y[i] = interp( pos_h, tfs, pos + (i - h / 2.0) * opt_shutter_angle / (h * 360.0));
			}
			rs_resample(rs,*image,pos_y);
			initialized=2;
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

void filter_close( mlt_filter this )
{
	if (pos_i) {free(pos_i); pos_i=NULL;}
	if (pos_h) {free(pos_h); pos_h=NULL;}
	if (pos_y) {free(pos_y); pos_y=NULL;}
	if (es) {es_free(es); es=NULL;}
	if (rs) {rs_free(rs); rs=NULL;}
	initialized=0;
	free_lanc_kernels();
}

mlt_filter filter_videostab_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = mlt_filter_new( );
	this->close = filter_close;
	prepare_lanc_kernels();
	if ( this != NULL )
	{
		this->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "shutterangle", "0" );
	}
	return this;
}

