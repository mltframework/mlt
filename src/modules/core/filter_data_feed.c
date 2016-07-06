/*
 * filter_data_feed.c -- data feed filter
 * Copyright (C) 2004-2014 Meltytech, LLC
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

#include <stdlib.h>
#include <string.h>
#include <framework/mlt.h>

/** This filter should be used in conjuction with the data_show filter.
	The concept of the data_feed is that it can be used to pass titles
	or images to render on the frame, but doesn't actually do it 
	itself. data_feed imposes few rules on what's passed on and the 
	validity is confirmed in data_show before use.
*/

/** Data queue destructor.
*/

static void destroy_data_queue( void *arg )
{
	if ( arg != NULL )
	{
		// Assign the correct type
		mlt_deque queue = arg;

		// Iterate through each item and destroy them
		while ( mlt_deque_peek_front( queue ) != NULL )
			mlt_properties_close( mlt_deque_pop_back( queue ) );

		// Close the deque
		mlt_deque_close( queue );
	}
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	// Get the filter properties
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	// Get the frame properties
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Get the data queue
	mlt_deque data_queue = mlt_properties_get_data( frame_properties, "data_queue", NULL );

	// Get the type of the data feed
	char *type = mlt_properties_get( filter_properties, "type" );

	// Get the in and out points of this filter
	int in = mlt_filter_get_in( filter );
	int out = mlt_filter_get_out( filter );

	// Create the data queue if it doesn't exist
	if ( data_queue == NULL )
	{
		// Create the queue
		data_queue = mlt_deque_init( );

		// Assign it to the frame with the destructor
		mlt_properties_set_data( frame_properties, "data_queue", data_queue, 0, destroy_data_queue, NULL );
	}

	// Now create the data feed
	if ( data_queue != NULL && type != NULL && !strcmp( type, "attr_check" ) )
	{
		int i = 0;
		int count = mlt_properties_count( frame_properties );

		for ( i = 0; i < count; i ++ )
		{
			char *name = mlt_properties_get_name( frame_properties, i );

			// Only deal with meta.attr.name values here - these should have a value of 1 to be considered
			// Additional properties of the form are meta.attr.name.property are passed down on the feed
			if ( !strncmp( name, "meta.attr.", 10 ) && strchr( name + 10, '.' ) == NULL && mlt_properties_get_int( frame_properties, name ) == 1 )
			{
				// Temp var to hold name + '.' for pass method
				char temp[ 132 ];

				// Create a new data feed
				mlt_properties feed = mlt_properties_new( );

				// Assign it the base properties
				mlt_properties_set( feed, "id", mlt_properties_get( filter_properties, "_unique_id" ) );
				mlt_properties_set( feed, "type", strrchr( name, '.' ) + 1 );
				mlt_properties_set_position( feed, "position", mlt_frame_get_position( frame ) );

				// Assign in/out of service we're connected to
				mlt_properties_set_position( feed, "in", mlt_properties_get_position( frame_properties, "in" ) );
				mlt_properties_set_position( feed, "out", mlt_properties_get_position( frame_properties, "out" ) );

				// Pass all meta properties 
				sprintf( temp, "%s.", name );
				mlt_properties_pass( feed, frame_properties, temp );

				// Push it on to the queue
				mlt_deque_push_back( data_queue, feed );

				// Make sure this attribute only gets processed once
				mlt_properties_set_int( frame_properties, name, 0 );
			}
		}
	}
	else if ( data_queue != NULL )
	{
		// Create a new data feed
		mlt_properties feed = mlt_properties_new( );

		// Assign it the base properties
		mlt_properties_set( feed, "id", mlt_properties_get( filter_properties, "_unique_id" ) );
		mlt_properties_set( feed, "type", type );
		mlt_properties_set_position( feed, "position", mlt_frame_get_position( frame ) );

		// Assign in/out of service we're connected to
		mlt_properties_set_position( feed, "in", mlt_properties_get_position( frame_properties, "in" ) );
		mlt_properties_set_position( feed, "out", mlt_properties_get_position( frame_properties, "out" ) );

		// Correct in/out to the filter if specified
		if ( in != 0 )
			mlt_properties_set_position( feed, "in", in );
		if ( out != 0 )
			mlt_properties_set_position( feed, "out", out );

		// Pass the properties which start with a "feed." prefix 
		// Note that 'feed.text' in the filter properties becomes 'text' on the feed
		mlt_properties_pass( feed, filter_properties, "feed." );

		// Push it on to the queue
		mlt_deque_push_back( data_queue, feed );
	}

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_data_feed_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Create the filter
	mlt_filter filter = mlt_filter_new( );

	// Initialise it
	if ( filter != NULL )
	{
		// Get the properties
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

		// Assign the argument (default to titles)
		mlt_properties_set( properties, "type", arg == NULL ? "titles" : arg );

		// Specify the processing method
		filter->process = filter_process;
	}

	return filter;
}

