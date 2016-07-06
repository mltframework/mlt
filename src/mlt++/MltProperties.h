/**
 * MltProperties.h - MLT Wrapper
 * Copyright (C) 2004-2015 Meltytech, LLC
 * Author: Charles Yates <charles.yates@gmail.com>
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

#ifndef MLTPP_PROPERTIES_H
#define MLTPP_PROPERTIES_H

#include "MltConfig.h"

#include <stdio.h>
#include <framework/mlt.h>

namespace Mlt 
{
	class Event;
	class Animation;

	/** Abstract Properties class.
	 */

	class MLTPP_DECLSPEC Properties 
	{
		private:
			mlt_properties instance;
		public:
			Properties( );
			Properties( bool dummy );
			Properties( Properties &properties );
			Properties( mlt_properties properties );
			Properties( void *properties );
			Properties( const char *file );
			virtual ~Properties( );
			virtual mlt_properties get_properties( );
			int inc_ref( );
			int dec_ref( );
			int ref_count( );
			void lock( );
			void unlock( );
			void block( void *object = NULL );
			void unblock( void *object = NULL );
			int fire_event( const char *event );
			bool is_valid( );
			int count( );
			char *get( const char *name );
			int get_int( const char *name );
			int64_t get_int64( const char *name );
			double get_double( const char *name );
			void *get_data( const char *name, int &size );
			void *get_data( const char *name );
			int set( const char *name, const char *value );
			int set( const char *name, int value );
			int set( const char *name, int64_t value );
			int set( const char *name, double value );
			int set( const char *name, void *value, int size, mlt_destructor destroy = NULL, mlt_serialiser serial = NULL );
			void pass_property( Properties &that, const char *name );
			int pass_values( Properties &that, const char *prefix );
			int pass_list( Properties &that, const char *list );
			int parse( const char *namevalue );
			char *get_name( int index );
			char *get( int index );
			void *get_data( int index, int &size );
			void mirror( Properties &that );
			int inherit( Properties &that );
			int rename( const char *source, const char *dest );
			void dump( FILE *output = stderr );
			void debug( const char *title = "Object", FILE *output = stderr );
			void load( const char *file );
			int save( const char *file );
			#if defined( __APPLE__ ) && GCC_VERSION < 40000
			Event *listen( const char *id, void *object, void (*)( ... ) );
			#else
			Event *listen( const char *id, void *object, mlt_listener );
			#endif
			static void delete_event( Event * );
			Event *setup_wait_for( const char *id );
			void wait_for( Event *, bool destroy = true );
			void wait_for( const char *id );
			bool is_sequence( );
			static Properties *parse_yaml( const char *file );
			char *serialise_yaml( );
			int preset( const char *name );
			int set_lcnumeric( const char *locale );
			const char *get_lcnumeric( );
			char *get_time( const char *name, mlt_time_format = mlt_time_smpte_df );
			char *frames_to_time( int, mlt_time_format = mlt_time_smpte_df );
			int time_to_frames( const char* time );

			mlt_color get_color( const char *name );
			int set( const char *name , mlt_color value );

			char* anim_get( const char *name, int position, int length = 0 );
			int anim_set( const char *name, const char *value, int position, int length = 0 );
			int anim_get_int( const char *name, int position, int length = 0 );
			int anim_set( const char *name, int value, int position, int length = 0,
				mlt_keyframe_type keyframe_type = mlt_keyframe_linear );
			double anim_get_double( const char *name, int position, int length = 0 );
			int anim_set( const char *name, double value, int position, int length = 0,
				mlt_keyframe_type keyframe_type = mlt_keyframe_linear );

			int set( const char *name, mlt_rect value );
			int set( const char *name, double x, double y, double w, double h, double opacity = 1.0 );
			mlt_rect get_rect( const char* name );
			int anim_set( const char *name, mlt_rect value, int position, int length = 0,
				mlt_keyframe_type keyframe_type = mlt_keyframe_linear );
			mlt_rect anim_get_rect( const char *name, int position, int length = 0 );
			mlt_animation get_animation( const char *name );
			Animation* get_anim( const char *name );
	};
}

#endif
