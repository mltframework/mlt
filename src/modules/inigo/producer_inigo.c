/*
 * producer_inigo.c -- simple inigo test case
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

#include "producer_inigo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <framework/mlt.h>

mlt_producer producer_inigo_file_init( char *file )
{
	FILE *input = fopen( file, "r" );
	char **args = calloc( sizeof( char * ), 1000 );
	int count = 0;
	char temp[ 2048 ];

	if ( input != NULL )
	{
		while( fgets( temp, 2048, input ) )
		{
			temp[ strlen( temp ) - 1 ] = '\0';
			if ( strcmp( temp, "" ) )
				args[ count ++ ] = strdup( temp );
		}
	}

	mlt_producer result = producer_inigo_init( args );

	if ( result != NULL )
	{
		mlt_properties properties = mlt_producer_properties( result );
		mlt_properties_set( properties, "resource", file );
	}

	while( count -- )
		free( args[ count ] );
	free( args );

	return result;
}

static void track_service( mlt_field field, void *service, mlt_destructor destructor )
{
	mlt_properties properties = mlt_field_properties( field );
	int registered = mlt_properties_get_int( properties, "registered" );
	char *key = mlt_properties_get( properties, "registered" );
	mlt_properties_set_data( properties, key, service, 0, destructor, NULL );
	mlt_properties_set_int( properties, "registered", ++ registered );
}

static mlt_producer create_producer( mlt_field field, char *file )
{
	mlt_producer result = mlt_factory_producer( "fezzik", file );

	if ( result != NULL )
		track_service( field, result, ( mlt_destructor )mlt_producer_close );

	return result;
}

static mlt_filter create_filter( mlt_field field, char *id, int track )
{
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';
	mlt_filter filter = mlt_factory_filter( id, arg );
	if ( filter != NULL )
	{
		mlt_field_plant_filter( field, filter, track );
		track_service( field, filter, ( mlt_destructor )mlt_filter_close );
	}
	return filter;
}

static mlt_transition create_transition( mlt_field field, char *id, int track )
{
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';
	mlt_transition transition = mlt_factory_transition( id, arg );
	if ( transition != NULL )
	{
		mlt_field_plant_transition( field, transition, track, track + 1 );
		track_service( field, transition, ( mlt_destructor )mlt_transition_close );
	}
	return transition;
}

mlt_producer producer_inigo_init( char **argv )
{
	int i;
	int track = 0;
	mlt_producer producer = NULL;
	mlt_playlist playlist = mlt_playlist_init( );
	mlt_properties group = mlt_properties_new( );
	mlt_properties properties = group;
	mlt_field field = mlt_field_init( );
	mlt_properties field_properties = mlt_field_properties( field );
	mlt_multitrack multitrack = mlt_field_multitrack( field );

	// We need to track the number of registered filters
	mlt_properties_set_int( field_properties, "registered", 0 );

	// Parse the arguments
	for ( i = 0; argv[ i ] != NULL; i ++ )
	{
		if ( !strcmp( argv[ i ], "-group" ) )
		{
			if ( mlt_properties_count( group ) != 0 )
			{
				mlt_properties_close( group );
				group = mlt_properties_new( );
			}
			if ( group != NULL )
				properties = group;
		}
		else if ( !strcmp( argv[ i ], "-filter" ) )
		{
			mlt_filter filter = create_filter( field, argv[ ++ i ], track );
			if ( filter != NULL )
			{
				properties = mlt_filter_properties( filter );
				mlt_properties_inherit( properties, group );
			}
		}
		else if ( !strcmp( argv[ i ], "-transition" ) )
		{
			mlt_transition transition = create_transition( field, argv[ ++ i ], track );
			if ( transition != NULL )
			{
				properties = mlt_transition_properties( transition );
				mlt_properties_inherit( properties, group );
			}
		}
		else if ( !strcmp( argv[ i ], "-blank" ) )
		{
			if ( producer != NULL )
				mlt_playlist_append( playlist, producer );
			producer = NULL;
			mlt_playlist_blank( playlist, atof( argv[ ++ i ] ) );
		}
		else if ( !strcmp( argv[ i ], "-track" ) ||
				  !strcmp( argv[ i ], "-hide-track" ) ||
				  !strcmp( argv[ i ], "-hide-video" ) ||
				  !strcmp( argv[ i ], "-hide-audio" ) )
		{
			if ( producer != NULL )
				mlt_playlist_append( playlist, producer );
			producer = NULL;
			mlt_multitrack_connect( multitrack, mlt_playlist_producer( playlist ), track ++ );
			track_service( field, playlist, ( mlt_destructor )mlt_playlist_close );
			playlist = mlt_playlist_init( );
			if ( playlist != NULL )
			{
				properties = mlt_playlist_properties( playlist );
				if ( !strcmp( argv[ i ], "-hide-track" ) )
					mlt_properties_set_int( properties, "hide", 3 );
				else if ( !strcmp( argv[ i ], "-hide-video" ) )
					mlt_properties_set_int( properties, "hide", 1 );
				else if ( !strcmp( argv[ i ], "-hide-audio" ) )
					mlt_properties_set_int( properties, "hide", 2 );
			}
		}
		else if ( strchr( argv[ i ], '=' ) )
		{
			mlt_properties_parse( properties, argv[ i ] );
		}
		else if ( argv[ i ][ 0 ] != '-' )
		{
			if ( producer != NULL )
				mlt_playlist_append( playlist, producer );
			producer = create_producer( field, argv[ i ] );
			if ( producer != NULL )
			{
				properties = mlt_producer_properties( producer );
				mlt_properties_inherit( properties, group );
			}
		}
		else
		{
			if ( !strcmp( argv[ i ], "-serialise" ) )
				i += 2;
			else if ( !strcmp( argv[ i ], "-consumer" ) )
				i += 2;

			while ( argv[ i ] != NULL && strchr( argv[ i ], '=' ) )
				i ++;

			i --;
		}
	}

	// Connect last producer to playlist
	if ( producer != NULL )
		mlt_playlist_append( playlist, producer );

	// Track the last playlist too
	track_service( field, playlist, ( mlt_destructor )mlt_playlist_close );

	// We must have a playlist to connect
	if ( mlt_playlist_count( playlist ) > 0 )
		mlt_multitrack_connect( multitrack, mlt_playlist_producer( playlist ), track );

	mlt_tractor tractor = mlt_field_tractor( field );
	mlt_producer prod = mlt_tractor_producer( tractor );
	mlt_properties props = mlt_tractor_properties( tractor );
	mlt_properties_set_data( props, "multitrack", multitrack, 0, ( mlt_destructor )mlt_multitrack_close, NULL );
	mlt_properties_set_data( props, "field", field, 0, ( mlt_destructor )mlt_field_close, NULL );
	mlt_properties_set_data( props, "group", group, 0, ( mlt_destructor )mlt_properties_close, NULL );
	mlt_properties_set_position( props, "length", mlt_producer_get_out( mlt_multitrack_producer( multitrack ) ) + 1 );
	mlt_producer_set_in_and_out( prod, 0, mlt_producer_get_out( mlt_multitrack_producer( multitrack ) ) );
	mlt_properties_set_double( props, "fps", mlt_producer_get_fps( mlt_multitrack_producer( multitrack ) ) );

	return mlt_tractor_producer( tractor );
}
