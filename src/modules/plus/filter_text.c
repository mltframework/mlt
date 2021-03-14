/*
 * filter_text.c -- text overlay filter
 * Copyright (C) 2018-2021 Meltytech, LLC
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

#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>

static void property_changed( mlt_service owner, mlt_filter filter, mlt_event_data event_data )
{
	const char *name = mlt_event_data_to_string(event_data);
	if (!name) return;
	if( !strcmp( "geometry", name ) ||
		!strcmp( "family", name ) ||
		!strcmp( "size", name ) ||
		!strcmp( "weight", name ) ||
		!strcmp( "style", name ) ||
		!strcmp( "fgcolour", name ) ||
		!strcmp( "bgcolour", name ) ||
		!strcmp( "olcolour", name ) ||
		!strcmp( "pad", name ) ||
		!strcmp( "halign", name ) ||
		!strcmp( "valign", name ) ||
		!strcmp( "outline", name ) )
	{
		mlt_properties_set_int( MLT_FILTER_PROPERTIES(filter), "_reset", 1 );
	}
}

static void setup_producer( mlt_producer producer, mlt_properties my_properties )
{
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	// Pass the properties to the text producer
	mlt_properties_set_string( producer_properties, "family", mlt_properties_get( my_properties, "family" ) );
	mlt_properties_set_string( producer_properties, "size", mlt_properties_get( my_properties, "size" ) );
	mlt_properties_set_string( producer_properties, "weight", mlt_properties_get( my_properties, "weight" ) );
	mlt_properties_set_string( producer_properties, "style", mlt_properties_get( my_properties, "style" ) );
	mlt_properties_set_string( producer_properties, "fgcolour", mlt_properties_get( my_properties, "fgcolour" ) );
	mlt_properties_set_string( producer_properties, "bgcolour", mlt_properties_get( my_properties, "bgcolour" ) );
	mlt_properties_set_string( producer_properties, "olcolour", mlt_properties_get( my_properties, "olcolour" ) );
	mlt_properties_set_string( producer_properties, "pad", mlt_properties_get( my_properties, "pad" ) );
	mlt_properties_set_string( producer_properties, "outline", mlt_properties_get( my_properties, "outline" ) );
	mlt_properties_set_string( producer_properties, "align", mlt_properties_get( my_properties, "halign" ) );
}

static void setup_transition( mlt_filter filter, mlt_transition transition, mlt_frame frame, mlt_properties my_properties )
{
	mlt_properties transition_properties = MLT_TRANSITION_PROPERTIES( transition );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );

	mlt_service_lock( MLT_TRANSITION_SERVICE(transition) );
	mlt_rect rect = mlt_properties_anim_get_rect( my_properties, "geometry", position, length );
	if (mlt_properties_get(my_properties, "geometry") && strchr(mlt_properties_get(my_properties, "geometry"), '%')) {
		mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
		rect.x *= profile->width;
		rect.y *= profile->height;
		rect.w *= profile->width;
		rect.h *= profile->height;
	}
	mlt_properties_set_rect( transition_properties, "rect", rect );
	mlt_properties_set_string( transition_properties, "halign", mlt_properties_get( my_properties, "halign" ) );
	mlt_properties_set_string( transition_properties, "valign", mlt_properties_get( my_properties, "valign" ) );
	mlt_service_unlock( MLT_TRANSITION_SERVICE(transition) );
}

static mlt_properties get_filter_properties( mlt_filter filter, mlt_frame frame )
{
	mlt_properties properties = mlt_frame_get_unique_properties( frame, MLT_FILTER_SERVICE(filter) );
	if ( !properties )
		properties = MLT_FILTER_PROPERTIES(filter);
	return properties;
}

/** Get the image.
*/
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = mlt_frame_pop_service( frame );
	char* argument = (char*)mlt_frame_pop_service( frame );
	mlt_properties my_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties properties = get_filter_properties( filter, frame );
	mlt_producer producer = mlt_properties_get_data( my_properties, "_producer", NULL );
	mlt_transition transition = mlt_properties_get_data( my_properties, "_transition", NULL );
	mlt_frame b_frame = 0;
	mlt_position position = 0;

	// Configure this filter
	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );
	if( mlt_properties_get_int( my_properties, "_reset" ) )
	{
		setup_producer( producer, properties );
		setup_transition( filter, transition, frame, properties );
	}
	mlt_properties_set_string( MLT_PRODUCER_PROPERTIES( producer ), "text", argument );

	// Make sure the producer is in the correct position
	position = mlt_filter_get_position( filter, frame );
	mlt_producer_seek( producer, position );

	// Get the b frame and process with transition if successful
	if ( mlt_service_get_frame( MLT_PRODUCER_SERVICE( producer ), &b_frame, 0 ) == 0 )
	{
		// This lock needs to also protect the producer properties from being
		// modified in setup_producer() while also being used in mlt_service_get_frame().
		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

		// Get the a and b frame properties
		mlt_properties a_props = MLT_FRAME_PROPERTIES( frame );
		mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

		// Set the b_frame to be in the same position and have same consumer requirements
		mlt_frame_set_position( b_frame, position );
		mlt_properties_set_int( b_props, "consumer_deinterlace", mlt_properties_get_int( a_props, "consumer_deinterlace" ) );
		mlt_properties_set_double( b_props, "consumer_scale", mlt_properties_get_double( a_props, "consumer_scale" ) );

		// Apply all filters that are attached to this filter to the b frame
		mlt_service_apply_filters( MLT_FILTER_SERVICE( filter ), b_frame, 0 );

		// Process the frame
		mlt_transition_process( transition, frame, b_frame );

		// Get the image
		error = mlt_frame_get_image( frame, image, format, width, height, writable );

		// Close the temporary frames
		mlt_frame_close( b_frame );
	}
	else
	{
		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
	}
	free( argument );

	return error;
}

