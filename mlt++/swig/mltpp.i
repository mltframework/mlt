/**
 * mltpp.i - Swig Bindings for mlt++
 * Copyright (C) 2004-2005 Charles Yates
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
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

%module mltpp
%include "carrays.i"
%array_class(unsigned char, unsignedCharArray);

%{
#include <mlt++/Mlt.h>
%}

/** These methods return objects which should be gc'd.
 */

namespace Mlt {
%newobject Factory::producer( char *, char * );
%newobject Factory::filter( char *, char * );
%newobject Factory::transition( char *, char * );
%newobject Factory::consumer( char *, char * );
%newobject Properties::listen( char *, void *, mlt_listener );
%newobject Service::producer( );
%newobject Service::consumer( );
%newobject Service::get_frame( int );
%newobject Service::filter( int );
%newobject Producer::filter( int );
%newobject Producer::cut( int, int );
%newobject Playlist::current( );
%newobject Playlist::clip_info( int );
%newobject Playlist::get_clip( int );
%newobject Multitrack::track( int );
%newobject Tractor::multitrack( );
%newobject Tractor::field( );
%newobject Tractor::track( int );
%newobject Frame::get_original_producer( );
%newobject Miracle::execute( char * );
%newobject Miracle::push( char *, Service & );
}

/** Classes to wrap.
 */

%include <framework/mlt_types.h>
%include <framework/mlt_factory.h>
%include <MltFactory.h>
%include <MltEvent.h>
%include <MltProperties.h>
%include <MltFrame.h>
%include <MltService.h>
%include <MltProducer.h>
%include <MltPlaylist.h>
%include <MltConsumer.h>
%include <MltFilter.h>
%include <MltTransition.h>
%include <MltMultitrack.h>
%include <MltField.h>
%include <MltTractor.h>
%include <MltParser.h>
%include <MltFilteredConsumer.h>
%include <MltMiracle.h>
%include <MltResponse.h>

#if defined(SWIGRUBY)

%{

static void ruby_listener( mlt_properties owner, void *object );

class RubyListener
{
	private:
		VALUE callback;
		Mlt::Event *event;

	public:
		RubyListener( Mlt::Properties &properties, char *id, VALUE callback ) : 
			callback( callback ) 
		{
			event = properties.listen( id, this, ( mlt_listener )ruby_listener );
		}

		~RubyListener( )
		{
			delete event;
		}

    	void mark( ) 
		{ 
			((void (*)(VALUE))(rb_gc_mark))( callback ); 
		}

    	void doit( ) 
		{
        	ID method = rb_intern( "call" );
        	rb_funcall( callback, method, 0 );
    	}
};

static void ruby_listener( mlt_properties owner, void *object )
{
	RubyListener *o = static_cast< RubyListener * >( object );
	o->doit( );
}

void markRubyListener( void* p ) 
{
    RubyListener *o = static_cast<RubyListener*>( p );
    o->mark( );
}

%}

// Ruby wrapper
%rename( Listener )  RubyListener;
%markfunc RubyListener "markRubyListener";

class RubyListener 
{
	public:
		RubyListener( Mlt::Properties &properties, char *id, VALUE callback );
};

#endif

