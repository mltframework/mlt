/*
 * producer_fezzik.c -- a normalising filter
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

#include "producer_fezzik.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <framework/mlt.h>

static void track_service( mlt_tractor tractor, void *service, mlt_destructor destructor )
{
	mlt_properties properties = mlt_tractor_properties( tractor );
	int registered = mlt_properties_get_int( properties, "_registered" );
	char *key = mlt_properties_get( properties, "_registered" );
	char *real = malloc( strlen( key ) + 2 );
	sprintf( real, "_%s", key );
	mlt_properties_set_data( properties, real, service, 0, destructor, NULL );
	mlt_properties_set_int( properties, "_registered", ++ registered );
	free( real );
}

static mlt_producer create_producer( char *file )
{
	mlt_producer result = NULL;

	// 0th Line - check for service:resource handling
	if ( strchr( file, ':' ) )
	{
		char *temp = strdup( file );
		char *service = temp;
		char *resource = strchr( temp, ':' );
		*resource ++ = '\0';
		result = mlt_factory_producer( service, resource );
		free( temp );
	}

	// 1st Line preferences
	if ( result == NULL )
	{
		if ( strstr( file, ".inigo" ) )
			result = mlt_factory_producer( "inigo_file", file );
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
		else if ( strstr( file, ".svg" ) )
			result = mlt_factory_producer( "pixbuf", file );
		else if ( strstr( file, ".txt" ) )
			result = mlt_factory_producer( "pango", file );
		else if ( strstr( file, ".westley" ) )
			result = mlt_factory_producer( "westley", file );
		else if ( strstr( file, ".ogg" ) )
			result = mlt_factory_producer( "vorbis", file );
	}

	// 2nd Line fallbacks
	if ( result == NULL )
	{
		if ( strstr( file, ".dv" ) )
			result = mlt_factory_producer( "libdv", file );
		else if ( strstr( file, ".dif" ) )
			result = mlt_factory_producer( "libdv", file );
	}

	// 3rd line fallbacks 
	if ( result == NULL )
		result = mlt_factory_producer( "avformat", file );

	return result;
}

static mlt_service create_filter( mlt_tractor tractor, mlt_service last, char *effect )
{
	char *id = strdup( effect );
	char *arg = strchr( id, ':' );
	if ( arg != NULL )
		*arg ++ = '\0';
	mlt_filter filter = mlt_factory_filter( id, arg );
	if ( filter != NULL )
	{
		mlt_filter_connect( filter, last, 0 );
		track_service( tractor, filter, ( mlt_destructor )mlt_filter_close );
		last = mlt_filter_service( filter );
	}
	free( id );
	return last;
}

mlt_producer producer_fezzik_init( char *arg )
{
	// Create the producer that the tractor will contain
	mlt_producer producer = NULL;
	if ( arg != NULL )
		producer = create_producer( arg );

	// Build the tractor if we have a producer and it isn't already westley'd :-)
	if ( producer != NULL && mlt_properties_get( mlt_producer_properties( producer ), "westley" ) == NULL )
	{
		// Construct the tractor
		mlt_tractor tractor = mlt_tractor_init( );

		// Sanity check
		if ( tractor != NULL )
		{
			// Extract the tractor properties
			mlt_properties properties = mlt_tractor_properties( tractor );

			// Our producer will be the last service
			mlt_service last = mlt_producer_service( producer );

			// Set the registered count
			mlt_properties_set_int( properties, "_registered", 0 );

			// Register our producer for seeking in the tractor
			mlt_properties_set_data( properties, "producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

			// Now attach normalising filters
			last = create_filter( tractor, last, "deinterlace" );
			last = create_filter( tractor, last, "rescale" );
			last = create_filter( tractor, last, "resize" );
			last = create_filter( tractor, last, "resample" );

			// Connect the tractor to the last
			mlt_tractor_connect( tractor, last );

			// Finally, inherit properties from producer
			mlt_properties_inherit( properties, mlt_producer_properties( producer ) );

			// Now make sure we don't lose our inherited identity
			mlt_properties_set_int( properties, "_mlt_service_hidden", 1 );

			// This is a temporary hack to ensure that westley doesn't dig too deep
			// and fezzik doesn't overdo it with throwing rocks...
			mlt_properties_set( properties, "westley", "was here" );

			// We need to ensure that all further properties are mirrored in the producer
			mlt_properties_mirror( properties, mlt_producer_properties( producer ) );

			// Now, we return the producer of the tractor
			producer = mlt_tractor_producer( tractor );
		}
	}

	// Return the tractor's producer
	return producer;
}
