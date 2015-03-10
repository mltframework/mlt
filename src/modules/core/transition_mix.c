/*
 * transition_mix.c -- mix two audio streams
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include <framework/mlt_transition.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


static int mix_audio( mlt_frame frame, mlt_frame that, float weight_start, float weight_end, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	int ret = 0;
	int16_t *src, *dest;
	int frequency_src = *frequency, frequency_dest = *frequency;
	int channels_src = *channels, channels_dest = *channels;
	int samples_src = *samples, samples_dest = *samples;
	int i, j;
	double d = 0, s = 0;

	mlt_frame_get_audio( that, (void**) &src, format, &frequency_src, &channels_src, &samples_src );
	mlt_frame_get_audio( frame, (void**) &dest, format, &frequency_dest, &channels_dest, &samples_dest );

	int silent = mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "silent_audio" );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "silent_audio", 0 );
	if ( silent )
		memset( dest, 0, samples_dest * channels_dest * sizeof( int16_t ) );

	silent = mlt_properties_get_int( MLT_FRAME_PROPERTIES( that ), "silent_audio" );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( that ), "silent_audio", 0 );
	if ( silent )
		memset( src, 0, samples_src * channels_src * sizeof( int16_t ) );

	// determine number of samples to process
	*samples = samples_src < samples_dest ? samples_src : samples_dest;
	*channels = channels_src < channels_dest ? channels_src : channels_dest;
	*buffer = dest;
	*frequency = frequency_dest;

	// Compute a smooth ramp over start to end
	float weight = weight_start;
	float weight_step = ( weight_end - weight_start ) / *samples;

	if ( src == dest )
	{
		*samples = samples_src;
		*channels = channels_src;
		*buffer = src;
		*frequency = frequency_src;
		return ret;
	}

	// Mixdown
	for ( i = 0; i < *samples; i++ )
	{
		for ( j = 0; j < *channels; j++ )
		{
			if ( j < channels_dest )
				d = (double) dest[ i * channels_dest + j ];
			if ( j < channels_src )
				s = (double) src[ i * channels_src + j ];
			dest[ i * channels_dest + j ] = s * weight + d * ( 1.0 - weight );
		}
		weight += weight_step;
	}

	return ret;
}

// Replacement for broken mlt_frame_audio_mix - this filter uses an inline low pass filter
// to allow mixing without volume hacking
static int combine_audio( mlt_frame frame, mlt_frame that, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	int ret = 0;
	int16_t *src, *dest;
	int frequency_src = *frequency, frequency_dest = *frequency;
	int channels_src = *channels, channels_dest = *channels;
	int samples_src = *samples, samples_dest = *samples;
	int i, j;
	double vp[ 6 ];
	double b_weight = 1.0;

	if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "meta.mixdown" ) )
		b_weight = 1.0 - mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "meta.volume" );

	mlt_frame_get_audio( that, (void**) &src, format, &frequency_src, &channels_src, &samples_src );
	mlt_frame_get_audio( frame, (void**) &dest, format, &frequency_dest, &channels_dest, &samples_dest );

	int silent = mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "silent_audio" );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "silent_audio", 0 );
	if ( silent )
		memset( dest, 0, samples_dest * channels_dest * sizeof( int16_t ) );

	silent = mlt_properties_get_int( MLT_FRAME_PROPERTIES( that ), "silent_audio" );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( that ), "silent_audio", 0 );
	if ( silent )
		memset( src, 0, samples_src * channels_src * sizeof( int16_t ) );

	if ( src == dest )
	{
		*samples = samples_src;
		*channels = channels_src;
		*buffer = src;
		*frequency = frequency_src;
		return ret;
	}

	// determine number of samples to process
	*samples = samples_src < samples_dest ? samples_src : samples_dest;
	*channels = channels_src < channels_dest ? channels_src : channels_dest;
	*buffer = dest;
	*frequency = frequency_dest;

	for ( j = 0; j < *channels; j++ )
		vp[ j ] = ( double )dest[ j ];

	double Fc = 0.5;
	double B = exp(-2.0 * M_PI * Fc);
	double A = 1.0 - B;
	double v;

	for ( i = 0; i < *samples; i++ )
	{
		for ( j = 0; j < *channels; j++ )
		{
			v = ( double )( b_weight * dest[ i * channels_dest + j ] + src[ i * channels_src + j ] );
			v = v < -32767 ? -32767 : v > 32768 ? 32768 : v;
			vp[ j ] = dest[ i * channels_dest + j ] = ( int16_t )( v * A + vp[ j ] * B );
		}
	}

	return ret;
}

/** Get the audio.
*/

static int transition_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_audio( frame );

	// Get the effect
	mlt_transition effect = mlt_frame_pop_audio( frame );

	// Get the properties of the b frame
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	// We can only mix s16
	*format = mlt_audio_s16;

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

		mix_audio( frame, b_frame, mix_start, mix_end, buffer, format, frequency, channels, samples );
	}
	else
	{
		combine_audio( frame, b_frame, buffer, format, frequency, channels, samples );
	}

	return 0;
}


/** Mix transition processing.
*/

static mlt_frame transition_process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	// Only if mix is specified, otherwise a producer may set the mix
	if ( mlt_properties_get( properties, "start" ) != NULL )
	{
		// Determine the time position of this frame in the transition duration
		mlt_properties props = mlt_properties_get_data( MLT_FRAME_PROPERTIES( b_frame ), "_producer", NULL );
		mlt_position in = mlt_properties_get_int( props, "in" );
		mlt_position out = mlt_properties_get_int( props, "out" );
		int length = mlt_properties_get_int( properties, "length" );
		mlt_position time = mlt_properties_get_int( props, "_frame" );
		double mix = mlt_transition_get_progress( transition, b_frame );
		if ( mlt_properties_get_int(  properties, "always_active" ) )
			mix = ( double ) ( time - in ) / ( double ) ( out - in + 1 );

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
			mlt_position last_position = mlt_properties_get_position( properties, "_last_position" );
			mlt_position current_position = mlt_frame_get_position( b_frame );
			mlt_properties_set_position( properties, "_last_position", current_position );
			if ( mlt_properties_get( properties, "_previous_mix" ) == NULL
			     || current_position != last_position + 1 )
				mlt_properties_set_double( properties, "_previous_mix", mix );
				
			// Tell b frame what the previous mix level was
			mlt_properties_set_double( b_props, "audio.previous_mix", mlt_properties_get_double( properties, "_previous_mix" ) );

			// Save the current mix level for the next iteration
			mlt_properties_set_double( properties, "_previous_mix", mlt_properties_get_double( b_props, "audio.mix" ) );
		
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
	mlt_frame_push_audio( a_frame, transition );
	mlt_frame_push_audio( a_frame, b_frame );
	mlt_frame_push_audio( a_frame, transition_get_audio );
	
	return a_frame;
}

/** Constructor for the transition.
*/

mlt_transition transition_mix_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_transition transition = calloc( 1, sizeof( struct mlt_transition_s ) );
	if ( transition != NULL && mlt_transition_init( transition, NULL ) == 0 )
	{
		transition->process = transition_process;
		if ( arg != NULL )
			mlt_properties_set_double( MLT_TRANSITION_PROPERTIES( transition ), "start", atof( arg ) );
		// Inform apps and framework that this is an audio only transition
		mlt_properties_set_int( MLT_TRANSITION_PROPERTIES( transition ), "_transition_type", 2 );
	}
	return transition;
}

