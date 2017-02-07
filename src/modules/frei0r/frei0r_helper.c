/*
 * frei0r_helper.c -- frei0r helper
 * Copyright (c) 2008 Marco Gittler <g.marco@freenet.de>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "frei0r_helper.h"
#include <frei0r.h>
#include <string.h>
#include <stdlib.h>

static void parse_color( int color, f0r_param_color_t *fcolor )
{
	fcolor->r = ( color >> 24 ) & 0xff;
	fcolor->r /= 255;
	fcolor->g = ( color >> 16 ) & 0xff;
	fcolor->g /= 255;
	fcolor->b = ( color >>  8 ) & 0xff;
	fcolor->b /= 255;
}

static void rgba_bgra( uint8_t *src, uint8_t* dst, int width, int height )
{
	int n = width * height + 1;

	while ( --n )
	{
		*dst++ = src[2];
		*dst++ = src[1];
		*dst++ = src[0];
		*dst++ = src[3];
		src += 4;
	}	
}

struct update_context {
	f0r_instance_t frei0r;
	int width;
	int height;
	double time;
	uint32_t* inputs[2];
	uint32_t* output;
	void (*f0r_update)  (f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe);
	void (*f0r_update2) (f0r_instance_t instance, double time, const uint32_t* inframe1,
						 const uint32_t* inframe2,const uint32_t* inframe3, uint32_t* outframe);
};

static int f0r_update_slice( int id, int index, int count, void *context )
{
	struct update_context *ctx = context;
	int slice_height = ctx->height / count;
	int slice_bytes = ctx->width * slice_height;
	uint32_t *input = ctx->inputs[0] + index * slice_bytes;
	uint32_t *output = ctx->output + index * slice_bytes;
	ctx->f0r_update(ctx->frei0r, ctx->time, input, output);
	return 0;
}

static int f0r_update2_slice( int id, int index, int count, void *context )
{
	struct update_context *ctx = context;
	int slice_height = ctx->height / count;
	int slice_bytes = ctx->width * slice_height;
	uint32_t *inputs[2] = {
		ctx->inputs[0] + index * slice_bytes,
		ctx->inputs[1] + index * slice_bytes
	};
	uint32_t *output = ctx->output + index * slice_bytes;
	ctx->f0r_update2(ctx->frei0r, ctx->time, inputs[0], inputs[1], NULL, output);
	return 0;
}

int process_frei0r_item( mlt_service service, double position, double time, mlt_properties prop, mlt_frame this, uint8_t **image, int *width, int *height )
{
	int i=0;
	f0r_instance_t ( *f0r_construct ) ( unsigned int , unsigned int ) = mlt_properties_get_data(  prop , "f0r_construct" ,NULL);
	if (!f0r_construct) {
		//printf("no ctor\n");
		return -1;
	}
	void (*f0r_update)(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)=mlt_properties_get_data(  prop , "f0r_update" ,NULL);
	void (*f0r_destruct)(f0r_instance_t instance)=mlt_properties_get_data(  prop , "f0r_destruct" ,NULL);
	void (*f0r_get_plugin_info)(f0r_plugin_info_t*)=mlt_properties_get_data( prop, "f0r_get_plugin_info" ,NULL);
	void (*f0r_get_param_info)(f0r_param_info_t* info, int param_index)=mlt_properties_get_data( prop ,  "f0r_get_param_info" ,NULL);
	void (*f0r_set_param_value)(f0r_instance_t instance, f0r_param_t param, int param_index)=mlt_properties_get_data(  prop , "f0r_set_param_value" ,NULL);
	void (*f0r_update2) (f0r_instance_t instance, double time,
			 const uint32_t* inframe1,const uint32_t* inframe2,const uint32_t* inframe3,
			 uint32_t* outframe) = mlt_properties_get_data(  prop , "f0r_update2" ,NULL);
	mlt_service_type type = mlt_service_identify( service );
	int not_thread_safe = mlt_properties_get_int( prop, "_not_thread_safe" );
	int slice_count = mlt_properties_get(prop, "threads") ? mlt_properties_get_int(prop, "threads") : -1;

	if (slice_count >= 0)
		slice_count = mlt_slices_count_normal();

	//use as name the width and height
	int slice_height = *height / (slice_count > 0? slice_count : 1);
	char ctorname[1024] = "";
	sprintf(ctorname, "ctor-%dx%d", *width, slice_height);

	mlt_service_lock( service );

	f0r_instance_t inst = mlt_properties_get_data(prop, ctorname, NULL);
	if (!inst) {
		inst = f0r_construct(*width, slice_height);
		mlt_properties_set_data(prop, ctorname, inst, 0, f0r_destruct , NULL);
	}

	if ( !not_thread_safe )
		mlt_service_unlock( service );

	f0r_plugin_info_t info;
	memset(&info, 0, sizeof(info));
	if (f0r_get_plugin_info) {
		f0r_get_plugin_info(&info);
		for (i=0;i<info.num_params;i++){
			f0r_param_info_t pinfo;
			f0r_get_param_info(&pinfo,i);
			char index[20];
			snprintf( index, sizeof(index), "%d", i );
			char *val = mlt_properties_get( prop , index );
			if ( !val )
				val = mlt_properties_get( prop , pinfo.name );
			if ( !val ) {
				// Use the backwards-compatibility param name map.
				mlt_properties map = mlt_properties_get_data( prop, "_param_name_map", NULL );
				if ( map ) {
					int j;
					for ( j = 0; !val && j < mlt_properties_count(map); j++ ) {
						if ( !strcmp(mlt_properties_get_value(map, j), index) )
							val = mlt_properties_get( prop , mlt_properties_get_name(map, j) );
					}
				}
			}
			if ( val ) {
				switch (pinfo.type) {
					case F0R_PARAM_DOUBLE:
					case F0R_PARAM_BOOL:
					{
						mlt_geometry geom=mlt_geometry_init();
						struct mlt_geometry_item_s item;
						mlt_geometry_parse(geom,val,-1,-1,-1);
						mlt_geometry_fetch(geom,&item,position);
						double t=item.x;
						f0r_set_param_value(inst,&t,i);
						mlt_geometry_close(geom);
						break;
					}
					case F0R_PARAM_COLOR:
					{
						f0r_param_color_t color;
						int int_color = mlt_properties_get(prop, index) ?
							mlt_properties_get_int(prop, index) : mlt_properties_get_int(prop, pinfo.name);
						parse_color(int_color, &color);
						f0r_set_param_value(inst, &color, i);
						break;
					}
					case F0R_PARAM_STRING:
					{
						f0r_set_param_value(inst, &val, i);
						break;
					}
				}
			}
		}
	}

	int video_area = *width * *height;
	uint32_t *result = mlt_pool_alloc( video_area * sizeof(uint32_t) );
	uint32_t *extra = NULL;
	uint32_t *source[2] = { (uint32_t*) image[0], (uint32_t*) image[1] };
	uint32_t *dest = result;

	if (info.color_model == F0R_COLOR_MODEL_BGRA8888) {
		rgba_bgra(image[0], (uint8_t*) result, *width, *height);
		source[0] = result;
		dest = (uint32_t*) image[0];
		if (type == producer_type) {
			extra = mlt_pool_alloc( video_area * sizeof(uint32_t) );
			dest = extra;
		}
		else if (type == transition_type && f0r_update2) {
			extra = mlt_pool_alloc( video_area * sizeof(uint32_t) );
			rgba_bgra(image[1], (uint8_t*) extra, *width, *height);
			source[1] = extra;
		}
	}
	if (type==producer_type) {
		if (slice_count > 0) {
			struct update_context ctx = {
				.frei0r = inst,
				.width = *width,
				.height = *height,
				.time = time,
				.inputs = { NULL, NULL },
				.output = dest,
				.f0r_update = f0r_update
			};
			mlt_slices_run_normal(slice_count, f0r_update_slice, &ctx);
		} else {
			f0r_update ( inst, time, source[0], dest );
		}
	} else if (type==filter_type) {
		if (slice_count > 0) {
			struct update_context ctx = {
				.frei0r = inst,
				.width = *width,
				.height = *height,
				.time = time,
				.inputs = { source[0], NULL },
				.output = dest,
				.f0r_update = f0r_update
			};
			mlt_slices_run_normal(slice_count, f0r_update_slice, &ctx);
		} else {
			f0r_update ( inst, time, source[0], dest );
		}
	} else if (type==transition_type && f0r_update2 ) {
		if (slice_count > 0) {
			struct update_context ctx = {
				.frei0r = inst,
				.width = *width,
				.height = *height,
				.time = time,
				.inputs = { source[0], source[1] },
				.output = dest,
				.f0r_update2 = f0r_update2
			};
			mlt_slices_run_normal(slice_count, f0r_update2_slice, &ctx);
		} else {
			f0r_update2 ( inst, time, source[0], source[1], NULL, dest );
		}
	}
	if ( not_thread_safe )
		mlt_service_unlock( service );
	if (info.color_model == F0R_COLOR_MODEL_BGRA8888) {
		rgba_bgra((uint8_t*) dest, (uint8_t*) result, *width, *height);
	}
	*image = (uint8_t*) result;
	mlt_frame_set_image(this, (uint8_t*) result, video_area * sizeof(uint32_t), mlt_pool_release);
	if (extra)
		mlt_pool_release(extra);

	return 0;
}

void destruct (mlt_properties prop ) {

	void (*f0r_destruct)(f0r_instance_t instance)=mlt_properties_get_data(  prop , "f0r_destruct" , NULL );
	void (*f0r_deinit)(void)=mlt_properties_get_data ( prop , "f0r_deinit" , NULL);
	int i=0;

	if ( f0r_deinit != NULL )
		f0r_deinit();

	for ( i=0 ; i < mlt_properties_count ( prop ) ; i++ ){
		if ( strstr ( mlt_properties_get_name ( prop , i ) , "ctor-" ) != NULL ){
			void * inst=mlt_properties_get_data( prop , mlt_properties_get_name ( prop , i ) , NULL );
			if ( inst != NULL ){
				f0r_destruct( (f0r_instance_t) inst);
			}
		}
	}
	void (*dlclose)(void*)=mlt_properties_get_data ( prop , "_dlclose" , NULL);
	void *handle=mlt_properties_get_data ( prop , "_dlclose_handle" , NULL);

	if (handle && dlclose ){
		dlclose ( handle );
	}

}
