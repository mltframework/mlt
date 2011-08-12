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
#include <framework/mlt_log.h>
#include <framework/mlt_producer.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)

#include "stab/vector.h"
#include "stab/utils.h"
#include "stab/estimate.h"
#include "stab/resample.h" 

int opt_shutter_angle = 0;

int nf, i, nc, nr;
int tfs, fps;

vc *pos_h=NULL, *pos_y=NULL;

rs_ctx *rs=NULL;
int initialized=0;

int load_vc_from_file(const char* filename ,vc* pos,int maxlength){

	struct stat stat_buff;
	stat (filename,&stat_buff);
	if (S_ISREG(stat_buff.st_mode) ){ 
		//load file
		mlt_log_verbose(NULL,"loading file %s\n",filename);
		FILE *infile;
		infile=fopen(filename,"r");
		if (infile){
			for (i=0;i< maxlength;i++){
				float x,y;
				fscanf(infile,"%f%f",&x,&y);
				pos_h[i].x=x;
				pos_h[i].y=y;
				//pos_h[i]=vc_set(x,y);
			}
			fclose(infile);
			return 1;
		}
	}
	return 0;
}

int save_vc_to_file(const char* filename ,vc* pos,int length){

	FILE *outfile;
	int i=0;
	outfile=fopen(filename,"w+");
	if (!outfile){
		mlt_log_error(NULL,"could not save shakefile %s\n",filename);
		return -1;
	}
	for (i=0;i< length ;i++){
		fprintf(outfile,"%f %f\n",pos[i].x,pos[i].y);
		//mlt_log_verbose(NULL,"writing %d/%d %f %f\n",i,length,pos[i].x,pos[i].y);
	}

	fclose(outfile);
	return 0;
}

int load_or_generate_pos_h(mlt_frame this,int  *h,int *w,int tfs, int fps2){
	int i=0;
	char shakefile[2048];
	mlt_producer producer = mlt_frame_get_original_producer(this) ;
	mlt_properties prod_props= MLT_PRODUCER_PROPERTIES ( producer );

	mlt_producer parent_prod;
	if (mlt_properties_get_int(prod_props,"_cut") == 1 ){
		parent_prod=mlt_producer_cut_parent( producer);
	}else{
		// had no such case, but the is a fallback
		parent_prod = producer;
	}
	sprintf(shakefile,"%s%s", mlt_properties_get (MLT_PRODUCER_PROPERTIES(parent_prod), "resource"),".deshake");
	if (!load_vc_from_file(shakefile,pos_h,tfs)){

		mlt_log_verbose(this,"calculating deshake, please wait\n");
		mlt_image_format format = mlt_image_rgb24;
		vc* pos_i = (vc *)malloc(tfs * sizeof(vc));
		es_ctx *es1=es_init(*w,*h);
		for (i=0;i< tfs;i++){
			mlt_producer_seek(producer,i);
			mlt_frame frame;
			mlt_service_get_frame( mlt_producer_service(producer), &frame, 0 );
			uint8_t *buffer= NULL;
			int error = mlt_frame_get_image( frame, &buffer, &format, w, h, 1 );

			pos_i[i] = vc_add( i > 0 ? pos_i[i - 1] : vc_set(0.0, 0.0), es_estimate(es1, buffer));
		} 
		hipass(pos_i, pos_h, tfs, fps2);
		free (pos_i);
		free(es1);
		save_vc_to_file(shakefile,pos_h,tfs);
	}
	return 0;
}

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

	mlt_filter filter = mlt_frame_pop_service( this );
	*format = mlt_image_rgb24;
	int error = mlt_frame_get_image( this, image, format, width, height, 1 );
	int in,out,length;
	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
	mlt_producer producer = mlt_frame_get_original_producer(this);

	mlt_properties prod_props= MLT_PRODUCER_PROPERTIES ( producer );
	mlt_position pos = mlt_filter_get_position( filter, this );
	int opt_shutter_angle=mlt_properties_get_int ( MLT_FRAME_PROPERTIES (this) , "shutterangle") ;


	if ( error == 0 && *image )
	{
		int h = *height;
		int w = *width;

		//double position = mlt_filter_get_progress( filter, this );

		mlt_properties pro=prod_props;
		in=mlt_properties_get_int( pro, "in" );
		out=mlt_properties_get_int( pro, "out" );
		length=mlt_properties_get_int( pro, "length" );
		mlt_log_verbose(filter,"deshaking for in=%d out=%d length=%d\n",in,out,length);
		if (!initialized){
			tfs = length;
			fps =  mlt_profile_fps( profile );

			pos_h = (vc *)malloc(tfs * sizeof(vc));

			pos_y = (vc *)malloc(h * sizeof(vc));
			rs = rs_init(w, h);
			initialized=1;

			load_or_generate_pos_h(this,&h,&w,tfs,fps/2);

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
	if (pos_h) {free(pos_h); pos_h=NULL;}
	if (pos_y) {free(pos_y); pos_y=NULL;}
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
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "shutterangle", "0" ); // 0 - 180 , default 0
	}
	return this;
}

