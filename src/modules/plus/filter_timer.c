/*
 * filter_timer.c -- timer text overlay filter
 * Copyright (C) 2018-2019 Meltytech, LLC
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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXT_LEN 512

double time_to_seconds( char* time )
{
	int hours = 0;
	int mins = 0;
	double secs = 0;
	if ( time )
		sscanf( time, "%d:%d:%lf", &hours, &mins, &secs );
	return ( hours * 60.0 * 60.0 ) + ( mins * 60.0 ) + secs;
}

static void get_timer_str( mlt_filter filter, mlt_frame frame, char* text )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position current_frame = mlt_filter_get_position( filter, frame );
	char* direction = mlt_properties_get( properties, "direction" );
	double timer_start = time_to_seconds( mlt_properties_get( properties, "start" ) );
	double timer_duration = time_to_seconds( mlt_properties_get( properties, "duration" ) );
	double timer_offset = time_to_seconds( mlt_properties_get( properties, "offset" ) );
	double value = time_to_seconds( mlt_properties_frames_to_time( properties, current_frame, mlt_time_clock ) );

	if ( timer_duration <= 0.0 )
	{
		// "duration" of zero means entire length of the filter.
		mlt_position filter_length = mlt_filter_get_length2( filter, frame ) - 1;
		double filter_duration = time_to_seconds( mlt_properties_frames_to_time( properties, filter_length, mlt_time_clock ) );
		timer_duration = filter_duration - timer_start;
	}

	if ( value < timer_start )
	{
		// Hold at 0 until start time.
		value = 0.0;
	}
	else
	{
		value = value - timer_start;
		if ( value > timer_duration )
		{
			// Hold at duration after the timer has elapsed.
			value = timer_duration;
		}
	}

	// Apply direction.
	if ( direction && !strcmp( direction, "down" ) )
	{
		value = timer_duration - value;
	}

	// Apply offset
	value += timer_offset;

	int hours = value / ( 60 * 60 );
	int mins = ( value / 60 ) - ( hours * 60 );
	double secs = value - (double)( mins * 60 ) - (double)( hours * 60 * 60 );
	char* format = mlt_properties_get( properties, "format" );

	if ( !strcmp( format, "HH:MM:SS" ) )
	{
		snprintf( text, MAX_TEXT_LEN, "%02d:%02d:%02d", hours, mins, (int)floor(secs) );
	}
	else if ( !strcmp( format, "HH:MM:SS.S" ) )
	{
		snprintf( text, MAX_TEXT_LEN, "%02d:%02d:%04.1f", hours, mins, floor(secs * 10.0) / 10.0 );
	}
	else if ( !strcmp( format, "MM:SS" ) )
	{
		snprintf( text, MAX_TEXT_LEN, "%02d:%02d", hours * 60 + mins, (int)floor(secs) );
	}
	else if ( !strcmp( format, "MM:SS.SS" ) )
	{
		snprintf( text, MAX_TEXT_LEN, "%02d:%05.2f", hours * 60 + mins, floor(secs * 100.0) / 100.0 );
	}
	else if ( !strcmp( format, "MM:SS.SSS" ) )
	{
		snprintf( text, MAX_TEXT_LEN, "%02d:%06.3f", hours * 60 + mins, floor(secs * 1000.0) / 1000.0 );
	}
	else if ( !strcmp( format, "SS" ) )
	{
		snprintf( text, MAX_TEXT_LEN, "%02d", (int)floor(value) );
	}
	else if ( !strcmp( format, "SS.S" ) )
	{
		snprintf( text, MAX_TEXT_LEN, "%04.1f", floor(value * 10.0) / 10.0 );
	}
	else if ( !strcmp( format, "SS.SS" ) )
	{
		snprintf( text, MAX_TEXT_LEN, "%05.2f", floor(value * 100.0) / 100.0 );
	}
	else if ( !strcmp( format, "SS.SSS" ) )
	{
		snprintf( text, MAX_TEXT_LEN, "%06.3f", floor(value * 1000.0) / 1000.0 );
	}
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_filter text_filter = mlt_properties_get_data( properties, "_text_filter", NULL );
	mlt_properties text_filter_properties = mlt_frame_unique_properties( frame, MLT_FILTER_SERVICE(text_filter));
	char* result = calloc( 1, MAX_TEXT_LEN );
	get_timer_str( filter, frame, result );
	mlt_properties_set( text_filter_properties, "argument", result );
	free( result );
	mlt_properties_pass_list( text_filter_properties, properties,
		"geometry family size weight style fgcolour bgcolour olcolour pad halign valign outline" );
	mlt_filter_set_in_and_out( text_filter, mlt_filter_get_in( filter ), mlt_filter_get_out( filter ) );
	return mlt_filter_process( text_filter, frame );
}

/** Constructor for the filter.
*/
mlt_filter filter_timer_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	mlt_filter text_filter = mlt_factory_filter( profile, "qtext", NULL );

	if( !text_filter )
		text_filter = mlt_factory_filter( profile, "text", NULL );

	if( !text_filter )
		mlt_log_warning( MLT_FILTER_SERVICE(filter), "Unable to create text filter.\n" );

	if ( filter && text_filter )
	{
		mlt_properties my_properties = MLT_FILTER_PROPERTIES( filter );

		// Register the text filter for reuse/destruction
		mlt_properties_set_data( my_properties, "_text_filter", text_filter, 0, ( mlt_destructor )mlt_filter_close, NULL );

		// Assign default values
		mlt_properties_set( my_properties, "format", "SS.SS" );
		mlt_properties_set( my_properties, "start", "00:00:00.000" );
		mlt_properties_set( my_properties, "duration", "00:10:00.000" );
		mlt_properties_set( my_properties, "offset", "00:00:00.000" );
		mlt_properties_set( my_properties, "direction", "up" );
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

		if( text_filter )
		{
			mlt_filter_close( text_filter );
		}

		filter = NULL;
	}
	return filter;
}
