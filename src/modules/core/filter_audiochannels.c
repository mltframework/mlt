/*
 * filter_audiochannels.c -- convert from one audio format to another
 * Copyright (C) 2009-2014 Meltytech, LLC
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
			int i, j, k = 0;
			for ( i = 0; i < *samples; i++ )
			{
				for ( j = 0; j < *channels; j++ )
				{
					new_buffer[ ( i * *channels ) + j ] = ((int16_t*)(*buffer))[ ( i * channels_avail ) + k ];
					k = ( k + 1 ) % channels_avail;
				}
			}
		}
		else if ( *format == mlt_audio_s32le || *format == mlt_audio_f32le )
		{
			int32_t *p = (int32_t*) new_buffer;
			int i, j, k = 0;
			for ( i = 0; i < *samples; i++ )
			{
				for ( j = 0; j < *channels; j++ )
				{
					p[ ( i * *channels ) + j ] = ((int32_t*)(*buffer))[ ( i * channels_avail ) + k ];
					k = ( k + 1 ) % channels_avail;
				}
			}
		}
		else if ( *format == mlt_audio_u8 )
		{
			uint8_t *p = (uint8_t*) new_buffer;
			int i, j, k = 0;
			for ( i = 0; i < *samples; i++ )
			{
				for ( j = 0; j < *channels; j++ )
				{
					p[ ( i * *channels ) + j ] = ((uint8_t*)(*buffer))[ ( i * channels_avail ) + k ];
					k = ( k + 1 ) % channels_avail;
				}
			}
		}
		else
		{
			// non-interleaved - s32 or float
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
	else if ( channels_avail == 6 && *channels == 2 )
	{
		// Downmix 5.1 audio to stereo.
		// Mix levels taken from ATSC A/52 assuming maximum center and surround
		// mix levels.
		#define MIX(front, center, surr) (front + (0.707 * center) + (0.5 * surr))

		// Convert to a supported format if necessary
		mlt_audio_format new_format = *format;
		switch( *format )
		{
		default:
			// Unknown. Try to convert to float anyway.
			mlt_log_error( NULL, "[audiochannels] Unknown format %d\n", *format );
		case mlt_audio_float:
		case mlt_audio_f32le:
			new_format = mlt_audio_float;
			break;
		case mlt_audio_s32le:
		case mlt_audio_s32:
			new_format = mlt_audio_s32;
			break;
		case mlt_audio_s16:
		case mlt_audio_u8:
			new_format = mlt_audio_s16;
			break;
		case mlt_audio_none:
			new_format = mlt_audio_none;
			break;
		}
		if ( *format != new_format && frame->convert_audio )
			frame->convert_audio( frame, buffer, format, new_format );

		// Perform the downmix. Operate on the buffer in place to avoid realloc.
		if ( *format == mlt_audio_s16 )
		{
			int16_t* in = *buffer;
			int16_t* out = *buffer;
			int i;
			for ( i = 0; i < *samples; i++ )
			{
				float fl = in[0];
				float fr = in[1];
				float c = in[2];
				// in[3] is LFE
				float sl = in[4];
				float sr = in[5];
				*out++ = CLAMP( MIX(fl, c, sl), INT16_MIN, INT16_MAX ); // Left
				*out++ = CLAMP( MIX(fr, c, sr), INT16_MIN, INT16_MAX ); // Right
				in +=6;
			}
		}
		else if ( *format == mlt_audio_s32 )
		{
			int32_t* flin = *buffer;
			int32_t* frin = *buffer + (*samples * sizeof(float));
			int32_t* cin  = *buffer + (2 * *samples * sizeof(float));
			int32_t* slin = *buffer + (4 * *samples * sizeof(float));
			int32_t* srin = *buffer + (5 * *samples * sizeof(float));
			int32_t* lout = *buffer;
			int32_t* rout = *buffer + (*samples * sizeof(float));
			int i;
			for ( i = 0; i < *samples; i++ )
			{
				double fl = *flin++;
				double fr = *frin++;
				double c = *cin++;
				double sl = *slin++;
				double sr = *srin++;
				*lout++ = CLAMP( MIX(fl, c, sl), INT32_MIN, INT32_MAX );
				*rout++ = CLAMP( MIX(fr, c, sr), INT32_MIN, INT32_MAX );
			}
		}
		else if ( *format == mlt_audio_float )
		{
			float* flin = *buffer;
			float* frin = *buffer + (*samples * sizeof(float));
			float* cin  = *buffer + (2 * *samples * sizeof(float));
			float* slin = *buffer + (4 * *samples * sizeof(float));
			float* srin = *buffer + (5 * *samples * sizeof(float));
			float* lout = *buffer;
			float* rout = *buffer + (*samples * sizeof(float));
			int i;
			for ( i = 0; i < *samples; i++ )
			{
				float fl = *flin++;
				float fr = *frin++;
				float c = *cin++;
				float sl = *slin++;
				float sr = *srin++;
				*lout++ = MIX(fl, c, sl);
				*rout++ = MIX(fr, c, sr);
			}
		}
		else
		{
			mlt_log_error( NULL, "[audiochannels] Unable to mix format %d\n", *format );
		}
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
