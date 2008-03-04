/*
 * factory.c -- the factory method interfaces
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

#include <string.h>
#include <framework/mlt.h>
#include <frei0r.h>

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdlib.h>

extern mlt_filter filter_frei0r_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_frame filter_process( mlt_filter this, mlt_frame frame );
extern void filter_close( mlt_filter this );
extern void transition_close( mlt_transition this );
extern mlt_frame transition_process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame );

static mlt_properties subtag ( mlt_properties prop , char *name ,  mlt_serialiser serialise_yaml ){
	mlt_properties ret = mlt_properties_get_data( prop , name , NULL );
	
	if (!ret){
		ret = mlt_properties_new ( );
		mlt_properties_set_data ( prop , name , ret , 0 , ( mlt_destructor )mlt_properties_close , serialise_yaml );
	}
	return ret;
}

void fill_param_info ( mlt_repository repository , void* handle, f0r_plugin_info_t* info , mlt_service_type type , char* name ) {
	
	int j=0;
	void (*param_info)(f0r_param_info_t*,int param_index)=dlsym(handle,"f0r_get_param_info");
	mlt_properties metadata_properties=NULL;
	switch (type){
		case filter_type:
			metadata_properties=mlt_repository_filters(repository);
			break;
		case transition_type:
			metadata_properties=mlt_repository_transitions(repository);
			break;
		default:
			metadata_properties=NULL;
	}
	
	if (!metadata_properties){
		return;
	}
	
	mlt_properties this_item_properties = mlt_properties_get_data( metadata_properties , name , NULL );
	mlt_properties metadata = subtag( this_item_properties , "metadata" , ( mlt_serialiser )mlt_properties_serialise_yaml );
	
	char descstr[2048];
	snprintf ( descstr, 2048 , "%s (Version: %d.%d)" , info->explanation , info->major_version , info->minor_version );
	mlt_properties_set ( metadata, "title" , info->name );
	mlt_properties_set ( metadata, "identifier" , name );
	mlt_properties_set ( metadata, "description" , descstr );
	mlt_properties_set ( metadata, "creator" , info->author );
	
	mlt_properties parameter = subtag ( metadata , "parameters" , NULL );
	mlt_properties tags = subtag ( metadata , "tags" , NULL );
	mlt_properties_set ( tags , "0" , "Video" );
	
	char numstr[512];
	
	for (j=0;j<info->num_params;j++){
		snprintf ( numstr , 512 , "%d" , j );
		mlt_properties pnum = subtag ( parameter , numstr , NULL );
		
		f0r_param_info_t paraminfo;
		param_info(&paraminfo,j);
		mlt_properties_set ( pnum , "identifier" , paraminfo.name );
		mlt_properties_set ( pnum , "title" , paraminfo.name );
		mlt_properties_set ( pnum , "description" , paraminfo.explanation);
		
		if ( paraminfo.type == F0R_PARAM_DOUBLE ){
			mlt_properties_set ( pnum , "type" , "integer" );
			mlt_properties_set ( pnum , "minimum" , "0" );
			mlt_properties_set ( pnum , "maximum" , "1000" );
			mlt_properties_set ( pnum , "readonly" , "no" );
			mlt_properties_set ( pnum , "widget" , "spinner" );
		}else
		if ( paraminfo.type == F0R_PARAM_BOOL ){
			mlt_properties_set ( pnum , "type" , "boolean" );
			mlt_properties_set ( pnum , "minimum" , "0" );
			mlt_properties_set ( pnum , "maximum" , "1" );
			mlt_properties_set ( pnum , "readonly" , "no" );
		}else
		if ( paraminfo.type == F0R_PARAM_STRING ){
			mlt_properties_set ( pnum , "type" , "string" );
			mlt_properties_set ( pnum , "readonly" , "no" );
		}
	}
}

void * load_lib(  mlt_profile profile, mlt_service_type type , void* handle){
	
	int i=0;
	void (*f0r_get_plugin_info)(f0r_plugin_info_t*),
		*f0r_construct , *f0r_update , *f0r_destruct,
		(*f0r_get_param_info)(f0r_param_info_t* info, int param_index),
		(*f0r_init)(void) , *f0r_deinit ,
		(*f0r_set_param_value)(f0r_instance_t instance, f0r_param_t param, int param_index),
		(*f0r_get_param_value)(f0r_instance_t instance, f0r_param_t param, int param_index),
		(*f0r_update2) (f0r_instance_t instance, double time,	const uint32_t* inframe1,
		  const uint32_t* inframe2,const uint32_t* inframe3, uint32_t* outframe);
				
	if ( ( f0r_construct = dlsym(handle, "f0r_construct") ) && 
				(f0r_update = dlsym(handle,"f0r_update") ) && 
				(f0r_destruct = dlsym(handle,"f0r_destruct") ) && 
				(f0r_get_plugin_info = dlsym(handle,"f0r_get_plugin_info") ) && 
				(f0r_get_param_info = dlsym(handle,"f0r_get_param_info") ) && 
				(f0r_set_param_value= dlsym(handle,"f0r_set_param_value" ) ) &&
				(f0r_get_param_value= dlsym(handle,"f0r_get_param_value" ) ) &&
				(f0r_init= dlsym(handle,"f0r_init" ) ) &&
				(f0r_deinit= dlsym(handle,"f0r_deinit" ) ) 
		){
	
		f0r_update2=dlsym(handle,"f0r_update2");
		
		f0r_plugin_info_t info;
		f0r_get_plugin_info(&info);
		
		void* ret=NULL;	
		mlt_properties properties=NULL;
		
		if (type == filter_type && info.plugin_type == F0R_PLUGIN_TYPE_FILTER ){
			mlt_filter this = mlt_filter_new( );
			if ( this != NULL )
			{
				this->process = filter_process;
				this->close = filter_close;			
				f0r_init();
				properties=MLT_FILTER_PROPERTIES ( this );
							
				for (i=0;i<info.num_params;i++){
					f0r_param_info_t pinfo;
					f0r_get_param_info(&pinfo,i);
					
				}
				
				ret=this;
			}
		}else if (type == transition_type && info.plugin_type == F0R_PLUGIN_TYPE_MIXER2){
			mlt_transition transition = mlt_transition_new( );
			if ( transition != NULL )
			{
				transition->process = transition_process;
				transition->close = transition_close;
				properties=MLT_TRANSITION_PROPERTIES( transition );
				mlt_properties_set_int(properties, "_transition_type", 1 );

				ret=transition;
			}
		}
		mlt_properties_set_data(properties, "_dlclose_handle", handle , sizeof (void*) , NULL , NULL );
		mlt_properties_set_data(properties, "_dlclose", dlclose , sizeof (void*) , NULL , NULL );
		mlt_properties_set_data(properties, "f0r_construct", f0r_construct , sizeof(void*),NULL,NULL);
		mlt_properties_set_data(properties, "f0r_update", f0r_update , sizeof(void*),NULL,NULL);
		if (f0r_update2)
			mlt_properties_set_data(properties, "f0r_update2", f0r_update2 , sizeof(void*),NULL,NULL);
		mlt_properties_set_data(properties, "f0r_destruct", f0r_destruct , sizeof(void*),NULL,NULL);
		mlt_properties_set_data(properties, "f0r_get_plugin_info", f0r_get_plugin_info , sizeof(void*),NULL,NULL);
		mlt_properties_set_data(properties, "f0r_get_param_info", f0r_get_param_info , sizeof(void*),NULL,NULL);
		mlt_properties_set_data(properties, "f0r_set_param_value", f0r_set_param_value , sizeof(void*),NULL,NULL);
		mlt_properties_set_data(properties, "f0r_get_param_value", f0r_get_param_value , sizeof(void*),NULL,NULL);
		
		
		return ret;
	}else{
		printf("some was wrong\n");
		dlerror();
	}
	return NULL;
}

void * create_frei0r_item ( mlt_profile profile, mlt_service_type type, const char *id, void *arg){

	mlt_tokeniser tokeniser = mlt_tokeniser_init ( );
	int dircount=mlt_tokeniser_parse_new (
		tokeniser ,
		getenv("MLT_FREI0R_PLUGIN_PATH") ? getenv("MLT_FREI0R_PLUGIN_PATH") : "/usr/lib/frei0r-1" ,
		 ":"
	);
	void* ret=NULL;
	while (dircount--){
		char soname[1024]="";
		
		char *save_firstptr = NULL;
		char *firstname=strtok_r(strdup(id),".",&save_firstptr);
		
		firstname=strtok_r(NULL,".",&save_firstptr);
		sprintf(soname,"%s/%s.so", mlt_tokeniser_get_string( tokeniser , dircount ) , firstname );

		if (firstname){
			
			void* handle=dlopen(soname,RTLD_LAZY);
			
			if (handle ){
				ret=load_lib ( profile , type , handle );
			}else{
				dlerror();
			}
		}
	}
	mlt_tokeniser_close ( tokeniser );
	return ret;
}


MLT_REPOSITORY
{
	int i=0;
	mlt_tokeniser tokeniser = mlt_tokeniser_init ( );
	int dircount=mlt_tokeniser_parse_new (
		tokeniser ,
   	getenv("MLT_FREI0R_PLUGIN_PATH") ? getenv("MLT_FREI0R_PLUGIN_PATH") : "/usr/lib/frei0r-1" ,
		":"
	);
	
	while (dircount--){
		
		mlt_properties direntries = mlt_properties_new();
		char* dirname = mlt_tokeniser_get_string ( tokeniser , dircount ) ;
		mlt_properties_dir_list(direntries, dirname ,"*.so",1);
	
		for (i=0;i<mlt_properties_count(direntries);i++){
			char* name=mlt_properties_get_value(direntries,i);	
			char* shortname=name+strlen(dirname)+1;
			char fname[1024]="";
			
			strcat(fname,dirname);
			strcat(fname,shortname);
			
			char *save_firstptr = NULL;
			char pluginname[1024]="frei0r.";
			char* firstname = strtok_r ( shortname , "." , &save_firstptr );
			strcat(pluginname,firstname);
	
			void* handle=dlopen(strcat(name,".so"),RTLD_LAZY);
			if (handle){
				void (*plginfo)(f0r_plugin_info_t*)=dlsym(handle,"f0r_get_plugin_info");
				
				if (plginfo){
					f0r_plugin_info_t info;
					plginfo(&info);
					
					if (firstname && info.plugin_type==F0R_PLUGIN_TYPE_FILTER){
						MLT_REGISTER( filter_type, pluginname, create_frei0r_item );
						fill_param_info ( repository , handle, &info , filter_type , pluginname );
					}
					else if (firstname && info.plugin_type==F0R_PLUGIN_TYPE_MIXER2 ){
						MLT_REGISTER( transition_type, pluginname, create_frei0r_item );
						fill_param_info ( repository , handle, &info , transition_type , pluginname );
					}
				}
				dlclose(handle);
			}
		}
		mlt_properties_close(direntries);
	}
	mlt_tokeniser_close ( tokeniser );
}
