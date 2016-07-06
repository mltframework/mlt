/*
 * filter_mono.c -- mix all channels to a mono signal across n channels
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

/** Get the audio.
*/

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the properties of the a frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	int channels_out = mlt_properties_get_int( properties, "mono.channels" );
	int i, j, size;

	// Get the producer's audio
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	if ( channels_out == -1 )
		channels_out = *channels;
	size = mlt_audio_format_size( *format, *samples, channels_out );

	switch ( *format )
	{
		case mlt_audio_u8:
		{
			uint8_t *new_buffer = mlt_pool_alloc( size );
			for ( i = 0; i < *samples; i++ )
			{
				uint8_t mixdown = 0;
				for ( j = 0; j < *channels; j++ )
					mixdown += ((uint8_t*) *buffer)[ ( i * *channels ) + j ] / *channels;
				for ( j = 0; j < channels_out; j++ )
					new_buffer[ ( i * channels_out ) + j ] = mixdown;
			}
			*buffer = new_buffer;
			break;
		}
		case mlt_audio_s16:
		{
			int16_t *new_buffer = mlt_pool_alloc( size );
			for ( i = 0; i < *samples; i++ )
			{
				int16_t mixdown = 0;
				for ( j = 0; j < *channels; j++ )
					mixdown += ((int16_t*) *buffer)[ ( i * *channels ) + j ] / *channels;
				for ( j = 0; j < channels_out; j++ )
					new_buffer[ ( i * channels_out ) + j ] = mixdown;
			}
			*buffer = new_buffer;
			break;
		}
		case mlt_audio_s32le:
		{
			int32_t *new_buffer = mlt_pool_alloc( size );
			for ( i = 0; i < *samples; i++ )
			{
				int32_t mixdown = 0;
				for ( j = 0; j < *channels; j++ )
					mixdown += ((int32_t*) *buffer)[ ( i * *channels ) + j ] / *channels;
				for ( j = 0; j < channels_out; j++ )
					new_buffer[ ( i * channels_out ) + j ] = mixdown;
			}
			*buffer = new_buffer;
			break;
		}
		case mlt_audio_f32le:
		{
			float *new_buffer = mlt_pool_alloc( size );
			for ( i = 0; i < *samples; i++ )
			{
				float mixdown = 0;
				for ( j = 0; j < *channels; j++ )
					mixdown += ((float*) *buffer)[ ( i * *channels ) + j ] / *channels;
				for ( j = 0; j < channels_out; j++ )
					new_buffer[ ( i * channels_out ) + j ] = mixdown;
			}
			*buffer = new_buffer;
			break;
		}
		case mlt_audio_s32:
		{
			int32_t *new_buffer = mlt_pool_alloc( size );
			for ( i = 0; i < *samples; i++ )
			{
				int32_t mixdown = 0;
				for ( j = 0; j < *channels; j++ )
					mixdown += ((int32_t*) *buffer)[ ( j * *channels ) + i ] / *channels;
				for ( j = 0; j < channels_out; j++ )
					new_buffer[ ( j * *samples ) + i ] = mixdown;
			}
			*buffer = new_buffer;
			break;
		}
		case mlt_audio_float:
		{
			float *new_buffer = mlt_pool_alloc( size );
			for ( i = 0; i < *samples; i++ )
			{
				float mixdown = 0;
				for ( j = 0; j < *channels; j++ )
					mixdown += ((float*) *buffer)[ ( j * *channels ) + i ] / *channels;
				for ( j = 0; j < channels_out; j++ )
					new_buffer[ ( j * *samples ) + i ] = mixdown;
			}
			*buffer = new_buffer;
			break;
		}
		default:
			mlt_log_error( NULL, "[filter mono] Invalid audio format\n" );
			break;
	}
	if ( size > *samples * channels_out )
	{
		mlt_frame_set_audio( frame, *buffer, *format, size, mlt_pool_release );
		*channels = channels_out;
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
	mlt_properties_set_int( frame_props, "mono.channels", mlt_properties_get_int( properties, "channels" ) );

	// Override the get_audio method
	mlt_frame_push_audio( frame, filter_get_audio );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_mono_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "channels", atoi( arg ) );
		else
			mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "channels", -1 );
	}
	return filter;
}

