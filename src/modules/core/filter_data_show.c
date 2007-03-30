/*
 * filter_data_show.c -- data feed filter
 * Copyright (C) 2004-2005 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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

/** Retrieve medatata value 
*/

const char* metadata_value(mlt_properties properties, char* name)
{
	if (name == NULL) return "-";
	char *meta = malloc( strlen(name) + 18 );
	sprintf( meta, "meta.attr.%s.markup", name);
	return mlt_properties_get( properties, meta);
}

/** Convert frames to Timecode 
*/

const char* frame_to_timecode( int frames , int fps)
{
	char *res = malloc(12);
	if (fps == 0) return "-";
	int seconds = frames / (int) fps;
	frames = frames % ((int) fps);
	int minutes = seconds / 60;
	seconds = seconds % 60;
	int hours = minutes / 60;
	minutes = minutes % 60;
	sprintf(res, "%.2d:%.2d:%.2d:%.2d", hours, minutes, seconds, frames);
	return res;
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
					mlt_properties_set_position( properties, key, ( length - period ) / period );
				}
				else
				{
					char *value = mlt_properties_get( feed, name + len );
					if ( value != NULL ) {
						// check for metadata keywords in metadata markup if user requested so
						if ( mlt_properties_get_int( filter_properties, "dynamic" ) == 1  && !strcmp( name + strlen( name ) - 6, "markup") )
						{
							// Find keywords which should be surrounded by '#', like: #title#
							char* keywords = strtok ( value, "\\#" );
							char result[] = "";
							int ct = 0;
							int fromStart = 0;
							if ( value[0] == '\\' ) fromStart = 1;
							
							while (keywords != NULL) {
								if (ct % 2 == fromStart) 
									strcat( result, keywords);
								else if (!strcmp(keywords, "timecode")) {
									// special case: replace #tc# with current frame timecode
									int pos = mlt_properties_get_int( feed, "position" );
									const char *tc = frame_to_timecode(pos, mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "fps" ));
									strcat( result, tc);
								}
								else {
									// replace keyword with metadata value
									const char * metavalue = metadata_value(MLT_FRAME_PROPERTIES( frame ), keywords);
									if (metavalue == NULL) metavalue = "-";
									strcat( result, metavalue);
								}
								keywords = strtok ( NULL, "\\#" );
								ct++;
							}
							mlt_properties_set( properties, key, (char*) result );
						}
						else mlt_properties_set( properties, key, value );
					}
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

void process_queue( mlt_deque data_queue, mlt_frame frame, mlt_filter filter )
{
	if ( data_queue != NULL )
	{
		// Create a new queue for those that we can't handle
		mlt_deque temp_queue = mlt_deque_init( );

		// Iterate through each entry on the queue
		while ( mlt_deque_peek_front( data_queue ) != NULL )
		{
			// Get the data feed
			mlt_properties feed = mlt_deque_pop_front( data_queue );

			if ( mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "debug" ) != NULL )
				mlt_properties_debug( feed, mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "debug" ), stderr );

			// Process the data feed...
			if ( process_feed( feed, filter, frame ) == 0 )
				mlt_properties_close( feed );
			else
				mlt_deque_push_back( temp_queue, feed );
		}
	
		// Now put the unprocessed feeds back on the stack
		while ( mlt_deque_peek_front( temp_queue ) )
		{
			// Get the data feed
			mlt_properties feed = mlt_deque_pop_front( temp_queue );
	
			// Put it back on the data queue
			mlt_deque_push_back( data_queue, feed );
		}
	
		// Close the temporary queue
		mlt_deque_close( temp_queue );
	}
}

/** Get the image.
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Pop the service
	mlt_filter filter = mlt_frame_pop_service( frame );

	// Get the frame properties
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Track specific
	process_queue( mlt_properties_get_data( frame_properties, "data_queue", NULL ), frame, filter );

	// Global
	process_queue( mlt_properties_get_data( frame_properties, "global_queue", NULL ), frame, filter );

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

