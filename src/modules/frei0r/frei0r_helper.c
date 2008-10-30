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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "frei0r_helper.h"
#include <frei0r.h>
#include <string.h>

 int process_frei0r_item( mlt_service_type type,  double position , mlt_properties prop , mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable ){

	int i=0;
	f0r_instance_t ( *f0r_construct ) ( unsigned int , unsigned int ) =  mlt_properties_get_data(  prop , "f0r_construct" ,NULL);
	void (*f0r_update)(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)=mlt_properties_get_data(  prop , "f0r_update" ,NULL);
	void (*f0r_destruct)(f0r_instance_t instance)=mlt_properties_get_data(  prop , "f0r_destruct" ,NULL);
		
	void (*f0r_get_plugin_info)(f0r_plugin_info_t*)=mlt_properties_get_data( prop, "f0r_get_plugin_info" ,NULL);
	void (*f0r_get_param_info)(f0r_param_info_t* info, int param_index)=mlt_properties_get_data( prop ,  "f0r_get_param_info" ,NULL);
	void (*f0r_set_param_value)(f0r_instance_t instance, f0r_param_t param, int param_index)=mlt_properties_get_data(  prop , "f0r_set_param_value" ,NULL);
	void (*f0r_get_param_value)(f0r_instance_t instance, f0r_param_t param, int param_index)=mlt_properties_get_data(  prop , "f0r_get_param_value" ,NULL);
	void (*f0r_update2) (f0r_instance_t instance, double time,
			 const uint32_t* inframe1,const uint32_t* inframe2,const uint32_t* inframe3,
	 uint32_t* outframe)=mlt_properties_get_data(  prop , "f0r_update2" ,NULL);	
	
	
	//use as name the width and height
	f0r_instance_t inst;
	char ctorname[1024]="";
	sprintf(ctorname,"ctor-%dx%d",*width,*height);
	
	void* neu=mlt_properties_get_data( prop , ctorname ,NULL );
	if (!f0r_construct){
		//printf("no ctor\n");
		return -1;
	}
	if ( neu == 0 ){
		inst= f0r_construct(*width,*height);
		mlt_properties_set_data(  prop  ,  ctorname , inst, sizeof(void*) , f0r_destruct , NULL );;
	}else{
		inst=mlt_properties_get_data( prop ,  ctorname , NULL );
	}
	if (f0r_get_plugin_info){
		f0r_plugin_info_t info;
		f0r_get_plugin_info(&info);
		for (i=0;i<info.num_params;i++){
			f0r_param_info_t pinfo;
			f0r_get_param_info(&pinfo,i);
			mlt_geometry geom=mlt_geometry_init();
			struct mlt_geometry_item_s item;
			//set param if found
			
			double t=0.0;
			f0r_get_param_value(inst,&t,i);
			char *val;
			if (mlt_properties_get( prop , pinfo.name ) !=NULL ){
				switch (pinfo.type) {
					case F0R_PARAM_DOUBLE:
					case F0R_PARAM_BOOL:
						val=mlt_properties_get(prop, pinfo.name );
						mlt_geometry_parse(geom,val,-1,-1,-1);
						mlt_geometry_fetch(geom,&item,position);
						t=item.x;
						f0r_set_param_value(inst,&t,i);
						break;
					//case F0R_PARAM_COLOR:
					//	t=mlt_properties_get_double( prop , pinfo.name );	
					
				}
			}
			
			mlt_geometry_close(geom);
		}
	}
	
	int video_area = *width * *height;
	uint32_t *img_a = mlt_pool_alloc( video_area * sizeof(uint32_t) );
	uint32_t *img_b = mlt_pool_alloc( video_area * sizeof(uint32_t) );
	
	if (type==filter_type){
		mlt_convert_yuv422_to_rgb24a(*image, (uint8_t *)img_a, video_area);
		f0r_update ( inst , position , img_a , img_b );
		mlt_convert_rgb24a_to_yuv422((uint8_t *)img_b , *width, *height, *width * sizeof(uint32_t), 
												*image, NULL );			
	}else if (type==transition_type && f0r_update2 ){
		uint32_t *result = mlt_pool_alloc( video_area * sizeof(uint32_t) );
		
		mlt_convert_yuv422_to_rgb24a ( image[0] , (uint8_t *)img_a , video_area );
		mlt_convert_yuv422_to_rgb24a ( image[1] , (uint8_t *)img_b , video_area );
		f0r_update2 ( inst , position , img_a , img_b , NULL , result );
		
		uint8_t * image_ptr=mlt_properties_get_data(MLT_FRAME_PROPERTIES(this), "image", NULL );
		if (image_ptr)
    		    mlt_convert_rgb24a_to_yuv422((uint8_t *)result, *width, *height, *width * sizeof(uint32_t), image_ptr , NULL );
		
		mlt_pool_release(result);
 	}
	mlt_pool_release(img_a);
	mlt_pool_release(img_b);
	
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
