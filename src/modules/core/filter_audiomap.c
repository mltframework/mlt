/*
 * filter_audiomap.c -- remap audio channels
 * Copyright (C) 2015 Meltytech, LLC
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

#include <string.h>
#include <stdlib.h>

#define MAX_CHANNELS 32

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	char prop_name[32], *prop_val;
	int i, j, l, m[MAX_CHANNELS];

	mlt_filter filter = mlt_frame_pop_audio(frame);

	// Get the producer's audio
	int error = mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	if ( error ) return error;

	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

	/* find samples length */
	int len = mlt_audio_format_size( *format, 1, 1 );

	/* pcm samples buffer */
	uint8_t *pcm = *buffer;

	/* build matrix */
	for ( i = 0; i < MAX_CHANNELS; i++ )
	{
		m[i] = i;

		snprintf( prop_name, sizeof(prop_name), "%d", i );
		if ( ( prop_val = mlt_properties_get( properties, prop_name ) ) )
		{
			j = atoi( prop_val );
			if( j >= 0 && j < MAX_CHANNELS )
				m[i] = j;
		}
	}

	/* process samples */
	for ( i = 0; i < *samples; i++ )
	{
		uint8_t tmp[MAX_CHANNELS * 4];

		for ( j = 0; j < MAX_CHANNELS && j < *channels; j++ )
			for(l = 0; l < len; l++)
				tmp[j * len + l] = pcm[m[j] * len + l];

		for ( j = 0; j < MAX_CHANNELS && j < *channels; j++ )
			for ( l = 0; l < len; l++ )
				pcm[j * len + l] = tmp[j * len + l];

		pcm += len * (*channels);
	}

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

/** Constructor for the filter.
*/

mlt_filter filter_audiomap_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	if ( filter )
		filter->process = filter_process;
	return filter;
}
