/*
 * filter_audiochannels.c -- convert from one audio format to another
 * Copyright (C) 2009 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <string.h>

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Used to return number of channels in the source
	int channels_avail = *channels;

	// Get the producer's audio
	int error = mlt_frame_get_audio( frame, buffer, format, frequency, &channels_avail, samples );
	if ( error ) return error;

	if ( channels_avail < *channels )
	{
		int size = mlt_audio_format_size( *format, *samples, *channels );
		int16_t *new_buffer = mlt_pool_alloc( size );

		// Duplicate the existing channels
		if ( *format == mlt_audio_s16 )
		{
			int i, j, k;
			for ( i = 0; i < *samples; i++ )
			{
				for ( j = 0; j < *channels; j++ )
				{
					new_buffer[ ( i * *channels ) + j ]	= ((int16_t*)(*buffer))[ ( i * channels_avail ) + k ];
					k = ( k + 1 ) % channels_avail;
				}
			}
		}
		else
		{
			// non-interleaved
			int size_avail = mlt_audio_format_size( *format, *samples, channels_avail );
			int32_t *p = (int32_t*) new_buffer;
			int i = *channels / channels_avail;
			while ( i-- )
			{
				memcpy( p, *buffer, size_avail );
				p += size_avail / sizeof(*p);
			}
			i = *channels % channels_avail;
			if ( i )
			{
				size_avail = mlt_audio_format_size( *format, *samples, i );
				memcpy( p, *buffer, size_avail );
			}
		}
		// Update the audio buffer now - destroys the old
		mlt_frame_set_audio( frame, new_buffer, *format, size, mlt_pool_release );
		*buffer = new_buffer;
	}
	else if ( channels_avail > *channels )
	{
		int size = mlt_audio_format_size( *format, *samples, *channels );
		int16_t *new_buffer = mlt_pool_alloc( size );

		// Drop all but the first *channels
		if ( *format == mlt_audio_s16 )
		{
			int i, j;
			for ( i = 0; i < *samples; i++ )
				for ( j = 0; j < *channels; j++ )
					new_buffer[ ( i * *channels ) + j ]	= ((int16_t*)(*buffer))[ ( i * channels_avail ) + j ];
		}
		else
		{
			// non-interleaved
			memcpy( new_buffer, *buffer, size );
		}
		// Update the audio buffer now - destroys the old
		mlt_frame_set_audio( frame, new_buffer, *format, size, mlt_pool_release );
		*buffer = new_buffer;
	}
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_audio( frame, filter_get_audio );
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_audiochannels_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	if ( filter )
		filter->process = filter_process;
	return filter;
}
