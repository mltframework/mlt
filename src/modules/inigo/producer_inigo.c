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

static mlt_producer parse_inigo( char *file )
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
		mlt_field field = mlt_properties_get_data( properties, "field", NULL );
		mlt_properties_set( properties, "resource", file );
		mlt_properties_set( mlt_field_properties( field ), "resource", file );
	}

	while( count -- )
		free( args[ count ] );
	free( args );

	return result;
}

static mlt_producer create_producer( char *file )
{
	mlt_producer result = NULL;

	// 1st Line preferences
	if ( strstr( file, ".inigo" ) )
		result = parse_inigo( file );
	else if ( strstr( file, ".mpg" ) )
		result = mlt_factory_producer( "mcmpeg", file );
	else if ( strstr( file, ".mpeg" ) )
		result = mlt_factory_producer( "mcmpeg", file );
	else if ( strstr( file, ".dv" ) )
		result = mlt_factory_producer( "mcdv", file );
	else if ( strstr( file, ".dif" ) )
		result = mlt_factory_producer( "mcdv", file );
	else if ( strstr( file, ".jpg" ) )
		result = mlt_factory_producer( "pixbuf", file );
	else if ( strstr( file, ".JPG" ) )
		result = mlt_factory_producer( "pixbuf", file );
	else if ( strstr( file, ".jpeg" ) )
		result = mlt_factory_producer( "pixbuf", file );
	else if ( strstr( file, ".png" ) )
		result = mlt_factory_producer( "pixbuf", file );
	else if ( strstr( file, ".txt" ) )
		result = mlt_factory_producer( "pango", file );

	// 2nd Line fallbacks
	if ( result == NULL && strstr( file, ".dv" ) )
		result = mlt_factory_producer( "libdv", file );
	else if ( result == NULL && strstr( file, ".dif" ) )
		result = mlt_factory_producer( "libdv", file );

	// 3rd line fallbacks 
	if ( result == NULL )
		result = mlt_factory_producer( "ffmpeg", file );

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

static void set_properties( mlt_properties properties, char *namevalue )
{
	mlt_properties_parse( properties, namevalue );
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
		if ( !strcmp( argv[ i ], "-serialise" ) )
		{
			i ++;
		}
		else if ( !strcmp( argv[ i ], "-group" ) )
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
		else if ( !strcmp( argv[ i ], "-track" ) )
		{
			if ( producer != NULL )
				mlt_playlist_append( playlist, producer );
			producer = NULL;
			mlt_multitrack_connect( multitrack, mlt_playlist_producer( playlist ), track ++ );
			playlist = mlt_playlist_init( );
		}
		else if ( strstr( argv[ i ], "=" ) )
		{
			set_properties( properties, argv[ i ] );
		}
		else if ( argv[ i ][ 0 ] != '-' )
		{
			if ( producer != NULL )
				mlt_playlist_append( playlist, producer );
			producer = create_producer( argv[ i ] );
			if ( producer != NULL )
			{
				properties = mlt_producer_properties( producer );
				mlt_properties_inherit( properties, group );
			}
		}
		else
		{
			while ( argv[ i ] != NULL && argv[ i ][ 0 ] != '-' )
				i ++;
			i --;
		}
	}

	// Connect producer to playlist
	if ( producer != NULL )
		mlt_playlist_append( playlist, producer );

	// We must have a producer at this point
	if ( mlt_playlist_count( playlist ) > 0 )
	{
		// Connect multitrack to producer
		mlt_multitrack_connect( multitrack, mlt_playlist_producer( playlist ), track );
	}

	mlt_properties props = mlt_multitrack_properties( multitrack );
	mlt_properties_set_data( props, "field", field, 0, NULL, NULL );
	mlt_properties_set_data( props, "group", group, 0, NULL, NULL );

	return mlt_multitrack_producer( multitrack );
}

