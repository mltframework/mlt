/*
 * filter_loudness_meter.c -- measure audio loudness according to EBU R128
 * Copyright (C) 2016 Brian Matherly <code@brianmatherly.com>
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
	int reset;
	mlt_position prev_pos;
} private_data;

static void property_changed( mlt_service owner, mlt_filter filter, char *name )
{
	private_data* pdata = (private_data*)filter->child;
	if ( !strcmp( name, "reset" ) ||
		!strcmp( name, "calc_program" ) ||
		!strcmp( name, "calc_shortterm" ) ||
		!strcmp( name, "calc_momentary" ) ||
		!strcmp( name, "calc_range" ) ||
		!strcmp( name, "calc_peak" ) ||
		!strcmp( name, "calc_true_peak" ) )
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
		pdata->reset = 0;
		pdata->prev_pos = -1;
		mlt_events_block( properties, filter );
		mlt_properties_set( properties, "frames_processed", "0" );
		mlt_properties_set( properties, "program", "-100.0" );
		mlt_properties_set( properties, "shortterm", "-100.0" );
		mlt_properties_set( properties, "momentary", "-100.0" );
		mlt_properties_set( properties, "range", "-1.0" );
		mlt_properties_set_int( properties, "reset_count", mlt_properties_get_int( properties, "reset_count") + 1 );
		mlt_properties_set_int( properties, "reset", 0 );
		mlt_events_unblock( properties, filter );
	}

	if( !pdata->r128 )
	{
		int mode = EBUR128_MODE_HISTOGRAM;

		if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_program" ) )
		{
			mode |= EBUR128_MODE_I;
		}

		if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_shortterm" ) )
		{
			mode |= EBUR128_MODE_S;
		}

		if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_momentary" ) )
		{
			mode |= EBUR128_MODE_M;
		}

		if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_range" ) )
		{
			mode |= EBUR128_MODE_LRA;
		}

		if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_peak" ) )
		{
			mode |= EBUR128_MODE_SAMPLE_PEAK;
		}

		if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_true_peak" ) )
		{
			mode |= EBUR128_MODE_TRUE_PEAK;
		}

		pdata->r128 = ebur128_init( channels, frequency,  mode );
	}
}

static void analyze_audio( mlt_filter filter, void* buffer, int samples )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	private_data* pdata = (private_data*)filter->child;
	int result = -1;
	double loudness = 0.0;

	ebur128_add_frames_float( pdata->r128, buffer, samples );

	if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_program" ) )
	{
		result = ebur128_loudness_global( pdata->r128, &loudness );
		if( result == EBUR128_SUCCESS && loudness != HUGE_VAL && loudness != -HUGE_VAL )
		{
			mlt_properties_set_double( properties, "program", loudness );
		}
	}

	if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_shortterm" ) )
	{
		result = ebur128_loudness_shortterm( pdata->r128, &loudness );
		if( result == EBUR128_SUCCESS && loudness != HUGE_VAL && loudness != -HUGE_VAL )
		{
			mlt_properties_set_double( properties, "shortterm", loudness );
		}
	}

	if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_momentary" ) )
	{
		result = ebur128_loudness_momentary( pdata->r128, &loudness );
		if( result == EBUR128_SUCCESS && loudness != HUGE_VAL && loudness != -HUGE_VAL )
		{
			mlt_properties_set_double( properties, "momentary", loudness );
		}
	}

	if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_range" ) )
	{
		double range = 0;
		result = ebur128_loudness_range( pdata->r128, &range );
		if( result == EBUR128_SUCCESS && range != HUGE_VAL && range != -HUGE_VAL )
		{
			mlt_properties_set_double( properties, "range", range );
		}
	}

	if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_peak" ) )
	{
		double prev_peak = 0.0;
		double max_peak = 0.0;
		int c = 0;
		for( c = 0; c < pdata->r128->channels; c++ )
		{
			double peak;
			result = ebur128_sample_peak( pdata->r128, c, &peak );
			if( result == EBUR128_SUCCESS && peak != HUGE_VAL && peak > max_peak )
			{
				max_peak = peak;
			}
			result = ebur128_prev_sample_peak( pdata->r128, c, &peak );
			if( result == EBUR128_SUCCESS && peak != HUGE_VAL && peak > prev_peak )
			{
				prev_peak = peak;
			}
		}
		mlt_properties_set_double( properties, "max_peak", 20 * log10(max_peak) );
		mlt_properties_set_double( properties, "peak", 20 * log10(prev_peak) );
	}

	if( mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "calc_true_peak" ) )
	{
		double prev_peak = 0.0;
		double max_peak = 0.0;
		int c = 0;
		for( c = 0; c < pdata->r128->channels; c++ )
		{
			double peak;
			result = ebur128_true_peak( pdata->r128, c, &peak );
			if( result == EBUR128_SUCCESS && peak != HUGE_VAL && peak > max_peak )
			{
				max_peak = peak;
			}
			result = ebur128_prev_true_peak( pdata->r128, c, &peak );
			if( result == EBUR128_SUCCESS && peak != HUGE_VAL && peak > prev_peak )
			{
				prev_peak = peak;
			}
		}
		mlt_properties_set_double( properties, "max_true_peak", 20 * log10(max_peak) );
		mlt_properties_set_double( properties, "true_peak", 20 * log10(prev_peak) );
	}

	mlt_properties_set_position( properties, "frames_processed", mlt_properties_get_position( properties, "frames_processed" ) + 1 );
}

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_filter filter = mlt_frame_pop_audio( frame );
	private_data* pdata = (private_data*)filter->child;
	mlt_position pos = mlt_frame_get_position( frame );

	// Get the producer's audio
	*format = mlt_audio_f32le;
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	check_for_reset( filter, *channels, *frequency );

	if( pos != pdata->prev_pos )
	{
		// Only analyze the audio if the producer is not paused.
		analyze_audio( filter, *buffer, *samples );
	}

	pdata->prev_pos = pos;

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

mlt_filter filter_loudness_meter_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if( filter && pdata )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set_int( properties, "calc_program", 1 );
		mlt_properties_set_int( properties, "calc_shortterm", 1 );
		mlt_properties_set_int( properties, "calc_momentary", 1 );
		mlt_properties_set_int( properties, "calc_range", 1 );
		mlt_properties_set_int( properties, "calc_peak", 1 );
		mlt_properties_set_int( properties, "calc_true_peak", 1 );
		mlt_properties_set( properties, "program", "-100.0" );
		mlt_properties_set( properties, "shortterm", "-100.0" );
		mlt_properties_set( properties, "momentary", "-100.0" );
		mlt_properties_set( properties, "range", "-1.0" );
		mlt_properties_set( properties, "peak", "-100.0" );
		mlt_properties_set( properties, "max_peak", "-100.0" );
		mlt_properties_set( properties, "true_peak", "-100.0" );
		mlt_properties_set( properties, "max_true_peak", "-100.0" );
		mlt_properties_set( properties, "reset", "1" );
		mlt_properties_set( properties, "reset_count", "0" );
		mlt_properties_set( properties, "frames_processed", "0" );

		pdata->r128 = 0;
		pdata->reset = 1;
		pdata->prev_pos = -1;

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
