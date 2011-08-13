/*
 * filter_imagestab.c -- video stabilization with code from http://vstab.sourceforge.net/
 * Copyright (c) 2011 Marco Gittler <g.marco@freenet.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_producer.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>

#include "stab/vector.h"
#include "stab/utils.h"
#include "stab/estimate.h"
#include "stab/resample.h" 


typedef struct {
	mlt_filter parent;
	int initialized;
	vc *pos_h;
	vc *pos_y;
	rs_ctx *rs;
} *videostab;

int load_vc_from_file(videostab self, const char* filename ,vc* pos,int maxlength){

	struct stat stat_buff;
	stat (filename,&stat_buff);
	if (S_ISREG(stat_buff.st_mode) ){ 
		//load file
		mlt_log_verbose(NULL,"loading file %s\n",filename);
		FILE *infile;
		infile=fopen(filename,"r");
		if (infile){
					int i;
			for (i=0;i< maxlength;i++){
				float x,y;
				fscanf(infile,"%f%f",&x,&y);
				self->pos_h[i].x=x;
				self->pos_h[i].y=y;
				//pos_h[i]=vc_set(x,y);
			}
			fclose(infile);
			return 1;
		}
	}
	return 0;
}

int save_vc_to_file(videostab self, const char* filename ,vc* pos,int length){

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

int load_or_generate_pos_h(videostab self, mlt_frame frame,int  *h,int *w,int tfs, int fps){
	int i=0;
	char shakefile[2048];
	mlt_producer producer = mlt_frame_get_original_producer(frame) ;
	mlt_properties prod_props= MLT_PRODUCER_PROPERTIES ( producer );

	mlt_producer parent_prod;
	if (mlt_properties_get_int(prod_props,"_cut") == 1 ){
		parent_prod=mlt_producer_cut_parent( producer);
	}else{
		// had no such case, but the is a fallback
		parent_prod = producer;
	}
	sprintf(shakefile,"%s%s", mlt_properties_get (MLT_PRODUCER_PROPERTIES(parent_prod), "resource"),".deshake");
	if (!load_vc_from_file( self, shakefile, self->pos_h, tfs)){

		mlt_log_verbose(frame,"calculating deshake, please wait\n");
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
				hipass(pos_i, self->pos_h, tfs, fps);
		free (pos_i);
		free(es1);
		save_vc_to_file( self, shakefile, self->pos_h,tfs);
	}
	return 0;
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{

	mlt_filter filter = mlt_frame_pop_service( frame );
	videostab self = filter->child;
	*format = mlt_image_rgb24;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );
	int in,out,length;
	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
	mlt_producer producer = mlt_frame_get_original_producer(frame);

	mlt_properties prod_props= MLT_PRODUCER_PROPERTIES ( producer );
	mlt_position pos = mlt_filter_get_position( filter, frame );
	int opt_shutter_angle=mlt_properties_get_int ( MLT_FRAME_PROPERTIES (frame) , "shutterangle") ;


	if ( error == 0 && *image )
	{
		int h = *height;
		int w = *width;

		//double position = mlt_filter_get_progress( filter, frame );

		mlt_properties pro=prod_props;
		in=mlt_properties_get_int( pro, "in" );
		out=mlt_properties_get_int( pro, "out" );
		length=mlt_properties_get_int( pro, "length" );
		mlt_log_verbose(filter,"deshaking for in=%d out=%d length=%d\n",in,out,length);
		if (!self->initialized){
						int fps =  mlt_profile_fps( profile );

						self->pos_h = (vc *)malloc(length * sizeof(vc));

			self->pos_y = (vc *)malloc(h * sizeof(vc));
			self->rs = rs_init(w, h);
			self->initialized=1;

						load_or_generate_pos_h(self, frame,&h,&w,length,fps/2);

		}
		if (self->initialized>=1){
					int i;
			for (i = 0; i < h; i ++) {
				self->pos_y[i] = interp( self->pos_h, length, pos + (i - h / 2.0) * opt_shutter_angle / (h * 360.0));
			}
			rs_resample( self->rs, *image, self->pos_y );
			self->initialized=2;
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

void filter_close( mlt_filter parent )
{
	mlt_service service = MLT_FILTER_SERVICE( parent );
	videostab self = parent->child;
	if ( self->pos_h ) free(self->pos_h);
	if ( self->pos_y ) free(self->pos_y);
	if ( self->rs ) rs_free(self->rs);
	free_lanc_kernels();
	service->close = NULL;
	mlt_service_close( service );
	free( self );
}

mlt_filter filter_videostab_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	videostab self = calloc( 1, sizeof(*self) );
	if ( self )
	{
		mlt_filter parent = mlt_filter_new();
		parent->child = self;
		parent->close = filter_close;
		parent->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES(parent), "shutterangle", "0" ); // 0 - 180 , default 0
		prepare_lanc_kernels();
		return parent;
	}
	return NULL;
}
