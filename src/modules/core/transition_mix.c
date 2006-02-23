/*
 * transition_mix.c -- mix two audio streams
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

#include "transition_mix.h"
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>


/** Get the audio.
*/

static int transition_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_audio( frame );

	// Get the effect
	mlt_transition effect = mlt_frame_pop_audio( frame );

	// Get the properties of the b frame
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	if ( mlt_properties_get_int( MLT_TRANSITION_PROPERTIES( effect ), "combine" ) == 0 )
	{
		double mix_start = 0.5, mix_end = 0.5;
		if ( mlt_properties_get( b_props, "audio.previous_mix" ) != NULL )
			mix_start = mlt_properties_get_double( b_props, "audio.previous_mix" );
		if ( mlt_properties_get( b_props, "audio.mix" ) != NULL )
			mix_end = mlt_properties_get_double( b_props, "audio.mix" );
		if ( mlt_properties_get_int( b_props, "audio.reverse" ) )
		{
			mix_start = 1 - mix_start;
			mix_end = 1 - mix_end;
		}

		mlt_frame_mix_audio( frame, b_frame, mix_start, mix_end, buffer, format, frequency, channels, samples );
	}
	else
	{
		mlt_frame_combine_audio( frame, b_frame, buffer, format, frequency, channels, samples );
	}

	return 0;
}


/** Mix transition processing.
*/

static mlt_frame transition_process( mlt_transition this, mlt_frame a_frame, mlt_frame b_frame )
{
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( this );
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	// Only if mix is specified, otherwise a producer may set the mix
	if ( mlt_properties_get( properties, "start" ) != NULL )
	{
		// Determine the time position of this frame in the transition duration
		mlt_properties props = mlt_properties_get_data( MLT_FRAME_PROPERTIES( b_frame ), "_producer", NULL );
		int always_active = mlt_properties_get_int(  MLT_TRANSITION_PROPERTIES( this ), "always_active" );
		mlt_position in = !always_active ? mlt_transition_get_in( this ) : mlt_properties_get_int( props, "in" );
		mlt_position out = !always_active ? mlt_transition_get_out( this ) : mlt_properties_get_int( props, "out" );
		int length = mlt_properties_get_int(  MLT_TRANSITION_PROPERTIES( this ), "length" );
		mlt_position time = !always_active ? mlt_frame_get_position( b_frame ) : mlt_properties_get_int( props, "_frame" );
		double mix = ( double )( time - in ) / ( double )( out - in + 1 );

		// TODO: Check the logic here - shouldn't we be computing current and next mixing levels in all cases?
		if ( length == 0 )
		{
			// If there is an end mix level adjust mix to the range
			if ( mlt_properties_get( properties, "end" ) != NULL )
			{
				double start = mlt_properties_get_double( properties, "start" );
				double end = mlt_properties_get_double( properties, "end" );
				mix = start + ( end - start ) * mix;
			}
			// A negative means total crossfade (uses position)
			else if ( mlt_properties_get_double( properties, "start" ) >= 0 )
			{
				// Otherwise, start/constructor is a constant mix level
		    	mix = mlt_properties_get_double( properties, "start" );
			}
		
			// Finally, set the mix property on the frame
			mlt_properties_set_double( b_props, "audio.mix", mix );
	
			// Initialise transition previous mix value to prevent an inadvertant jump from 0
			if ( mlt_properties_get( properties, "previous_mix" ) == NULL )
				mlt_properties_set_double( properties, "previous_mix", mlt_properties_get_double( b_props, "audio.mix" ) );
				
			// Tell b frame what the previous mix level was
			mlt_properties_set_double( b_props, "audio.previous_mix", mlt_properties_get_double( properties, "previous_mix" ) );

			// Save the current mix level for the next iteration
			mlt_properties_set_double( properties, "previous_mix", mlt_properties_get_double( b_props, "audio.mix" ) );
		
			mlt_properties_set_double( b_props, "audio.reverse", mlt_properties_get_double( properties, "reverse" ) );
		}
		else
		{
			double level = mlt_properties_get_double( properties, "start" );
			double mix_start = level;
			double mix_end = mix_start;
			double mix_increment = 1.0 / length;
			if ( time - in < length )
			{
				mix_start = mix_start * ( ( double )( time - in ) / length );
				mix_end = mix_start + mix_increment;
			}
			else if ( time > out - length )
			{
				mix_end = mix_start * ( ( double )( out - time - in ) / length );
				mix_start = mix_end - mix_increment;
			}

			mix_start = mix_start < 0 ? 0 : mix_start > level ? level : mix_start;
			mix_end = mix_end < 0 ? 0 : mix_end > level ? level : mix_end;
			mlt_properties_set_double( b_props, "audio.previous_mix", mix_start );
			mlt_properties_set_double( b_props, "audio.mix", mix_end );
		}
	}

	// Override the get_audio method
	mlt_frame_push_audio( a_frame, this );
	mlt_frame_push_audio( a_frame, b_frame );
	mlt_frame_push_audio( a_frame, transition_get_audio );
	
	return a_frame;
}

/** Constructor for the transition.
*/

mlt_transition transition_mix_init( char *arg )
{
	mlt_transition this = calloc( sizeof( struct mlt_transition_s ), 1 );
	if ( this != NULL && mlt_transition_init( this, NULL ) == 0 )
	{
		this->process = transition_process;
		if ( arg != NULL )
			mlt_properties_set_double( MLT_TRANSITION_PROPERTIES( this ), "start", atof( arg ) );
		// Inform apps and framework that this is an audio only transition
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( this ), "_transition_type", 2 );
	}
	return this;
}

