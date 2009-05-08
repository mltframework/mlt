/*
 * consumer_valerie.c -- pushes a service via valerie
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author:  Charles Yates <charles.yates@telenet.be>
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

#include <valerie/valerie.h>
#include <valerie/valerie_remote.h>
#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

static int consumer_is_stopped( mlt_consumer this );
static int consumer_start( mlt_consumer this );

/** This is what will be called by the factory
*/

mlt_consumer consumer_valerie_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create the consumer object
	mlt_consumer this = calloc( sizeof( struct mlt_consumer_s ), 1 );

	// If no malloc'd and consumer init ok
	if ( this != NULL && mlt_consumer_init( this, NULL, profile ) == 0 )
	{
		if ( arg != NULL && strchr( arg, ':' ) )
		{
			char *temp = NULL;
			int port = atoi( strchr( arg, ':' ) + 1 );
			mlt_properties_set( MLT_CONSUMER_PROPERTIES( this ), "server", arg );
			temp = mlt_properties_get( MLT_CONSUMER_PROPERTIES( this ), "server" );
			*( strchr( temp, ':' ) ) = '\0';
			mlt_properties_set_int( MLT_CONSUMER_PROPERTIES( this ), "port", port );
		}
		else
		{
			mlt_properties_set( MLT_CONSUMER_PROPERTIES( this ), "server", arg == NULL ? "localhost" : arg );
			mlt_properties_set_int( MLT_CONSUMER_PROPERTIES( this ), "port", 5250 );
		}

		mlt_properties_set_int( MLT_CONSUMER_PROPERTIES( this ), "unit", 0 );
		mlt_properties_set( MLT_CONSUMER_PROPERTIES( this ), "command", "append" );

		// Allow thread to be started/stopped
		this->start = consumer_start;
		this->is_stopped = consumer_is_stopped;

		// Return the consumer produced
		return this;
	}

	// malloc or consumer init failed
	free( this );

	// Indicate failure
	return NULL;
}

static int consumer_start( mlt_consumer this )
{
	// Get the producer service
	mlt_service service = mlt_service_producer( MLT_CONSUMER_SERVICE( this ) );

	// Get the properties object
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( this );

	// Get all the properties now
	char *server = mlt_properties_get( properties, "server" );
	int port = mlt_properties_get_int( properties, "port" );
	char *cmd = mlt_properties_get( properties, "command" );
	int unit = mlt_properties_get_int( properties, "unit" );
	char *title = mlt_properties_get( properties, "title" );
	char command[ 2048 ];

	// If this is a reuse, then a valerie object will exist
	valerie connection = mlt_properties_get_data( properties, "connection", NULL );

	// Special case - we can get a doc too...
	char *doc = mlt_properties_get( properties, "westley" );

	// Set the title if provided
	if ( service != NULL )
	{
		if ( title != NULL )
			mlt_properties_set( MLT_SERVICE_PROPERTIES( service ), "title", title );
		else if ( mlt_properties_get( MLT_SERVICE_PROPERTIES( service ), "title" ) == NULL )
			mlt_properties_set( MLT_SERVICE_PROPERTIES( service ), "title", "Anonymous Submission" );
		title = mlt_properties_get( MLT_SERVICE_PROPERTIES( service ), "title" );
	}

	strcpy( command, cmd == NULL ? "" : cmd );
	if ( strstr( command, "title=" ) == NULL && title != NULL )
	{
		strcat( command, " title=\"" );
		strcat( command, title );
		strcat( command, "\"" );
	}

	if ( service != NULL || doc != NULL )
	{
		// Initiate the connection if required
		if ( connection == NULL )
		{
			valerie_parser parser = valerie_parser_init_remote( server, port );
			connection = valerie_init( parser );
			if ( valerie_connect( connection ) == valerie_ok )
			{
				mlt_properties_set_data( properties, "connection", connection, 0, ( mlt_destructor )valerie_close, NULL );
				mlt_properties_set_data( properties, "parser", parser, 0, ( mlt_destructor )valerie_parser_close, NULL );
			}
			else
			{
				fprintf( stderr, "Unable to connect to the server at %s:%d\n", server, port );
				mlt_properties_set_int( properties, "_error", 1 );
				valerie_close( connection );
				valerie_parser_close( parser );
				connection = NULL;
			}
		}

		// If we have connection, push the service over
		if ( connection != NULL )
		{
			if ( doc == NULL )
			{
				int error;

				// Push the service
				error = valerie_unit_push( connection, unit, command, service );

				// Report error
				if ( error != valerie_ok )
					fprintf( stderr, "Push failed on %s:%d %s u%d (%d)\n", server, port, command, unit, error );
			}
			else
			{
				// Push the service
				int error = valerie_unit_receive( connection, unit, command, doc );

				// Report error
				if ( error != valerie_ok )
					fprintf( stderr, "Send failed on %s:%d %s u%d (%d)\n", server, port, command, unit, error );
			}
		}
	}
	
	mlt_consumer_stop( this );
	mlt_consumer_stopped( this );

	return 0;
}

static int consumer_is_stopped( mlt_consumer this )
{
	return 1;
}
