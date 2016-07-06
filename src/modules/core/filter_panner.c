/*
 * filter_panner.c --pan/balance audio channels
 * Copyright (C) 2010-2014 Meltytech, LLC
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


/** Get the audio.
*/

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_properties properties = mlt_frame_pop_audio( frame );
	mlt_filter filter = mlt_frame_pop_audio( frame );
	mlt_properties filter_props = MLT_FILTER_PROPERTIES( filter );
	mlt_properties frame_props = MLT_FRAME_PROPERTIES( frame );

	// We can only mix s16
	*format = mlt_audio_s16;
	mlt_frame_get_audio( frame, (void**) buffer, format, frequency, channels, samples );

	// Apply silence
	int silent = mlt_properties_get_int( frame_props, "silent_audio" );
	mlt_properties_set_int( frame_props, "silent_audio", 0 );
	if ( silent )
		memset( *buffer, 0, *samples * *channels * sizeof( int16_t ) );

	int src_size = 0;
	int16_t *src = mlt_properties_get_data( filter_props, "scratch_buffer", &src_size );
	int16_t *dest = *buffer;
	double v; // sample accumulator
	int i, out, in;
	double factors[6][6]; // mixing weights [in][out]
	double mix_start = 0.5, mix_end = 0.5;
	if ( mlt_properties_get( properties, "previous_mix" ) != NULL )
		mix_start = mlt_properties_get_double( properties, "previous_mix" );
	if ( mlt_properties_get( properties, "mix" ) != NULL )
		mix_end = mlt_properties_get_double( properties, "mix" );
	double weight = mix_start;
	double weight_step = ( mix_end - mix_start ) / *samples;
	int active_channel = mlt_properties_get_int( properties, "channel" );
	int gang = mlt_properties_get_int( properties, "gang" ) ? 2 : 1;
	// Use an inline low-pass filter to help avoid clipping
	double Fc = 0.5;
	double B = exp(-2.0 * M_PI * Fc);
	double A = 1.0 - B;
	double vp[6];

	// Setup or resize a scratch buffer
	if ( !src || src_size < *samples * *channels * sizeof(int16_t) )
	{
		// We allocate 4 more samples than we need to deal with jitter in the sample count per frame.
		src_size = ( *samples + 4 ) * *channels * sizeof(int16_t);
		src = mlt_pool_alloc( src_size );
		if ( !src )
			return 0;
		mlt_properties_set_data( filter_props, "scratch_buffer", src, src_size, mlt_pool_release, NULL );
	}

	// We must use a pristine copy as the source
	memcpy( src, *buffer, *samples * *channels * sizeof(int16_t) );

	// Initialize the mix factors
	for ( i = 0; i < 6; i++ )
		for ( out = 0; out < 6; out++ )
			factors[i][out] = 0.0;

	for ( out = 0; out < *channels; out++ )
		vp[out] = (double) dest[out];

	for ( i = 0; i < *samples; i++ )
	{
		// Recompute the mix factors
		switch ( active_channel )
		{
			case -1: // Front L/R balance
			case -2: // Rear L/R balance
			{
				// Gang front/rear balance if requested
				int g, active = active_channel;
				for ( g = 0; g < gang; g++, active-- )
				{
					int left = active == -1 ? 0 : 2;
					int right = left + 1;
					if ( weight < 0.0 )
					{
						factors[left][left] = 1.0;
						factors[right][right] = weight + 1.0 < 0.0 ? 0.0 : weight + 1.0;
					}
					else
					{
						factors[left][left] = 1.0 - weight < 0.0 ? 0.0 : 1.0 - weight;
						factors[right][right] = 1.0;
					}
				}
				break;
			}
			case -3: // Left fade
			case -4: // right fade
			{
				// Gang left/right fade if requested
				int g, active = active_channel;
				for ( g = 0; g < gang; g++, active-- )
				{
					int front = active == -3 ? 0 : 1;
					int rear = front + 2;
					if ( weight < 0.0 )
					{
						factors[front][front] = 1.0;
						factors[rear][rear] = weight + 1.0 < 0.0 ? 0.0 : weight + 1.0;
					}
					else
					{
						factors[front][front] = 1.0 - weight < 0.0 ? 0.0 : 1.0 - weight;
						factors[rear][rear] = 1.0;
					}
				}
				break;
			}
			case 0: // left
			case 2:
			{
				int left = active_channel;
				int right = left + 1;
				factors[right][right] = 1.0;
				if ( weight < 0.0 ) // output left toward left
				{
					factors[left][left] = 0.5 - weight * 0.5;
					factors[left][right] = ( 1.0 + weight ) * 0.5;
				}
				else // output left toward right
				{
					factors[left][left] = ( 1.0 - weight ) * 0.5;
					factors[left][right] = 0.5 + weight * 0.5;
				}
				break;
			}
			case 1: // right
			case 3:
			{
				int right = active_channel;
				int left = right - 1;
				factors[left][left] = 1.0;
				if ( weight < 0.0 ) // output right toward left
				{
					factors[right][left] = 0.5 - weight * 0.5;
					factors[right][right] = ( 1.0 + weight ) * 0.5;
				}
				else // output right toward right
				{
					factors[right][left] = ( 1.0 - weight ) * 0.5;
					factors[right][right] = 0.5 + weight * 0.5;
				}
				break;
			}
		}

		// Do the mixing
		for ( out = 0; out < *channels && out < 6; out++ )
		{
			v = 0;
			for ( in = 0; in < *channels && in < 6; in++ )
				v += factors[in][out] * src[ i * *channels + in ];

			v = v < -32767 ? -32767 : v > 32768 ? 32768 : v;
			vp[out] = dest[ i * *channels + out ] = (int16_t) ( v * A + vp[ out ] * B );
		}
		weight += weight_step;
	}

	return 0;
}


