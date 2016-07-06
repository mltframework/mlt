/*
 * filter_audiolevel.c -- get the audio level of each channel
 * Copyright (C) 2002 Steve Harris
 * Copyright (C) 2010 Marco Gittler <g.marco@freenet.de>
 * Copyright (C) 2012 Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <stdlib.h>
#include <math.h>

#define AMPTODBFS(n) (log10(n) * 20.0)

//----------------------------------------------------------------------------
// IEC standard dB scaling -- as borrowed from meterbridge (c) Steve Harris

static inline double IEC_Scale(double dB)
{
	double fScale = 1.0f;

	if (dB < -70.0f)
		fScale = 0.0f;
	else if (dB < -60.0f)                 //  0.0  ..   2.5
		fScale = (dB + 70.0f) * 0.0025f;
	else if (dB < -50.0f)                 //  2.5  ..   7.5
		fScale = (dB + 60.0f) * 0.005f + 0.025f;
	else if (dB < -40.0)                  //  7.5  ..  15.0
		fScale = (dB + 50.0f) * 0.0075f + 0.075f;
	else if (dB < -30.0f)                 // 15.0  ..  30.0
		fScale = (dB + 40.0f) * 0.015f + 0.15f;
	else if (dB < -20.0f)                 // 30.0  ..  50.0
		fScale = (dB + 30.0f) * 0.02f + 0.3f;
	else if (dB < -0.001f || dB > 0.001f) // 50.0  .. 100.0
		fScale = (dB + 20.0f) * 0.025f + 0.5f;

	return fScale;
}

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_filter filter = mlt_frame_pop_audio( frame );
	int iec_scale = mlt_properties_get_int( MLT_FILTER_PROPERTIES(filter), "iec_scale" );
	*format = mlt_audio_s16;
	int error = mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	if ( error || !buffer ) return error;

	int num_channels = *channels;
	int num_samples = *samples > 200 ? 200 : *samples;
	int num_oversample = 0;
	int c, s;
	char key[ 50 ];
	int16_t *pcm = (int16_t*) *buffer;

	for ( c = 0; c < *channels; c++ )
	{
		double val = 0;
		double level = 0.0;

		for ( s = 0; s < num_samples; s++ )
		{
			double sample = fabs( pcm[c + s * num_channels] / 128.0 );
			val += sample;
			if ( sample == 128 )
				num_oversample++;
			else
				num_oversample = 0;
			// 10 samples @max => show max signal
			if ( num_oversample > 10 )
			{
				level = 1.0;
				break;
			}
			// if 3 samples over max => 1 peak over 0 db (0 dB = 40.0)
			if ( num_oversample > 3 )
				level = 41.0/42.0;
		}
		// max amplitude = 40/42, 3to10  oversamples=41, more then 10 oversamples=42
		if ( level == 0.0 && num_samples > 0 )
			level = val / num_samples * 40.0/42.0 / 127.0;
		if ( iec_scale )
			level = IEC_Scale( AMPTODBFS( level ) );
		sprintf( key, "meta.media.audio_level.%d", c );
		mlt_properties_set_double( MLT_FRAME_PROPERTIES( frame ), key, level );
		sprintf( key, "_audio_level.%d", c );
		mlt_properties_set_double( MLT_FILTER_PROPERTIES( filter ), key, level );
		mlt_log_debug( MLT_FILTER_SERVICE( filter ), "channel %d level %f\n", c, level );
	}

	return error;
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

mlt_filter filter_audiolevel_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	if ( filter )
	{
		filter->process = filter_process;
		mlt_properties_set_int( MLT_FILTER_PROPERTIES(filter), "iec_scale", 1 );
	}
	return filter;
}
