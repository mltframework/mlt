/*
 * filter_data_show.c -- data feed filter
 * Copyright (C) 2004-2005 Ushodaya Enterprises Limited
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

#include "filter_data.h"
#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>

/** Handle the profile.
*/

static mlt_filter obtain_filter( mlt_filter filter, char *type )
{
	// Result to return
	mlt_filter result = NULL;

	// Miscelaneous variable
	int i = 0;
	int type_len = strlen( type );

	// Get the properties of the data show filter
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	// Get the profile properties
	mlt_properties profile_properties = mlt_properties_get_data( filter_properties, "profile_properties", NULL );

	// Obtain the profile_properties if we haven't already
	if ( profile_properties == NULL )
	{
		char temp[ 512 ];

		// Get the profile requested
		char *profile = mlt_properties_get( filter_properties, "resource" );

		// If none is specified, pick up the default for this normalisation
		if ( profile == NULL )
			sprintf( temp, "%s/feeds/%s/data_fx.properties", mlt_factory_prefix( ), mlt_environment( "MLT_NORMALISATION" ) );
		else if ( strchr( profile, '%' ) )
			sprintf( temp, "%s/feeds/%s/%s", mlt_factory_prefix( ), mlt_environment( "MLT_NORMALISATION" ), strchr( profile, '%' ) + 1 );
		else
			strcpy( temp, profile );

		// Load the specified profile or use the default
		profile_properties = mlt_properties_load( temp );

		// Store for later retrieval
		mlt_properties_set_data( filter_properties, "profile_properties", profile_properties, 0, ( mlt_destructor )mlt_properties_close, NULL );
	}

	if ( profile_properties != NULL )
	{
		for ( i = 0; i < mlt_properties_count( profile_properties ); i ++ )
		{
			char *name = mlt_properties_get_name( profile_properties, i );
			char *value = mlt_properties_get_value( profile_properties, i );
	
			if ( result == NULL && !strcmp( name, type ) && result == NULL )
				result = mlt_factory_filter( value, NULL );
			else if ( result != NULL && !strncmp( name, type, type_len ) && name[ type_len ] == '.' )
				mlt_properties_set( MLT_FILTER_PROPERTIES( result ), name + type_len + 1, value );
			else if ( result != NULL )
				break;
		}
	}

	return result;
}

/** Process the frame for the requested type
*/

static int process_feed( mlt_properties feed, mlt_filter filter, mlt_frame frame )
{
	// Error return
	int error = 1;

	// Get the properties of the data show filter
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	// Get the type requested by the feeding filter
	char *type = mlt_properties_get( feed, "type" );

	// Fetch the filter associated to this type
	mlt_filter requested = mlt_properties_get_data( filter_properties, type, NULL );

	// If it doesn't exist, then create it now
	if ( requested == NULL )
	{
		// Source filter from profile
		requested = obtain_filter( filter, type );

		// Store it on the properties for subsequent retrieval/destruction
		mlt_properties_set_data( filter_properties, type, requested, 0, ( mlt_destructor )mlt_filter_close, NULL );
	}

	// If we have one, then process it now...
	if ( requested != NULL )
	{
		int i = 0;
		mlt_properties properties = MLT_FILTER_PROPERTIES( requested );
		static char *prefix = "properties.";
		int len = strlen( prefix );

		// Determine if this is an absolute or relative feed
		int absolute = mlt_properties_get_int( feed, "absolute" );

		// Make do with what we have
		int length = !absolute ? 
					 mlt_properties_get_int( feed, "out" ) - mlt_properties_get_int( feed, "in" ) + 1 :
					 mlt_properties_get_int( feed, "out" ) + 1;

		// Repeat period
		int period = mlt_properties_get_int( properties, "period" );
		period = period == 0 ? 1 : period;

		// Pass properties from feed into requested
		for ( i = 0; i < mlt_properties_count( properties ); i ++ )
		{
			char *name = mlt_properties_get_name( properties, i );
			char *key = mlt_properties_get_value( properties, i );
			if ( !strncmp( name, prefix, len ) )
			{
				if ( !strncmp( name + len, "length[", 7 ) )
				{
					mlt_properties_set_position( properties, key, length / period );
				}
				else
				{
					char *value = mlt_properties_get( feed, name + len );
					if ( value != NULL )
						mlt_properties_set( properties, key, value );
				}
			}
		}

		// Set the original position on the frame
		if ( absolute == 0 )
			mlt_frame_set_position( frame, mlt_properties_get_int( feed, "position" ) - mlt_properties_get_int( feed, "in" ) );
		else
			mlt_frame_set_position( frame, mlt_properties_get_int( feed, "position" ) );

		// Process the filter
		mlt_filter_process( requested, frame );

		// Should be ok...
		error = 0;
	}

	return error;
}

/** Get the image.
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Pop the service
	mlt_filter filter = mlt_frame_pop_service( frame );

	// Get the frame properties
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Fetch the data queue
	mlt_deque data_queue = mlt_properties_get_data( frame_properties, "data_queue", NULL );

	// Create a new queue for those that we can't handle
	mlt_deque temp_queue = mlt_deque_init( );

	// Iterate through each entry on the queue
	while ( data_queue != NULL && mlt_deque_peek_front( data_queue ) != NULL )
	{
		// Get the data feed
		mlt_properties feed = mlt_deque_pop_front( data_queue );

		// Process the data feed...
		if ( process_feed( feed, filter, frame ) == 0 )
			mlt_properties_close( feed );
		else
			mlt_deque_push_back( temp_queue, feed );
	}

	// Now put the unprocessed feeds back on the stack
	while ( data_queue != NULL && mlt_deque_peek_front( temp_queue ) )
	{
		// Get the data feed
		mlt_properties feed = mlt_deque_pop_front( temp_queue );

		// Put it back on the data queue
		mlt_deque_push_back( data_queue, feed );
	}

	// Close the temporary queue
	mlt_deque_close( temp_queue );

	// Need to get the image
	return mlt_frame_get_image( frame, image, format, width, height, 1 );
}


/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Push the filter
	mlt_frame_push_service( frame, this );

	// Register the get image method
	mlt_frame_push_get_image( frame, filter_get_image );

	// Return the frame
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_data_show_init( char *arg )
{
	// Create the filter
	mlt_filter this = mlt_filter_new( );

	// Initialise it
	if ( this != NULL )
	{
		// Get the properties
		mlt_properties properties = MLT_FILTER_PROPERTIES( this );

		// Assign the argument (default to titles)
		mlt_properties_set( properties, "resource", arg == NULL ? NULL : arg );

		// Specify the processing method
		this->process = filter_process;
	}

	return this;
}

