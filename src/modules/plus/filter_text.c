/*
 * filter_text.c -- text overlay filter
 * Copyright (C) 2018 Meltytech, LLC
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

static void setup_producer( mlt_filter filter, mlt_producer producer, mlt_frame frame )
{
	mlt_properties my_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	// Make sure the producer is in the correct position
	mlt_producer_seek( producer, mlt_filter_get_position( filter, frame ) );

	// Pass the properties to the text producer
	mlt_properties_set( producer_properties, "text", mlt_properties_get( my_properties, "argument" ) );
	mlt_properties_set( producer_properties, "family", mlt_properties_get( my_properties, "family" ) );
	mlt_properties_set( producer_properties, "size", mlt_properties_get( my_properties, "size" ) );
	mlt_properties_set( producer_properties, "weight", mlt_properties_get( my_properties, "weight" ) );
	mlt_properties_set( producer_properties, "style", mlt_properties_get( my_properties, "style" ) );
	mlt_properties_set( producer_properties, "fgcolour", mlt_properties_get( my_properties, "fgcolour" ) );
	mlt_properties_set( producer_properties, "bgcolour", mlt_properties_get( my_properties, "bgcolour" ) );
	mlt_properties_set( producer_properties, "olcolour", mlt_properties_get( my_properties, "olcolour" ) );
	mlt_properties_set( producer_properties, "pad", mlt_properties_get( my_properties, "pad" ) );
	mlt_properties_set( producer_properties, "outline", mlt_properties_get( my_properties, "outline" ) );
	mlt_properties_set( producer_properties, "align", mlt_properties_get( my_properties, "halign" ) );
}

static void setup_transition( mlt_filter filter, mlt_transition transition, mlt_frame frame )
{
	mlt_properties my_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties transition_properties = MLT_TRANSITION_PROPERTIES( transition );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );

	mlt_service_lock( MLT_TRANSITION_SERVICE(transition) );
	mlt_rect rect = mlt_properties_anim_get_rect( my_properties, "geometry", position, length );
	mlt_properties_set_rect( transition_properties, "rect", rect );
	mlt_properties_set( transition_properties, "halign", mlt_properties_get( my_properties, "halign" ) );
	mlt_properties_set( transition_properties, "valign", mlt_properties_get( my_properties, "valign" ) );
	mlt_properties_set_int( transition_properties, "out", mlt_properties_get_int( my_properties, "_out" ) );
	mlt_service_unlock( MLT_TRANSITION_SERVICE(transition) );
}


/** Get the image.
*/
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_producer producer = mlt_properties_get_data( properties, "_producer", NULL );
	mlt_transition transition = mlt_properties_get_data( properties, "_transition", NULL );
	mlt_frame b_frame = 0;
	char* argument = mlt_properties_get( properties, "argument" );

	if ( !argument || !strcmp( "", argument ) )
		return mlt_frame_get_image( frame, image, format, width, height, writable );

	// Configure this filter
	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );
	setup_producer( filter, producer, frame );
	setup_transition( filter, transition, frame );

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
		mlt_frame_set_position( b_frame, mlt_filter_get_position( filter, frame ) );
		mlt_properties_set_int( b_props, "consumer_deinterlace", mlt_properties_get_int( a_props, "consumer_deinterlace" ) );

		// Apply all filters that are attached to this filter to the b frame
		mlt_service_apply_filters( MLT_FILTER_SERVICE( filter ), b_frame, 0 );

		// Process the frame
		mlt_transition_process( transition, frame, b_frame );

		// Get the image
		*format = mlt_image_rgb24a;
		error = mlt_frame_get_image( frame, image, format, width, height, writable );

		// Close the temporary frames
		mlt_frame_close( b_frame );
	}
	else
	{
		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
	}

	return error;
}

/** Filter processing.
*/
static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	// Get the properties of the frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Save the frame out point
	mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "_out", mlt_properties_get_int( properties, "out" ) );

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
		mlt_properties_set( MLT_PRODUCER_PROPERTIES( producer ), "eof", "loop" );

		// Assign default values
		mlt_properties_set( my_properties, "argument", arg ? arg: "text" );
		mlt_properties_set( my_properties, "geometry", "0%/0%:100%x100%:100%" );
		mlt_properties_set( my_properties, "family", "Sans" );
		mlt_properties_set( my_properties, "size", "48" );
		mlt_properties_set( my_properties, "weight", "400" );
		mlt_properties_set( my_properties, "style", "normal" );
		mlt_properties_set( my_properties, "fgcolour", "0x000000ff" );
		mlt_properties_set( my_properties, "bgcolour", "0x00000020" );
		mlt_properties_set( my_properties, "olcolour", "0x00000000" );
		mlt_properties_set( my_properties, "pad", "0" );
		mlt_properties_set( my_properties, "halign", "left" );
		mlt_properties_set( my_properties, "valign", "top" );
		mlt_properties_set( my_properties, "outline", "0" );

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
