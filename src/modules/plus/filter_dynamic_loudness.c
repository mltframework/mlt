/*
 * filter_loudness.c -- normalize audio according to EBU R128
 * Copyright (C) 2014 Brian Matherly <code@brianmatherly.com>
 * Author: Brian Matherly <code@brianmatherly.com>
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
#include <math.h>
#include "ebur128/ebur128.h"

typedef struct
{
	ebur128_state* r128;
	double target_gain;
	double start_gain;
	double end_gain;
	int reset;
	unsigned int time_elapsed_ms;
	mlt_position prev_o_pos;
} private_data;

static void property_changed( mlt_service owner, mlt_filter filter, char *name )
{
	private_data* pdata = (private_data*)filter->child;
	if ( !strcmp( name, "window" ) )
	{
		pdata->reset = 1;
	}
}

static void check_for_reset( mlt_filter filter, int channels, int frequency )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	private_data* pdata = (private_data*)filter->child;

	if( pdata->reset )
	{
		if( pdata->r128 )
		{
			ebur128_destroy( &pdata->r128 );
		}
		pdata->r128 = 0;
		pdata->target_gain = 0.0;
		pdata->start_gain = 0.0;
		pdata->end_gain = 0.0;
		pdata->reset = 0;
		pdata->time_elapsed_ms = 0;
		pdata->prev_o_pos = -1;
		mlt_properties_set_double( properties, "out_gain", 0.0 );
		mlt_properties_set_double( properties, "in_loudness", -100.0 );
		mlt_properties_set_int( properties, "reset_count", mlt_properties_get_int( properties, "reset_count") + 1 );
	}

	if( !pdata->r128 )
	{
		pdata->r128 = ebur128_init( channels, frequency, EBUR128_MODE_I );
		ebur128_set_max_window( pdata->r128, 400 );
		ebur128_set_max_history( pdata->r128, mlt_properties_get_int( properties, "window" ) * 1000.0 );
	}
}

static void analyze_audio( mlt_filter filter, void* buffer, int samples, int frequency )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	private_data* pdata = (private_data*)filter->child;
	int result = -1;
	double in_loudness = 0.0;
	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE(filter) );
	double fps = mlt_profile_fps( profile );

	ebur128_add_frames_float( pdata->r128, buffer, samples );

	if( pdata->time_elapsed_ms < 400 )
	{
		// Waiting for first program loudness measurement.
		// Use window loudness as initial guess.
		result = ebur128_loudness_window( pdata->r128, pdata->time_elapsed_ms, &in_loudness );
		pdata->time_elapsed_ms += samples * 1000 / frequency;
	}
	else
	{
		result = ebur128_loudness_global( pdata->r128, &in_loudness );
	}

	if( result == EBUR128_SUCCESS && in_loudness != HUGE_VAL && in_loudness != -HUGE_VAL )
	{
		mlt_properties_set_double( properties, "in_loudness", in_loudness );
		double target_loudness = mlt_properties_get_double( properties, "target_loudness" );
		pdata->target_gain = target_loudness - in_loudness;

		// Make sure gain limits are not exceeded.
		double max_gain = mlt_properties_get_double( properties, "max_gain" );
		double min_gain = mlt_properties_get_double( properties, "min_gain" );
		if( pdata->target_gain > max_gain )
		{
			pdata->target_gain = max_gain;
		}
		else if ( pdata->target_gain < min_gain )
		{
			pdata->target_gain = min_gain;
		}
	}

	// Make sure gain does not change too quickly.
	pdata->start_gain = pdata->end_gain;
	pdata->end_gain = pdata->target_gain;
	double max_frame_gain = mlt_properties_get_double( properties, "max_rate" ) / fps;
	if( pdata->start_gain - pdata->end_gain > max_frame_gain )
	{
		pdata->end_gain = pdata->start_gain - max_frame_gain;
	}
	else if( pdata->end_gain - pdata->start_gain > max_frame_gain )
	{
		pdata->end_gain = pdata->start_gain + max_frame_gain;
	}
	mlt_properties_set_double( properties, "out_gain", pdata->end_gain );
}

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_filter filter = mlt_frame_pop_audio( frame );
	private_data* pdata = (private_data*)filter->child;
	mlt_position o_pos = mlt_frame_original_position( frame );

	// Get the producer's audio
	*format = mlt_audio_f32le;
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	if( abs( o_pos - pdata->prev_o_pos ) > 1 )
	{
		// Assume this is a new clip and restart
		// Use original position so that transitions between clips are detected.
		pdata->reset = 1;
		mlt_log_info( MLT_FILTER_SERVICE( filter ), "Reset. Old Pos: %d\tNew Pos: %d\n", pdata->prev_o_pos, o_pos );
	}

	check_for_reset( filter, *channels, *frequency );

	if( o_pos != pdata->prev_o_pos )
	{
		// Only analyze the audio is the producer is not paused.
		analyze_audio( filter, *buffer, *samples, *frequency );
	}

	double start_coeff = pdata->start_gain > -90.0 ? pow(10.0, pdata->start_gain / 20.0) : 0.0;
	double end_coeff = pdata->end_gain > -90.0 ? pow(10.0, pdata->end_gain / 20.0) : 0.0;
	double coeff_factor = pow( (end_coeff / start_coeff), 1.0 / (double)*samples );
	double coeff = start_coeff;
	float* p = *buffer;
	int s = 0;
	int c = 0;
	for( s = 0; s < *samples; s++ )
	{
		coeff = coeff * coeff_factor;
		for ( c = 0; c < *channels; c++ )
		{
			*p = *p * coeff;
			p++;
		}
	}

	pdata->prev_o_pos = o_pos;

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_audio( frame, filter );
	mlt_frame_push_audio( frame, filter_get_audio );
	return frame;
}

/** Destructor for the filter.
*/

static void filter_close( mlt_filter filter )
{
	private_data* pdata = (private_data*)filter->child;

	if( pdata )
	{
		if( pdata->r128 )
		{
			ebur128_destroy( &pdata->r128 );
		}
		free( pdata );
	}
	filter->child = NULL;
	filter->close = NULL;
	filter->parent.close = NULL;
	mlt_service_close( &filter->parent );
}

/** Constructor for the filter.
*/

mlt_filter filter_dynamic_loudness_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if( filter && pdata )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set( properties, "target_loudness", "-23.0" );
		mlt_properties_set( properties, "window", "3.0" );
		mlt_properties_set( properties, "max_gain", "15.0" );
		mlt_properties_set( properties, "min_gain", "-15.0" );
		mlt_properties_set( properties, "max_rate", "3.0" );
		mlt_properties_set( properties, "in_loudness", "-100.0" );
		mlt_properties_set( properties, "out_gain", "0.0" );
		mlt_properties_set( properties, "reset_count", "0" );

		pdata->target_gain = 0.0;
		pdata->start_gain = 0.0;
		pdata->end_gain = 0.0;
		pdata->r128 = 0;
		pdata->reset = 1;
		pdata->time_elapsed_ms = 0;
		pdata->prev_o_pos = 0;

		filter->close = filter_close;
		filter->process = filter_process;
		filter->child = pdata;

		mlt_events_listen( properties, filter, "property-changed", (mlt_listener)property_changed );
	}
	else
	{
		if( filter )
		{
			mlt_filter_close( filter );
			filter = NULL;
		}

		free( pdata );
	}

	return filter;
}
