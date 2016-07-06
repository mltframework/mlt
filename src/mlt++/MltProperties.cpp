/**
 * MltProperties.cpp - MLT Wrapper
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

#include "MltProperties.h"
#include "MltEvent.h"
#include "MltAnimation.h"
using namespace Mlt;

Properties::Properties( ) :
	instance( NULL )
{
	instance = mlt_properties_new( );
}

Properties::Properties( bool /*dummy*/ ) :
	instance( NULL )
{
}

Properties::Properties( Properties &properties ) :
	instance( properties.get_properties( ) )
{
	inc_ref( );
}

Properties::Properties( mlt_properties properties ) :
	instance( properties )
{
	inc_ref( );
}

Properties::Properties( void *properties ) :
	instance( mlt_properties( properties ) )
{
	inc_ref( );
}

Properties::Properties( const char *file ) :
	instance( NULL )
{
	instance = mlt_properties_load( file );
}

Properties::~Properties( )
{
	mlt_properties_close( instance );
}

mlt_properties Properties::get_properties( )
{
	return instance;
}

int Properties::inc_ref( )
{
	return mlt_properties_inc_ref( get_properties( ) );
}

int Properties::dec_ref( )
{
	return mlt_properties_dec_ref( get_properties( ) );
}

int Properties::ref_count( )
{
	return mlt_properties_ref_count( get_properties( ) );
}

void Properties::lock( )
{
	mlt_properties_lock( get_properties( ) );
}

void Properties::unlock( )
{
	mlt_properties_unlock( get_properties( ) );
}

void Properties::block( void *object )
{
	mlt_events_block( get_properties( ), object != NULL ? object : get_properties( ) );
}

void Properties::unblock( void *object )
{
	mlt_events_unblock( get_properties( ), object != NULL ? object : get_properties( ) );
}

int Properties::fire_event( const char *event )
{
	return mlt_events_fire( get_properties( ), event, NULL );
}

bool Properties::is_valid( )
{
	return get_properties( ) != NULL;
}

int Properties::count( )
{
	return mlt_properties_count( get_properties( ) );
}

char *Properties::get( const char *name )
{
	return mlt_properties_get( get_properties( ), name );
}

int Properties::get_int( const char *name )
{
	return mlt_properties_get_int( get_properties( ), name );
}

int64_t Properties::get_int64( const char *name )
{
	return mlt_properties_get_int64( get_properties( ), name );
}

double Properties::get_double( const char *name )
{
	return mlt_properties_get_double( get_properties( ), name );
}

void *Properties::get_data( const char *name, int &size )
{
	return mlt_properties_get_data( get_properties( ), name, &size );
}

void *Properties::get_data( const char *name )
{
	return mlt_properties_get_data( get_properties( ), name, NULL );
}

int Properties::set( const char *name, const char *value )
{
	return mlt_properties_set( get_properties( ), name, value );
}

int Properties::set( const char *name, int value )
{
	return mlt_properties_set_int( get_properties( ), name, value );
}

int Properties::set( const char *name, int64_t value )
{
	return mlt_properties_set_int64( get_properties( ), name, value );
}

int Properties::set( const char *name, double value )
{
	return mlt_properties_set_double( get_properties( ), name, value );
}

int Properties::set( const char *name, void *value, int size, mlt_destructor destructor, mlt_serialiser serialiser )
{
	return mlt_properties_set_data( get_properties( ), name, value, size, destructor, serialiser );
}

void Properties::pass_property( Properties &that, const char *name )
{
	return mlt_properties_pass_property( get_properties( ), that.get_properties( ), name );
}

int Properties::pass_values( Properties &that, const char *prefix )
{
	return mlt_properties_pass( get_properties( ), that.get_properties( ), prefix );
}

int Properties::pass_list( Properties &that, const char *list )
{
	return mlt_properties_pass_list( get_properties( ), that.get_properties( ), list );
}

int Properties::parse( const char *namevalue )
{
	return mlt_properties_parse( get_properties( ), namevalue );
}

char *Properties::get_name( int index )
{
	return mlt_properties_get_name( get_properties( ), index );
}

char *Properties::get( int index )
{
	return mlt_properties_get_value( get_properties( ), index );
}

void *Properties::get_data( int index, int &size )
{
	return mlt_properties_get_data_at( get_properties( ), index, &size );
}

void Properties::mirror( Properties &that )
{
	mlt_properties_mirror( get_properties( ), that.get_properties( ) );
}

int Properties::inherit( Properties &that )
{
	return mlt_properties_inherit( get_properties( ), that.get_properties( ) );
}

int Properties::rename( const char *source, const char *dest )
{
	return mlt_properties_rename( get_properties( ), source, dest );
}

void Properties::dump( FILE *output )
{
	mlt_properties_dump( get_properties( ), output );
}

void Properties::debug( const char *title, FILE *output )
{
	mlt_properties_debug( get_properties( ), title, output );
}

