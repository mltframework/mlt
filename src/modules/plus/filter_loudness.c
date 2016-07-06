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

#define MAX_RESULT_SIZE 512

typedef struct
{
	ebur128_state* state;
} analyze_data;

typedef struct
{
	double in_loudness;
	double in_range;
	double in_peak;
} apply_data;

typedef struct
{
	analyze_data* analyze;
	apply_data* apply;
	mlt_position last_position;
} private_data;

static void destroy_analyze_data( mlt_filter filter )
{
	private_data* private = (private_data*)filter->child;
	ebur128_destroy( &private->analyze->state );
	free( private->analyze );
	private->analyze = NULL;
}

static void init_analyze_data( mlt_filter filter, int channels, int samplerate )
{
	private_data* private = (private_data*)filter->child;
	private->analyze = (analyze_data*)calloc( 1, sizeof(analyze_data) );
	private->analyze->state = ebur128_init( (unsigned int)channels, (unsigned long)samplerate, EBUR128_MODE_I | EBUR128_MODE_LRA | EBUR128_MODE_SAMPLE_PEAK );
	private->last_position = 0;
}

static void destroy_apply_data( mlt_filter filter )
{
	private_data* private = (private_data*)filter->child;
	free( private->apply );
	private->apply = NULL;
}

static void init_apply_data( mlt_filter filter )
{
	private_data* private = (private_data*)filter->child;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	char* results = mlt_properties_get( properties, "results" );
	int scan_return = 0;

	private->apply = (apply_data*)calloc( 1, sizeof(apply_data) );

	scan_return = sscanf( results,"L: %lf\tR: %lf\tP %lf", &private->apply->in_loudness, &private->apply->in_range, &private->apply->in_peak );
	if( scan_return != 3 )
	{
		mlt_log_error( MLT_FILTER_SERVICE( filter ), "Unable to load results: %s\n", results );
		destroy_apply_data( filter );
		return;
	}
	else
	{
		mlt_log_info( MLT_FILTER_SERVICE( filter ), "Loaded Results: L: %lf\tR: %lf\tP %lf\n", private->apply->in_loudness, private->apply->in_range, private->apply->in_peak );
	}
}

static void analyze( mlt_filter filter, mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	private_data* private = (private_data*)filter->child;
	mlt_position pos = mlt_filter_get_position( filter, frame );

	// If any frames are skipped, analysis data will be incomplete.
	if( private->analyze && pos != private->last_position + 1 )
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Analysis Failed: Bad frame sequence\n" );
		destroy_analyze_data( filter );
	}

	// Analyze Audio
	if( !private->analyze && pos == 0 )
	{
		init_analyze_data( filter, *channels, *frequency );
	}

	if( private->analyze )
	{
		ebur128_add_frames_float( private->analyze->state, *buffer, *samples );

		if ( pos + 1 == mlt_filter_get_length2( filter, frame ) )
		{
			mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
			double loudness = 0.0;
			double range = 0.0;
			double tmpPeak = 0.0;
			double peak = 0.0;
			int i = 0;
			char result[MAX_RESULT_SIZE];
			ebur128_loudness_global( private->analyze->state, &loudness );
			ebur128_loudness_range( private->analyze->state, &range );

			for ( i = 0; i < *channels; i++ )
			{
				ebur128_sample_peak( private->analyze->state, i, &tmpPeak );
				if( tmpPeak > peak )
				{
					peak = tmpPeak;
				}
			}

			snprintf( result, MAX_RESULT_SIZE, "L: %lf\tR: %lf\tP %lf", loudness, range, peak );
			result[ MAX_RESULT_SIZE - 1 ] = '\0';
			mlt_log_info( MLT_FILTER_SERVICE( filter ), "Stored results: %s\n", result );
			mlt_properties_set( properties, "results", result );
			destroy_analyze_data( filter );
		}

		private->last_position = pos;
	}
}

static void apply( mlt_filter filter, mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	private_data* private = (private_data*)filter->child;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

	// Analyze Audio
	if( !private->apply )
	{
		init_apply_data( filter );
	}

	if( private->apply )
	{
		double target_db = mlt_properties_get_double( properties, "program" );
		double delta_db = target_db - private->apply->in_loudness;
		double coeff = delta_db > -90.0 ? pow(10.0, delta_db / 20.0) : 0.0;
		float* p = *buffer;
		int count = *samples * *channels;
		while( count-- )
		{
			*p = *p * coeff;
			p++;
		}
	}
}

/** Get the audio.
*/

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_filter filter = mlt_frame_pop_audio( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	// Get the producer's audio
	*format = mlt_audio_f32le;
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	char* results = mlt_properties_get( properties, "results" );
	if( results && strcmp( results, "" ) )
	{
		apply( filter, frame, buffer, format, frequency, channels, samples );
	}
	else
	{
		analyze( filter, frame, buffer, format, frequency, channels, samples );
	}

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
	private_data* private = (private_data*)filter->child;

	if ( private )
	{
		if ( private->analyze )
		{
			destroy_analyze_data( filter );
		}
		free( private );
	}
	filter->child = NULL;
	filter->close = NULL;
	filter->parent.close = NULL;
	mlt_service_close( &filter->parent );
}

/** Constructor for the filter.
*/

mlt_filter filter_loudness_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	private_data* data = (private_data*)calloc( 1, sizeof(private_data) );

	if ( filter && data )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set( properties, "program", "-23.0" );

		data->analyze = NULL;

		filter->close = filter_close;
		filter->process = filter_process;
		filter->child = data;
	}
	else
	{
		if( filter )
		{
			mlt_filter_close( filter );
			filter = NULL;
		}

		if( data )
		{
			free( data );
		}
	}

	return filter;
}
