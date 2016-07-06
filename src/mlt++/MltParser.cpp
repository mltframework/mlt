/**
 * MltParser.cpp - MLT Wrapper
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

#include "Mlt.h"
using namespace Mlt;

static int on_invalid_cb( mlt_parser self, mlt_service object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Service service( object );
	return parser->on_invalid( &service );
}

static int on_unknown_cb( mlt_parser self, mlt_service object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Service service( object );
	return parser->on_unknown( &service );
}

static int on_start_producer_cb( mlt_parser self, mlt_producer object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Producer producer( object );
	return parser->on_start_producer( &producer );
}

static int on_end_producer_cb( mlt_parser self, mlt_producer object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Producer producer( object );
	return parser->on_end_producer( &producer );
}

static int on_start_playlist_cb( mlt_parser self, mlt_playlist object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Playlist playlist( object );
	return parser->on_start_playlist( &playlist );
}

static int on_end_playlist_cb( mlt_parser self, mlt_playlist object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Playlist playlist( object );
	return parser->on_end_playlist( &playlist );
}

static int on_start_tractor_cb( mlt_parser self, mlt_tractor object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Tractor tractor( object );
	return parser->on_start_tractor( &tractor );
}

static int on_end_tractor_cb( mlt_parser self, mlt_tractor object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Tractor tractor( object );
	return parser->on_end_tractor( &tractor );
}

static int on_start_multitrack_cb( mlt_parser self, mlt_multitrack object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Multitrack multitrack( object );
	return parser->on_start_multitrack( &multitrack );
}

static int on_end_multitrack_cb( mlt_parser self, mlt_multitrack object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Multitrack multitrack( object );
	return parser->on_end_multitrack( &multitrack );
}

static int on_start_track_cb( mlt_parser self )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	return parser->on_start_track( );
}

static int on_end_track_cb( mlt_parser self )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	return parser->on_end_track( );
}

static int on_start_filter_cb( mlt_parser self, mlt_filter object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Filter filter( object );
	return parser->on_start_filter( &filter );
}

static int on_end_filter_cb( mlt_parser self, mlt_filter object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Filter filter( object );
	return parser->on_end_filter( &filter );
}

static int on_start_transition_cb( mlt_parser self, mlt_transition object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Transition transition( object );
	return parser->on_start_transition( &transition );
}

static int on_end_transition_cb( mlt_parser self, mlt_transition object )
{
	mlt_properties properties = mlt_parser_properties( self );
	Parser *parser = ( Parser * )mlt_properties_get_data( properties, "_parser_object", NULL );
	Transition transition( object );
	return parser->on_end_transition( &transition );
}

Parser::Parser( ) :
	Properties( false )
{
	parser = mlt_parser_new( );
	set( "_parser_object", this, 0 );
	parser->on_invalid = on_invalid_cb;
	parser->on_unknown = on_unknown_cb;
	parser->on_start_producer = on_start_producer_cb;
	parser->on_end_producer = on_end_producer_cb;
	parser->on_start_playlist = on_start_playlist_cb;
	parser->on_end_playlist = on_end_playlist_cb;
	parser->on_start_tractor = on_start_tractor_cb;
	parser->on_end_tractor = on_end_tractor_cb;
	parser->on_start_multitrack = on_start_multitrack_cb;
	parser->on_end_multitrack = on_end_multitrack_cb;
	parser->on_start_track = on_start_track_cb;
	parser->on_end_track = on_end_track_cb;
	parser->on_start_filter = on_start_filter_cb;
	parser->on_end_filter = on_end_filter_cb;
	parser->on_start_transition = on_start_transition_cb;
	parser->on_end_transition = on_end_transition_cb;
}

Parser::~Parser( )
{
	mlt_parser_close( parser );
}

mlt_properties Parser::get_properties( )
{
	return mlt_parser_properties( parser );
}

int Parser::start( Service &service )
{
	return mlt_parser_start( parser, service.get_service( ) );
}

int Parser::on_invalid( Service *object )
{
	object->debug( "Invalid" );
	return 0;
}

int Parser::on_unknown( Service *object )
{
	object->debug( "Unknown" );
	return 0;
}

int Parser::on_start_producer( Producer *object )
{
	object->debug( "on_start_producer" );
	return 0;
}

int Parser::on_end_producer( Producer *object )
{
	object->debug( "on_end_producer" );
	return 0;
}

int Parser::on_start_playlist( Playlist *object )
{
	object->debug( "on_start_playlist" );
	return 0;
}

int Parser::on_end_playlist( Playlist *object )
{
	object->debug( "on_end_playlist" );
	return 0;
}

int Parser::on_start_tractor( Tractor *object )
{
	object->debug( "on_start_tractor" );
	return 0;
}

int Parser::on_end_tractor( Tractor *object )
{
	object->debug( "on_end_tractor" );
	return 0;
}

int Parser::on_start_multitrack( Multitrack *object )
{
	object->debug( "on_start_multitrack" );
	return 0;
}

int Parser::on_end_multitrack( Multitrack *object )
{
	object->debug( "on_end_multitrack" );
	return 0;
}

int Parser::on_start_track( )
{
	fprintf( stderr, "on_start_track\n" );
	return 0;
}

int Parser::on_end_track( )
{
	fprintf( stderr, "on_end_track\n" );
	return 0;
}

int Parser::on_start_filter( Filter *object )
{
	object->debug( "on_start_filter" );
	return 0;
}

int Parser::on_end_filter( Filter *object )
{
	object->debug( "on_end_filter" );
	return 0;
}

int Parser::on_start_transition( Transition *object )
{
	object->debug( "on_start_transition" );
	return 0;
}

int Parser::on_end_transition( Transition *object )
{
	object->debug( "on_end_transition" );
	return 0;
}