void Properties::load( const char *file )
{
	mlt_properties properties = mlt_properties_load( file );
	if ( properties != NULL )
		mlt_properties_pass( get_properties( ), properties, "" );
	mlt_properties_close( properties );
}

int Properties::save( const char *file )
{
	return mlt_properties_save( get_properties( ), file );
}

#if defined( __APPLE__ ) && GCC_VERSION < 40000

Event *Properties::listen( const char *id, void *object, void (*listener)( ... ) )
{
	mlt_event event = mlt_events_listen( get_properties( ), object, id, ( mlt_listener )listener );
	return new Event( event );
}

#else

Event *Properties::listen( const char *id, void *object, mlt_listener listener )
{
	mlt_event event = mlt_events_listen( get_properties( ), object, id, listener );
	return new Event( event );
}

#endif

Event *Properties::setup_wait_for( const char *id )
{
	return new Event( mlt_events_setup_wait_for( get_properties( ), id ) );
}

void Properties::delete_event( Event *event )
{
	delete event;
}

void Properties::wait_for( Event *event, bool destroy )
{
	mlt_events_wait_for( get_properties( ), event->get_event( ) );
	if ( destroy )
		mlt_events_close_wait_for( get_properties( ), event->get_event( ) );
}

void Properties::wait_for( const char *id )
{
	Event *event = setup_wait_for( id );
	wait_for( event );
	delete event;
}

bool Properties::is_sequence( )
{
	return mlt_properties_is_sequence( get_properties( ) );
}

Properties *Properties::parse_yaml( const char *file )
{
	return new Properties( mlt_properties_parse_yaml( file ) );
}

char *Properties::serialise_yaml( )
{
	return mlt_properties_serialise_yaml( get_properties( ) );
}

int Properties::preset( const char *name )
{
	return mlt_properties_preset( get_properties(), name );
}

int Properties::set_lcnumeric( const char *locale )
{
	return mlt_properties_set_lcnumeric( get_properties(), locale );
}

const char *Properties::get_lcnumeric( )
{
	return mlt_properties_get_lcnumeric( get_properties() );
}

char *Properties::get_time( const char *name, mlt_time_format format )
{
	return mlt_properties_get_time( get_properties(), name, format );
}

char *Properties::frames_to_time( int frames, mlt_time_format format )
{
	return mlt_properties_frames_to_time( get_properties(), frames, format );
}

int Properties::time_to_frames( const char *time )
{
	return mlt_properties_time_to_frames( get_properties(), time );
}

mlt_color Properties::get_color( const char *name )
{
	return mlt_properties_get_color( get_properties(), name );
}

int Properties::set( const char *name, mlt_color value )
{
	return mlt_properties_set_color( get_properties(), name, value );
}

char *Properties::anim_get( const char *name, int position, int length )
{
	return mlt_properties_anim_get( get_properties(), name, position, length );
}

int Properties::anim_set( const char *name, const char *value, int position, int length )
{
	return mlt_properties_anim_set( get_properties(), name, value, position, length );
}

int Properties::anim_get_int( const char *name, int position, int length )
{
	return mlt_properties_anim_get_int( get_properties(), name, position, length );
}

int Properties::anim_set( const char *name, int value, int position, int length, mlt_keyframe_type keyframe_type )
{
	return mlt_properties_anim_set_int( get_properties(), name, value, position, length, keyframe_type );
}

double Properties::anim_get_double(const char *name, int position, int length)
{
	return mlt_properties_anim_get_double( get_properties(), name, position, length );
}

int Properties::anim_set( const char *name, double value, int position, int length, mlt_keyframe_type keyframe_type )
{
	return mlt_properties_anim_set_double( get_properties(), name, value, position, length, keyframe_type );
}

int Properties::set( const char *name, mlt_rect value )
{
	return mlt_properties_set_rect( get_properties(), name, value );
}

int Properties::set( const char *name, double x, double y, double w, double h, double opacity )
{
	mlt_rect value = { x, y, w, h, opacity };
	return mlt_properties_set_rect( get_properties(), name, value );
}

mlt_rect Properties::get_rect( const char *name )
{
	return mlt_properties_get_rect( get_properties(), name );
}

int Properties::anim_set( const char *name, mlt_rect value, int position, int length, mlt_keyframe_type keyframe_type )
{
	return mlt_properties_anim_set_rect( get_properties(), name, value, position, length, keyframe_type );
}

mlt_rect Properties::anim_get_rect(const char *name, int position, int length)
{
	return mlt_properties_anim_get_rect( get_properties(), name, position, length );
}

mlt_animation Properties::get_animation( const char *name )
{
	return mlt_properties_get_animation( get_properties(), name );
}

Animation *Properties::get_anim(const char *name)
{
	return new Animation( mlt_properties_get_animation( get_properties(), name ) );
}