/** Filter processing.
*/
static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_properties properties = get_filter_properties( filter, frame );
	char* argument = mlt_properties_get( properties, "argument" );
	if ( !argument || !strcmp( "", argument ) )
		return frame;

	// Save the text to be used by get_image() to support parallel processing
	// when this filter is encapsulated by other filters.
	mlt_frame_push_service( frame, strdup( argument ) );

	// Push the filter on to the stack
	mlt_frame_push_service( frame, filter );

	// Push the get_image on to the stack
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/
mlt_filter filter_text_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	mlt_transition transition = mlt_factory_transition( profile, "affine", NULL );
	mlt_producer producer = mlt_factory_producer( profile, mlt_environment( "MLT_PRODUCER" ), "qtext:" );

	// Use pango if qtext is not available.
	if( !producer )
		producer = mlt_factory_producer( profile, mlt_environment( "MLT_PRODUCER" ), "pango:" );

	if( !producer )
		mlt_log_warning( MLT_FILTER_SERVICE(filter), "QT or GTK modules required for text.\n" );

	if ( filter && transition && producer )
	{
		mlt_properties my_properties = MLT_FILTER_PROPERTIES( filter );

		// Register the transition for reuse/destruction
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES(transition), "fill", 0 );
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES(transition), "b_scaled", 1 );
		mlt_properties_set_data( my_properties, "_transition", transition, 0, ( mlt_destructor )mlt_transition_close, NULL );

		// Register the producer for reuse/destruction
		mlt_properties_set_data( my_properties, "_producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

		// Ensure that we loop
		mlt_properties_set_string( MLT_PRODUCER_PROPERTIES( producer ), "eof", "loop" );

		// Listen for property changes.
		mlt_events_listen( MLT_FILTER_PROPERTIES(filter), filter, "property-changed", (mlt_listener)property_changed );

		// Assign default values
		mlt_properties_set_string( my_properties, "argument", arg ? arg: "text" );
		mlt_properties_set_string( my_properties, "geometry", "0%/0%:100%x100%:100%" );
		mlt_properties_set_string( my_properties, "family", "Sans" );
		mlt_properties_set_string( my_properties, "size", "48" );
		mlt_properties_set_string( my_properties, "weight", "400" );
		mlt_properties_set_string( my_properties, "style", "normal" );
		mlt_properties_set_string( my_properties, "fgcolour", "0x000000ff" );
		mlt_properties_set_string( my_properties, "bgcolour", "0x00000020" );
		mlt_properties_set_string( my_properties, "olcolour", "0x00000000" );
		mlt_properties_set_string( my_properties, "pad", "0" );
		mlt_properties_set_string( my_properties, "halign", "left" );
		mlt_properties_set_string( my_properties, "valign", "top" );
		mlt_properties_set_string( my_properties, "outline", "0" );
		mlt_properties_set_int( my_properties, "_reset", 1 );
		mlt_properties_set_int( my_properties, "_filter_private", 1 );

		filter->process = filter_process;
	}
	else
	{
		if( filter )
		{
			mlt_filter_close( filter );
		}

		if( transition )
		{
			mlt_transition_close( transition );
		}

		if( producer )
		{
			mlt_producer_close( producer );
		}

		filter = NULL;
	}
	return filter;
}
