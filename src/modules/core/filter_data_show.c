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
	mlt_properties filter_properties = mlt_filter_properties( filter );

	// Get the profile properties
	mlt_properties profile_properties = mlt_properties_get_data( filter_properties, "profile_properties", NULL );

	// Obtain the profile_properties if we haven't already
	if ( profile_properties == NULL )
	{
		// Get the profile requested
		char *profile = mlt_properties_get( filter_properties, "profile" );

		// Load the specified profile or use the default
		if ( profile != NULL )
		{
			profile_properties = mlt_properties_load( profile );
		}
		else
		{
			// Sometimes C can be laborious.. 
			static char *default_file = "/data_fx.properties";
			char *temp = malloc( strlen( mlt_factory_prefix( ) ) + strlen( default_file ) + 1 );
			if ( temp != NULL )
			{
				strcpy( temp, mlt_factory_prefix( ) );
				strcat( temp, default_file );
				profile_properties = mlt_properties_load( temp );
				free( temp );
			}
		}

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
				mlt_properties_set( mlt_filter_properties( result ), name + type_len + 1, value );
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
	mlt_properties filter_properties = mlt_filter_properties( filter );

	// Get the type requested by the feeding filter
	char *type = mlt_properties_get( feed, "type" );

	// Fetch the filter associated to this type
	mlt_filter requested = mlt_properties_get_data( filter_properties, type, NULL );

	// Calculate the length of the feed
	int length = mlt_properties_get_int( feed, "out" ) - mlt_properties_get_int( feed, "in" ) + 1;

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
		mlt_properties properties = mlt_filter_properties( requested );
		static char *prefix = "properties.";
		int len = strlen( prefix );

		// Pass properties from feed into requested
		for ( i = 0; i < mlt_properties_count( properties ); i ++ )
		{
			char *name = mlt_properties_get_name( properties, i );
			char *key = mlt_properties_get_value( properties, i );
			if ( !strncmp( name, prefix, len ) )
			{
				if ( !strncmp( name + len, "length[", 7 ) )
				{
					int period = mlt_properties_get_int( properties, "period" );
					period = period == 0 ? 1 : period;
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
		mlt_frame_set_position( frame, mlt_properties_get_int( feed, "position" ) - mlt_properties_get_int( feed, "in" ) );

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
	mlt_properties frame_properties = mlt_frame_properties( frame );

	// Fetch the data queue
	mlt_deque data_queue = mlt_properties_get_data( frame_properties, "data_queue", NULL );

	// Iterate through each entry on the queue
	while ( data_queue != NULL && mlt_deque_peek_front( data_queue ) != NULL )
	{
		// Get the data feed
		mlt_properties feed = mlt_deque_pop_front( data_queue );

		// Process the data feed...
		process_feed( feed, filter, frame );

		// Close the feed
		mlt_properties_close( feed );
	}

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
		mlt_properties properties = mlt_filter_properties( this );

		// Assign the argument (default to titles)
		mlt_properties_set( properties, "profile", arg == NULL ? NULL : arg );

		// Specify the processing method
		this->process = filter_process;
	}

	return this;
}