/** Pan filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties frame_props = MLT_FRAME_PROPERTIES( frame );
	mlt_properties instance_props = mlt_properties_new();

	// Only if mix is specified, otherwise a producer may set the mix
	if ( mlt_properties_get( properties, "start" ) != NULL )
	{
		// Determine the time position of this frame in the filter duration
		mlt_properties props = mlt_properties_get_data( frame_props, "_producer", NULL );
		int always_active = mlt_properties_get_int(  properties, "always_active" );
		mlt_position in = !always_active ? mlt_filter_get_in( filter ) : mlt_properties_get_int( props, "in" );
		mlt_position out = !always_active ? mlt_filter_get_out( filter ) : mlt_properties_get_int( props, "out" );
		int length = mlt_properties_get_int(  properties, "length" );
		mlt_position time = !always_active ? mlt_frame_get_position( frame ) : mlt_properties_get_int( props, "_frame" );
		double mix = ( double )( time - in ) / ( double )( out - in + 1 );

		if ( length == 0 )
		{
			// If there is an end mix level adjust mix to the range
			if ( mlt_properties_get( properties, "end" ) != NULL )
			{
				double start = mlt_properties_get_double( properties, "start" );
				double end = mlt_properties_get_double( properties, "end" );
				mix = start + ( end - start ) * mix;
			}
			// Use constant mix level if only start
			else if ( mlt_properties_get( properties, "start" ) != NULL )
			{
				mix = mlt_properties_get_double( properties, "start" );
			}

			// Use animated property "split" to get mix level if property is set
			char* split_property = mlt_properties_get( properties, "split" );
			if ( split_property )
			{
				mlt_position pos = mlt_filter_get_position( filter, frame );
				mlt_position len = mlt_filter_get_length2( filter, frame );
				mix = mlt_properties_anim_get_double( properties, "split", pos, len );
			}

			// Convert it from [0, 1] to [-1, 1]
			mix = mix * 2.0 - 1.0;
		
			// Finally, set the mix property on the frame
			mlt_properties_set_double( instance_props, "mix", mix );
	
			// Initialise filter previous mix value to prevent an inadvertant jump from 0
			mlt_position last_position = mlt_properties_get_position( properties, "_last_position" );
			mlt_position current_position = mlt_frame_get_position( frame );
			mlt_properties_set_position( properties, "_last_position", current_position );
			if ( mlt_properties_get( properties, "_previous_mix" ) == NULL
				 || current_position != last_position + 1 )
				mlt_properties_set_double( properties, "_previous_mix", mix );
				
			// Tell the frame what the previous mix level was
			mlt_properties_set_double( instance_props, "previous_mix", mlt_properties_get_double( properties, "_previous_mix" ) );

			// Save the current mix level for the next iteration
			mlt_properties_set_double( properties, "_previous_mix", mix );
		}
		else
		{
			double level = mlt_properties_get_double( properties, "start" );
			double mix_start = level;
			double mix_end = mix_start;
			double mix_increment = 1.0 / length;
			if ( time - in < length )
			{
				mix_start *= ( double )( time - in ) / length;
				mix_end = mix_start + mix_increment;
			}
			else if ( time > out - length )
			{
				mix_end = mix_start * ( ( double )( out - time - in ) / length );
				mix_start = mix_end - mix_increment;
			}

			mix_start = mix_start < 0 ? 0 : mix_start > level ? level : mix_start;
			mix_end = mix_end < 0 ? 0 : mix_end > level ? level : mix_end;
			mlt_properties_set_double( instance_props, "previous_mix", mix_start );
			mlt_properties_set_double( instance_props, "mix", mix_end );
		}
		mlt_properties_set_int( instance_props, "channel", mlt_properties_get_int( properties, "channel" ) );
		mlt_properties_set_int( instance_props, "gang", mlt_properties_get_int( properties, "gang" ) );
	}
	mlt_properties_set_data( frame_props, mlt_properties_get( properties, "_unique_id" ),
		instance_props, 0, (mlt_destructor) mlt_properties_close, NULL );

	// Override the get_audio method
	mlt_frame_push_audio( frame, filter );
	mlt_frame_push_audio( frame, instance_props );
	mlt_frame_push_audio( frame, filter_get_audio );
	
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_panner_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = calloc( 1, sizeof( struct mlt_filter_s ) );
	if ( filter != NULL && mlt_filter_init( filter, NULL ) == 0 )
	{
		filter->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set_double( MLT_FILTER_PROPERTIES( filter ), "start", atof( arg ) );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "channel", -1 );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "split", NULL );
	}
	return filter;
}

