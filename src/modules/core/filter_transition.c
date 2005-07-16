/*
 * filter_transition.c -- Convert any transition into a filter
 * Copyright (C) 2005 Ushodaya Enterprises Limited
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

#include "filter_transition.h"
#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_transition.h>

/** Get the image via the transition.
	NB: Not all transitions will accept a and b frames being the same...
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_transition transition = mlt_frame_pop_service( this );
	if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "image_count" ) >= 1 )
		mlt_transition_process( transition, this, this );
	return mlt_frame_get_image( this, image, format, width, height, writable );
}

/** Get the audio via the transition.
	NB: Not all transitions will accept a and b frames being the same...
*/

static int filter_get_audio( mlt_frame this, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Obtain the transition instance
	mlt_transition transition = mlt_frame_pop_audio( this );
	mlt_transition_process( transition, this, this );
	return mlt_frame_get_audio( this, buffer, format, frequency, channels, samples );
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Obtain the transition instance
	mlt_transition transition = mlt_properties_get_data( MLT_FILTER_PROPERTIES( this ), "instance", NULL );

	// If we haven't created the instance, do it now
	if ( transition == NULL )
	{
		char *name = mlt_properties_get( MLT_FILTER_PROPERTIES( this ), "transition" );
		transition = mlt_factory_transition( name, NULL );
		mlt_properties_set_data( MLT_FILTER_PROPERTIES( this ), "instance", transition, 0, ( mlt_destructor )mlt_transition_close, NULL );
	}

	// We may still not have a transition...
	if ( transition != NULL )
	{
		// Get the transition type
		int type = mlt_properties_get_int( MLT_TRANSITION_PROPERTIES( transition ), "_transition_type" );

		// Set the basic info
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "in", mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "in" ) );
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "out", mlt_properties_get_int( MLT_FILTER_PROPERTIES( this ), "out" ) );

		// Refresh with current user values
		mlt_properties_pass( MLT_TRANSITION_PROPERTIES( transition ), MLT_FILTER_PROPERTIES( this ), "transition." );

		if ( type & 1 && !mlt_frame_is_test_card( frame ) && !( mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "hide" ) & 1 ) )
		{
			mlt_frame_push_service( frame, transition );
			mlt_frame_push_get_image( frame, filter_get_image );
		}
		if ( type & 2 && !mlt_frame_is_test_audio( frame ) && !( mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "hide" ) & 2 ) )
		{
			mlt_frame_push_audio( frame, transition );
			mlt_frame_push_audio( frame, filter_get_audio );
		}

		if ( type == 0 )
			mlt_properties_debug( MLT_TRANSITION_PROPERTIES( transition ), "unknown transition type", stderr );
	}
	else
	{
		mlt_properties_debug( MLT_FILTER_PROPERTIES( this ), "no transition", stderr );
	}

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_transition_init( char *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "transition", arg );
		this->process = filter_process;
	}
	return this;
}

