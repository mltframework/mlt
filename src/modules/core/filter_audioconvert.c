/*
 * filter_audioconvert.c -- convert from one audio format to another
 * Copyright (C) 2009-2016 Meltytech, LLC
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

static int convert_audio( mlt_frame frame, void **audio, mlt_audio_format *format, mlt_audio_format requested_format )
{
	int error = 1;
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	int channels = mlt_properties_get_int( properties, "audio_channels" );
	int samples = mlt_properties_get_int( properties, "audio_samples" );
	int size = mlt_audio_format_size( requested_format, samples, channels );

	if ( *format != requested_format )
	{
		mlt_log_debug( NULL, "[filter audioconvert] %s -> %s %d channels %d samples\n",
			mlt_audio_format_name( *format ), mlt_audio_format_name( requested_format ),
			channels, samples );
		switch ( *format )
		{
		case mlt_audio_s16:
			switch ( requested_format )
			{
			case mlt_audio_s32:
			{
				int32_t *buffer = mlt_pool_alloc( size );
				int32_t *p = buffer;
				int c;
				for ( c = 0; c < channels; c++ )
				{
					int16_t *q = (int16_t*) *audio + c;
					int i = samples + 1;
					while ( --i )
					{
						*p++ = (int32_t) *q << 16;
						q += channels;
					}
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_float:
			{
				float *buffer = mlt_pool_alloc( size );
				float *p = buffer;
				int c;
				for ( c = 0; c < channels; c++ )
				{
					int16_t *q = (int16_t*) *audio + c;
					int i = samples + 1;
					while ( --i )
					{
						*p++ = (float)( *q ) / 32768.0;
						q += channels;
					}
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_s32le:
			{
				int32_t *buffer = mlt_pool_alloc( size );
				int32_t *p = buffer;
				int16_t *q = (int16_t*) *audio;
				int i = samples * channels + 1;
				while ( --i )
					*p++ = (int32_t) *q++ << 16;
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_f32le:
			{
				float *buffer = mlt_pool_alloc( size );
				float *p = buffer;
				int16_t *q = (int16_t*) *audio;
				int i = samples * channels + 1;
				while ( --i )
				{
					float f = (float)( *q++ ) / 32768.0;
					*p++ = CLAMP( f, -1.0f, 1.0f );
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_u8:
			{
				uint8_t *buffer = mlt_pool_alloc( size );
				uint8_t *p = buffer;
				int16_t *q = (int16_t*) *audio;
				int i = samples * channels + 1;
				while ( --i )
					*p++ = ( *q++ >> 8 ) + 128;
				*audio = buffer;
				error = 0;
				break;
			}
			default:
				break;
			}
			break;
		case mlt_audio_s32:
			switch ( requested_format )
			{
			case mlt_audio_s16:
			{
				int16_t *buffer = mlt_pool_alloc( size );
				int16_t *p = buffer;
				int32_t *q = (int32_t*) *audio;
				int s, c;
				for ( s = 0; s < samples; s++ )
					for ( c = 0; c < channels; c++ )
						*p++ = *( q + c * samples + s ) >> 16;
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_float:
			{
				float *buffer = mlt_pool_alloc( size );
				float *p = buffer;
				int32_t *q = (int32_t*) *audio;
				int i = samples * channels + 1;
				while ( --i )
					*p++ = (float)( *q++ ) / 2147483648.0;
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_s32le:
			{
				int32_t *buffer = mlt_pool_alloc( size );
				int32_t *p = buffer;
				int32_t *q = (int32_t*) *audio;
				int s, c;
				for ( s = 0; s < samples; s++ )
					for ( c = 0; c < channels; c++ )
						*p++ = *( q + c * samples + s );
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_f32le:
			{
				float *buffer = mlt_pool_alloc( size );
				float *p = buffer;
				int32_t *q = (int32_t*) *audio;
				int s, c;
				for ( s = 0; s < samples; s++ )
					for ( c = 0; c < channels; c++ )
					{
						float f = (float)( *( q + c * samples + s ) ) / 2147483648.0;
						*p++ = CLAMP( f, -1.0f, 1.0f );
					}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_u8:
			{
				uint8_t *buffer = mlt_pool_alloc( size );
				uint8_t *p = buffer;
				int32_t *q = (int32_t*) *audio;
				int s, c;
				for ( s = 0; s < samples; s++ )
					for ( c = 0; c < channels; c++ )
						*p++ = ( q[c * samples + s] >> 24 ) + 128;
				*audio = buffer;
				error = 0;
				break;
			}
			default:
				break;
			}
			break;
		case mlt_audio_float:
			switch ( requested_format )
			{
			case mlt_audio_s16:
			{
				int16_t *buffer = mlt_pool_alloc( size );
				int16_t *p = buffer;
				float *q = (float*) *audio;
				int s, c;
				for ( s = 0; s < samples; s++ )
					for ( c = 0; c < channels; c++ )
					{
						float f = *( q + c * samples + s );
						f = CLAMP( f, -1.0f, 1.0f );
						*p++ = 32767 * f;
					}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_s32:
			{
				int32_t *buffer = mlt_pool_alloc( size );
				int32_t *p = buffer;
				float *q = (float*) *audio;
				int i = samples * channels + 1;
				while ( --i )
				{
					float f = *q++;
					f = CLAMP( f, -1.0f, 1.0f );
					int64_t pcm = ( f > 0.0f ? 2147483647LL : 2147483648LL ) * f;
					*p++ = CLAMP( pcm, -2147483648LL, 2147483647LL );
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_s32le:
			{
				int32_t *buffer = mlt_pool_alloc( size );
				int32_t *p = buffer;
				float *q = (float*) *audio;
				int s, c;
				for ( s = 0; s < samples; s++ )
					for ( c = 0; c < channels; c++ )
					{
						float f = *( q + c * samples + s );
						f = CLAMP( f, -1.0f, 1.0f );
						int64_t pcm = ( f > 0.0f ? 2147483647LL : 2147483648LL ) * f;
						*p++ = CLAMP( pcm, -2147483648LL, 2147483647LL );
					}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_f32le:
			{
				float *buffer = mlt_pool_alloc( size );
				float *p = buffer;
				float *q = (float*) *audio;
				int s, c;
				for ( s = 0; s < samples; s++ )
					for ( c = 0; c < channels; c++ )
						*p++ = *( q + c * samples + s );
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_u8:
			{
				uint8_t *buffer = mlt_pool_alloc( size );
				uint8_t *p = buffer;
				float *q = (float*) *audio;
				int s, c;
				for ( s = 0; s < samples; s++ )
					for ( c = 0; c < channels; c++ )
					{
						float f = *( q + c * samples + s );
						f = CLAMP( f, -1.0f, 1.0f );
						*p++ = ( 127 * f ) + 128;
					}
				*audio = buffer;
				error = 0;
				break;
			}
			default:
				break;
			}
			break;
		case mlt_audio_s32le:
			switch ( requested_format )
			{
			case mlt_audio_s16:
			{
				int16_t *buffer = mlt_pool_alloc( size );
				int16_t *p = buffer;
				int32_t *q = (int32_t*) *audio;
				int i = samples * channels + 1;
				while ( --i )
					*p++ = *q++ >> 16;
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_s32:
			{
				int32_t *buffer = mlt_pool_alloc( size );
				int32_t *p = buffer;
				int c;
				for ( c = 0; c < channels; c++ )
				{
					int32_t *q = (int32_t*) *audio + c;
					int i = samples + 1;
					while ( --i )
					{
						*p++ = *q;
						q += channels;
					}
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_float:
			{
				float *buffer = mlt_pool_alloc( size );
				float *p = buffer;
				int c;
				for ( c = 0; c < channels; c++ )
				{
					int32_t *q = (int32_t*) *audio + c;
					int i = samples + 1;
					while ( --i )
					{
						*p++ = (float)( *q ) / 2147483648.0;
						q += channels;
					}
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_f32le:
			{
				float *buffer = mlt_pool_alloc( size );
				float *p = buffer;
				int32_t *q = (int32_t*) *audio;
				int i = samples * channels + 1;
				while ( --i )
					*p++ = (float)( *q++ ) / 2147483648.0;
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_u8:
			{
				uint8_t *buffer = mlt_pool_alloc( size );
				uint8_t *p = buffer;
				int32_t *q = (int32_t*) *audio;
				int i = samples * channels + 1;
				while ( --i )
					*p++ = ( *q++ >> 24 ) + 128;
				*audio = buffer;
				error = 0;
				break;
			}
			default:
				break;
			}
			break;
		case mlt_audio_f32le:
			switch ( requested_format )
			{
			case mlt_audio_s16:
			{
				int16_t *buffer = mlt_pool_alloc( size );
				int16_t *p = buffer;
				float *q = (float*) *audio;
				int i = samples * channels + 1;
				while ( --i )
				{
					float f = *q++;
					f = CLAMP( f, -1.0f , 1.0f );
					*p++ = 32767 * f;
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_float:
			{
				float *buffer = mlt_pool_alloc( size );
				float *p = buffer;
				int c;
				for ( c = 0; c < channels; c++ )
				{
					float *q = (float*) *audio + c;
					int i = samples + 1;
					while ( --i )
					{
						*p++ = *q;
						q += channels;
					}
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_s32:
			{
				int32_t *buffer = mlt_pool_alloc( size );
				int32_t *p = buffer;
				int c;
				for ( c = 0; c < channels; c++ )
				{
					float *q = (float*) *audio + c;
					int i = samples + 1;
					while ( --i )
					{
						float f = *q;
						f = CLAMP( f, -1.0f , 1.0f );
						int64_t pcm = ( f > 0.0f ? 2147483647LL : 2147483648LL ) * f;
						*p++ = CLAMP( pcm, -2147483648LL, 2147483647LL );
						q += channels;
					}
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_s32le:
			{
				int32_t *buffer = mlt_pool_alloc( size );
				int32_t *p = buffer;
				float *q = (float*) *audio;
				int i = samples * channels + 1;
				while ( --i )
				{
					float f = *q++;
					f = CLAMP( f, -1.0f , 1.0f );
					int64_t pcm = ( f > 0.0f ? 2147483647LL : 2147483648LL ) * f;
					*p++ = CLAMP( pcm, -2147483648LL, 2147483647LL );
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_u8:
			{
				uint8_t *buffer = mlt_pool_alloc( size );
				uint8_t *p = buffer;
				float *q = (float*) *audio;
				int i = samples * channels + 1;
				while ( --i )
				{
					float f = *q++;
					f = CLAMP( f, -1.0f , 1.0f );
					*p++ = ( 127 * f ) + 128;
				}
				*audio = buffer;
				error = 0;
				break;
			}
			default:
				break;
			}
			break;
		case mlt_audio_u8:
			switch ( requested_format )
			{
			case mlt_audio_s32:
			{
				int32_t *buffer = mlt_pool_alloc( size );
				int32_t *p = buffer;
				int c;
				for ( c = 0; c < channels; c++ )
				{
					uint8_t *q = (uint8_t*) *audio + c;
					int i = samples + 1;
					while ( --i )
					{
						*p++ = ( (int32_t) *q - 128 ) << 24;
						q += channels;
					}
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_float:
			{
				float *buffer = mlt_pool_alloc( size );
				float *p = buffer;
				int c;
				for ( c = 0; c < channels; c++ )
				{
					uint8_t *q = (uint8_t*) *audio + c;
					int i = samples + 1;
					while ( --i )
					{
						*p++ = ( (float) *q - 128 ) / 256.0f;
						q += channels;
					}
				}
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_s16:
			{
				int16_t *buffer = mlt_pool_alloc( size );
				int16_t *p = buffer;
				uint8_t *q = (uint8_t*) *audio;
				int i = samples * channels + 1;
				while ( --i )
					*p++ = ( (int16_t) *q++ - 128 ) << 8;
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_s32le:
			{
				int32_t *buffer = mlt_pool_alloc( size );
				int32_t *p = buffer;
				uint8_t *q = (uint8_t*) *audio;
				int i = samples * channels + 1;
				while ( --i )
					*p++ = ( (int32_t) *q++ - 128 ) << 24;
				*audio = buffer;
				error = 0;
				break;
			}
			case mlt_audio_f32le:
			{
				float *buffer = mlt_pool_alloc( size );
				float *p = buffer;
				uint8_t *q = (uint8_t*) *audio;
				int i = samples * channels + 1;
				while ( --i )
				{
					float f = ( (float) *q++ - 128 ) / 256.0f;
					*p++ = CLAMP( f, -1.0f, 1.0f );
				}
				*audio = buffer;
				error = 0;
				break;
			}
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	if ( !error )
	{
		mlt_frame_set_audio( frame, *audio, requested_format, size, mlt_pool_release );
		*format = requested_format;
	}
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	frame->convert_audio = convert_audio;
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_audioconvert_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = calloc( 1, sizeof( struct mlt_filter_s ) );
	if ( mlt_filter_init( filter, filter ) == 0 )
		filter->process = filter_process;
	return filter;
}

