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
%newobject Service::producer( );
%newobject Service::consumer( );
%newobject Service::get_frame( int );
%newobject Producer::filter( int );
%newobject Playlist::current( );
%newobject Playlist::clip_info( int );
%newobject Multitrack::track( int );
%newobject Tractor::multitrack( );
%newobject Tractor::field( );
%newobject Tractor::track( int );
}

/** Classes to wrap.
 */

%include <framework/mlt_types.h>
%include <framework/mlt_factory.h>
%include <MltFactory.h>
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
%include <MltFilteredConsumer.h>

