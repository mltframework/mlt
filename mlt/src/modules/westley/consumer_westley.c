/*
 * consumer_westley.c -- a libxml2 serialiser of mlt service networks
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "consumer_westley.h"
#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <libxml/tree.h>

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );

/** This is what will be called by the factory - anything can be passed in
	via the argument, but keep it simple.
*/

mlt_consumer consumer_westley_init( char *arg )
{
	// Create the consumer object
	mlt_consumer this = calloc( sizeof( struct mlt_consumer_s ), 1 );

	// If no malloc'd and consumer init ok
	if ( this != NULL && mlt_consumer_init( this, NULL ) == 0 )
	{
		// We have stuff to clean up, so override the close method
		//parent->close = consumer_close;

		// Allow thread to be started/stopped
		this->start = consumer_start;

		// Return the consumer produced
		return this;
	}

	// malloc or consumer init failed
	free( this );

	// Indicate failure
	return NULL;
}

void serialise_service( mlt_service service )
{
	// Iterate over consumer/producer connections
	while ( service != NULL )
	{
		char *mlt_type = mlt_properties_get( mlt_service_properties(service ), "mlt_type" );
		
		// Tell about the producer
		if ( strcmp( mlt_type, "producer" ) == 0 )
		{
			fprintf( stderr, "mlt_type '%s' mlt_service '%s' resource '%s'\n", mlt_type,
				mlt_properties_get( mlt_service_properties( service ), "mlt_service" ),
				mlt_properties_get( mlt_service_properties( service ), "resource" ) );
		}
		
		// Tell about the framework container producers
		else if ( strcmp( mlt_type, "mlt_producer" ) == 0 )
		{
			fprintf( stderr, "mlt_type '%s' resource '%s'\n", mlt_type,
				mlt_properties_get( mlt_service_properties( service ), "resource" ) );

			// Recurse on multitrack's tracks
			if ( strcmp( mlt_properties_get( mlt_service_properties( service ), "resource" ), "<multitrack>" ) == 0 )
			{
				int i;

				fprintf( stderr, "contains...\n" );
				for ( i = 0; i < mlt_multitrack_count( MLT_MULTITRACK( service ) ); i++ )
					serialise_service( MLT_SERVICE( mlt_multitrack_track( MLT_MULTITRACK( service ), i ) ) );
				fprintf( stderr, "...done.\n" );
			}
			
			// Recurse on playlist's clips
			else if ( strcmp( mlt_properties_get( mlt_service_properties( service ), "resource" ), "<playlist>" ) == 0 )
			{
				int i;
				mlt_playlist_clip_info info;
				
				fprintf( stderr, "contains...\n" );
				for ( i = 0; i < mlt_playlist_count( MLT_PLAYLIST( service ) ); i++ )
				{
					if ( ! mlt_playlist_get_clip_info( MLT_PLAYLIST( service ), &info, i ) )
						serialise_service( MLT_SERVICE( info.producer ) );
				}
				fprintf( stderr, "...done.\n" );
			}
		}
		
		// Tell about a filter
		else if ( strcmp( mlt_type, "filter" ) == 0 )
		{
			fprintf( stderr, "mlt_type '%s' mlt_service '%s'\n", mlt_type,
				mlt_properties_get( mlt_service_properties( service ), "mlt_service" ) );

			fprintf( stderr, "is applied to\n" );
			
			// Recurse on connected producer
			serialise_service( MLT_SERVICE( MLT_FILTER( service )->producer ) );
		}
		
		// Tell about a transition
		else if ( strcmp( mlt_type, "transition" ) == 0 )
		{
			fprintf( stderr, "mlt_type '%s' mlt_service '%s'\n", mlt_type,
				mlt_properties_get( mlt_service_properties( service ), "mlt_service" ) );
				
			fprintf( stderr, "is applied to\n" );
			
			// Recurse on connected producer
			serialise_service( MLT_SERVICE( MLT_TRANSITION( service )->producer ) );
		}
		
		// Get the next connected service
		service = mlt_service_get_producer( service );
		if ( service != NULL )
			fprintf( stderr, "is connected to\n" );
	}
}

static int consumer_start( mlt_consumer this )
{
	// get a handle on properties
	mlt_properties properties = mlt_consumer_properties( this );
	
	mlt_service inigo = NULL;
	
	fprintf( stderr, "mlt_type '%s' mlt_service '%s'\n",
		mlt_properties_get( properties, "mlt_type" ),
		mlt_properties_get( properties, "mlt_service" ) );

	// Get the producer service
	mlt_service service = mlt_service_get_producer( mlt_consumer_service( this ) );
	if ( service != NULL )
	{
		fprintf( stderr, "is connected to\n" );

		// Remember inigo
		if ( mlt_properties_get( mlt_service_properties( service ), "mlt_service" ) != NULL &&
				strcmp( mlt_properties_get( mlt_service_properties( service ), "mlt_service" ), "inigo" ) == 0 )
			inigo = service;

		serialise_service( service );
	}

	mlt_consumer_stop( this );

	// Tell inigo, enough already!
	if ( inigo != NULL )
		mlt_properties_set_int( mlt_service_properties( inigo ), "done", 1 );
	
	return 0;
}

