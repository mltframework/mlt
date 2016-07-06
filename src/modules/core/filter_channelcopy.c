/*
 * filter_channelcopy.c -- copy one audio channel to another
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Get the audio.
*/

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the properties of the a frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Get the filter service
	mlt_filter filter = mlt_frame_pop_audio( frame );

	int from = mlt_properties_get_int( properties, "channelcopy.from" );
	int to = mlt_properties_get_int( properties, "channelcopy.to" );
	int swap = mlt_properties_get_int( properties, "channelcopy.swap" );

	// Get the producer's audio
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	// Copy channels as necessary
	if ( from != to)
	switch ( *format )
	{
		case mlt_audio_u8:
		{
			uint8_t *f = (uint8_t*) *buffer + from;
			uint8_t *t = (uint8_t*) *buffer + to;
			uint8_t x;
			int i;

			if ( swap )
				for ( i = 0; i < *samples; i++, f += *channels, t += *channels )
				{
					x = *t;
					*t = *f;
					*f = x;
				}
			else
				for ( i = 0; i < *samples; i++, f += *channels, t += *channels )
					*t = *f;
			break;
		}
		case mlt_audio_s16:
		{
			int16_t *f = (int16_t*) *buffer + from;
			int16_t *t = (int16_t*) *buffer + to;
			int16_t x;
			int i;

			if ( swap )
				for ( i = 0; i < *samples; i++, f += *channels, t += *channels )
				{
					x = *t;
					*t = *f;
					*f = x;
				}
			else
				for ( i = 0; i < *samples; i++, f += *channels, t += *channels )
					*t = *f;
			break;
		}
		case mlt_audio_s32:
		{
			int32_t *f = (int32_t*) *buffer + from * *samples;
			int32_t *t = (int32_t*) *buffer + to * *samples;

			if ( swap )
			{
				int32_t *x = malloc( *samples * sizeof(int32_t) );
				memcpy( x, t, *samples * sizeof(int32_t) );
				memcpy( t, f, *samples * sizeof(int32_t) );
				memcpy( f, x, *samples * sizeof(int32_t) );
				free( x );
			}
			else
			{
				memcpy( t, f, *samples * sizeof(int32_t) );
			}
			break;
		}
		case mlt_audio_s32le:
		case mlt_audio_f32le:
		{
			int32_t *f = (int32_t*) *buffer + from;
			int32_t *t = (int32_t*) *buffer + to;
			int32_t x;
			int i;

			if ( swap )
				for ( i = 0; i < *samples; i++, f += *channels, t += *channels )
				{
					x = *t;
					*t = *f;
					*f = x;
				}
			else
				for ( i = 0; i < *samples; i++, f += *channels, t += *channels )
					*t = *f;
			break;
		}
		case mlt_audio_float:
		{
			float *f = (float*) *buffer + from * *samples;
			float *t = (float*) *buffer + to * *samples;

			if ( swap )
			{
				float *x = malloc( *samples * sizeof(float) );
				memcpy( x, t, *samples * sizeof(float) );
				memcpy( t, f, *samples * sizeof(float) );
				memcpy( f, x, *samples * sizeof(float) );
				free( x );
			}
			else
			{
				memcpy( t, f, *samples * sizeof(float) );
			}
			break;
		}
		default:
			mlt_log_error( MLT_FILTER_SERVICE( filter ), "Invalid audio format\n" );
			break;
	}

	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties frame_props = MLT_FRAME_PROPERTIES( frame );

	// Propogate the parameters
	mlt_properties_set_int( frame_props, "channelcopy.to", mlt_properties_get_int( properties, "to" ) );
	mlt_properties_set_int( frame_props, "channelcopy.from", mlt_properties_get_int( properties, "from" ) );
	mlt_properties_set_int( frame_props, "channelcopy.swap", mlt_properties_get_int( properties, "swap" ) );

	// Override the get_audio method
	mlt_frame_push_audio( frame, filter );
	mlt_frame_push_audio( frame, filter_get_audio );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_channelcopy_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "to", atoi( arg ) );
		else
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "to", 1 );
		if ( strcmp(id, "channelswap") == 0 )
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "swap", 1 );
	}
	return filter;
}

