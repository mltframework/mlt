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

// Mix levels taken from ATSC A/52 assuming maximum center and surround mix levels.
#define C_MIX 0.70710678118654752440  // 1/sqrt(2)
#define S_MIX 0.5

enum Channel
{
	CHAN_LF  = 0,
	CHAN_RF  = 1,
	CHAN_C   = 2,
	CHAN_LFE = 3,
	CHAN_LS  = 4,
	CHAN_RS  = 5,
	CHAN_LB  = 6,
	CHAN_RB  = 7,
	CHAN_MAX = 8,
};

enum ChannelMask
{
	CHAN_MASK_LF  = 1 << CHAN_LF,
	CHAN_MASK_RF  = 1 << CHAN_RF,
	CHAN_MASK_C   = 1 << CHAN_C,
	CHAN_MASK_LFE = 1 << CHAN_LFE,
	CHAN_MASK_LS  = 1 << CHAN_LS,
	CHAN_MASK_RS  = 1 << CHAN_RS,
	CHAN_MASK_LB  = 1 << CHAN_LB,
	CHAN_MASK_RB  = 1 << CHAN_RB,
};

enum ChannelConfig
{
	CHAN_CFG_NONE    = 0,
	CHAN_CFG_MONO    = CHAN_MASK_C,
	CHAN_CFG_STEREO  = CHAN_MASK_LF | CHAN_MASK_RF,
	CHAN_CFG_3_0     = CHAN_MASK_LF | CHAN_MASK_RF | CHAN_MASK_C,
	CHAN_CFG_4_0     = CHAN_MASK_LF | CHAN_MASK_RF | CHAN_MASK_LS | CHAN_MASK_RS,
	CHAN_CFG_5_0     = CHAN_MASK_LF | CHAN_MASK_RF | CHAN_MASK_C | CHAN_MASK_LS | CHAN_MASK_RS,
	CHAN_CFG_5_1     = CHAN_MASK_LF | CHAN_MASK_RF | CHAN_MASK_C | CHAN_MASK_LFE | CHAN_MASK_LS | CHAN_MASK_RS,
	CHAN_CFG_7_0     = CHAN_MASK_LF | CHAN_MASK_RF | CHAN_MASK_C | CHAN_MASK_LS | CHAN_MASK_RS | CHAN_MASK_LB | CHAN_MASK_RB,
	CHAN_CFG_7_1     = CHAN_MASK_LF | CHAN_MASK_RF | CHAN_MASK_C | CHAN_MASK_LFE | CHAN_MASK_LS | CHAN_MASK_RS | CHAN_MASK_LB | CHAN_MASK_RB,
};

// Convert number of channels to a channel configuration
static int CONFIG_LOOKUP[] =
{
	CHAN_CFG_NONE,
	CHAN_CFG_MONO,
	CHAN_CFG_STEREO,
	CHAN_CFG_3_0,
	CHAN_CFG_4_0,
	CHAN_CFG_5_0,
	CHAN_CFG_5_1,
	CHAN_CFG_7_0,
	CHAN_CFG_7_1,
};

static void move_row( double matrix[CHAN_MAX][CHAN_MAX], int src, int dst )
{
	int i;
	for ( i = 0; i < CHAN_MAX; i++ )
		matrix[dst][i] = matrix[src][i];
}

static void move_column( double matrix[CHAN_MAX][CHAN_MAX], int src, int dst )
{
	int i;
	for ( i = 0; i < CHAN_MAX; i++ )
		matrix[i][dst] = matrix[i][src];
}

static void mix_channels( void* in_buffer, void* out_buffer, int in_channels, int out_channels, int samples, int format )
{
	// Coefficients indexed by [out][in]
	double coeffs[CHAN_MAX][CHAN_MAX] = {{0}};
	int in_config = CONFIG_LOOKUP[in_channels];
	int out_config = CONFIG_LOOKUP[out_channels];
	int i = 0;
	int o = 0;
	int c = 0;

	if ( out_config == CHAN_CFG_MONO )
	{
		coeffs[CHAN_C][CHAN_LF] = 1.0;
		coeffs[CHAN_C][CHAN_RF] = 1.0;
		coeffs[CHAN_C][CHAN_C]  = C_MIX;
		coeffs[CHAN_C][CHAN_LFE]  = 0.0;
		coeffs[CHAN_C][CHAN_LS] = S_MIX;
		coeffs[CHAN_C][CHAN_RS] = S_MIX;
		coeffs[CHAN_C][CHAN_LB] = S_MIX;
		coeffs[CHAN_C][CHAN_RB] = S_MIX;
	}
	else
	{
		// Set up default channel mix levels
		coeffs[CHAN_LF][CHAN_LF] = 1.0;
		coeffs[CHAN_RF][CHAN_RF] = 1.0;
		coeffs[CHAN_C][CHAN_C] = 1.0;
		coeffs[CHAN_LFE][CHAN_LFE] = 1.0;
		coeffs[CHAN_LS][CHAN_LS] = 1.0;
		coeffs[CHAN_RS][CHAN_RS] = 1.0;
		coeffs[CHAN_LB][CHAN_LB] = 1.0;
		coeffs[CHAN_RB][CHAN_RB] = 1.0;

		if ( !( out_config & CHAN_MASK_C ) )
		{
			// No center channel - mix center into LF & RF
			coeffs[CHAN_LF][CHAN_C] = C_MIX;
			coeffs[CHAN_RF][CHAN_C] = C_MIX;
		}

		if ( !( out_config & CHAN_MASK_LS ) )
		{
			// No surround channels - mix surround & back into LF & RF
			coeffs[CHAN_LF][CHAN_LS] = S_MIX;
			coeffs[CHAN_RF][CHAN_RS] = S_MIX;
			coeffs[CHAN_LF][CHAN_LB] = S_MIX;
			coeffs[CHAN_RF][CHAN_RB] = S_MIX;
		}

		if ( !( out_config & CHAN_MASK_LB ) )
		{
			// No back channels - mix back into LF & RF
			coeffs[CHAN_LS][CHAN_LB] = S_MIX;
			coeffs[CHAN_RS][CHAN_RB] = S_MIX;
		}
	}

	// Reduce the coefficients to remove missing input channels
	i = 0;
	c = 0;
	for ( int pos = CHAN_MASK_LF; pos <= CHAN_MASK_RB; pos = pos << 1 )
	{
		if ( in_config & pos )
		{
			if ( c != i )
				move_column( coeffs, c, i );
			i++;
			if ( i == in_channels )
				break;
		}
		c++;
	}
	// Reduce the coefficients to remove missing output channels
	o = 0;
	c = 0;
	for ( int pos = CHAN_MASK_LF; pos <= CHAN_MASK_RB; pos = pos << 1 )
	{
		if ( out_config & pos )
		{
			if ( c != o )
				move_row( coeffs, c, o );
			o++;
			if ( o == out_channels )
				break;
		}
		c++;
	}

	// Normalize the coefficients so that the total gain never exceeds 1.0
	float max_gain = 0.0;
	for ( o = 0; o < out_channels; o++ )
	{
		float gain = 0.0;
		for ( i = 0; i < in_channels; i++ )
			gain += coeffs[o][i];
		if ( gain > max_gain )
			max_gain = gain;
	}
	if ( max_gain > 1.0 )
	{
		for ( o = 0; o < out_channels; o++ )
			for ( i = 0; i < in_channels; i++ )
				coeffs[o][i] /= max_gain;
	}

	// Apply the coefficients to mix the channels.
	if ( format == mlt_audio_s16 )
	{
		int s = 0;
		int16_t *po = (int16_t*) out_buffer;
		int16_t *pi = (int16_t*) in_buffer;
		for ( s = 0; s < samples; s++ )
		{
			for ( o = 0; o < out_channels; o++ )
			{
				float value = 0.0;
				for ( i = 0; i < in_channels; i++ )
					value += (float)*(pi + i) * coeffs[o][i];
				*po++ = CLAMP( value, INT16_MIN, INT16_MAX );
			}
			pi += in_channels;
		}
	}
	else if ( format == mlt_audio_s32 )
	{
		int s = 0;
		for ( s = 0; s < samples; s++ )
		{
			int32_t* po = (int32_t*)out_buffer + s;
			for ( o = 0; o < out_channels; o++ )
			{
				float value = 0.0;
				int32_t* pi = (int32_t*)in_buffer + s;
				for ( i = 0; i < in_channels; i++ )
				{
					value += (float)*pi * coeffs[o][i];
					pi += samples;
				}
				*po = CLAMP( value, INT32_MIN, INT32_MAX );
				po += samples;
			}
		}
	}
	else if ( format == mlt_audio_float )
	{
		int s = 0;
		for ( s = 0; s < samples; s++ )
		{
			float* po = (float*)out_buffer + s;
			for ( o = 0; o < out_channels; o++ )
			{
				*po = 0.0;
				float* pi = (float*)in_buffer + s;
				for ( i = 0; i < in_channels; i++ )
				{
					*po += *pi * coeffs[o][i];
					pi += samples;
				}
				po += samples;
			}
		}
	}
	else if ( format == mlt_audio_s32le )
	{
		int s = 0;
		int32_t *po = (int32_t*) out_buffer;
		int32_t *pi = (int32_t*) in_buffer;
		for ( s = 0; s < samples; s++ )
		{
			for ( o = 0; o < out_channels; o++ )
			{
				float value = 0.0;
				for ( i = 0; i < in_channels; i++ )
					value += (float)*(pi + i) * coeffs[o][i];
				*po++ = CLAMP( value, INT32_MIN, INT32_MAX );
			}
			pi += in_channels;
		}
	}
	else if ( format == mlt_audio_f32le )
	{
		int s = 0;
		float *po = (float*) out_buffer;
		float *pi = (float*) in_buffer;
		for ( s = 0; s < samples; s++ )
		{
			for ( o = 0; o < out_channels; o++ )
			{
				*po = 0.0;
				for ( i = 0; i < in_channels; i++ )
					*po += *(pi + i) * coeffs[o][i];
				po++;
			}
			pi += in_channels;
		}
	}
	else if ( format == mlt_audio_u8 )
	{
		int s = 0;
		uint8_t *po = (uint8_t*) out_buffer;
		uint8_t *pi = (uint8_t*) in_buffer;
		for ( s = 0; s < samples; s++ )
		{
			for ( o = 0; o < out_channels; o++ )
			{
				float value = 0.0;
				for ( i = 0; i < in_channels; i++ )
					value += (float)*(pi + i) * coeffs[o][i];
				*po++ = CLAMP( value, 0, UINT8_MAX );
			}
			pi += in_channels;
		}
	}
}

static void dup_drop_channels( void* in_buffer, void* out_buffer, int in_channels, int out_channels, int samples, int format )
{
	if ( in_channels < out_channels )
	{
		// Duplicate the existing channels
		if ( format == mlt_audio_s16 )
		{
			int16_t *p = (int16_t*) out_buffer;
			int i, j, k = 0;
			for ( i = 0; i < samples; i++ )
			{
				for ( j = 0; j < out_channels; j++ )
				{
					p[ ( i * out_channels ) + j ] = ((int16_t*)(in_buffer))[ ( i * in_channels ) + k ];
					k = ( k + 1 ) % in_channels;
				}
			}
		}
		else if ( format == mlt_audio_s32le || format == mlt_audio_f32le )
		{
			int32_t *p = (int32_t*) out_buffer;
			int i, j, k = 0;
			for ( i = 0; i < samples; i++ )
			{
				for ( j = 0; j < out_channels; j++ )
				{
					p[ ( i * out_channels ) + j ] = ((int32_t*)(in_buffer))[ ( i * in_channels ) + k ];
					k = ( k + 1 ) % in_channels;
				}
			}
		}
		else if ( format == mlt_audio_u8 )
		{
			uint8_t *p = (uint8_t*) out_buffer;
			int i, j, k = 0;
			for ( i = 0; i < samples; i++ )
			{
				for ( j = 0; j < out_channels; j++ )
				{
					p[ ( i * out_channels ) + j ] = ((uint8_t*)(in_buffer))[ ( i * in_channels ) + k ];
					k = ( k + 1 ) % in_channels;
				}
			}
		}
		else
		{
			// non-interleaved - s32 or float
			int size_avail = mlt_audio_format_size( format, samples, in_channels );
			int32_t *p = (int32_t*) out_buffer;
			int i = out_channels / in_channels;
			while ( i-- )
			{
				memcpy( p, in_buffer, size_avail );
				p += size_avail / sizeof(*p);
			}
			i = out_channels % in_channels;
			if ( i )
			{
				size_avail = mlt_audio_format_size( format, samples, i );
				memcpy( p, in_buffer, size_avail );
			}
		}
	}
	else if ( in_channels > out_channels )
	{
		int i, j;

		// Drop all but the first out_channels
		if ( format == mlt_audio_s16 )
		{
			int16_t *p = (int16_t*) out_buffer;
			for ( i = 0; i < samples; i++ )
				for ( j = 0; j < out_channels; j++ )
					p[ ( i * out_channels ) + j ]	= ((int16_t*)(in_buffer))[ ( i * in_channels ) + j ];
		}
		else if ( format == mlt_audio_s32le || format == mlt_audio_f32le )
		{
			int32_t *p = (int32_t*) out_buffer;
			for ( i = 0; i < samples; i++ )
				for ( j = 0; j < out_channels; j++ )
					p[ ( i * out_channels ) + j ]	= ((int32_t*)(in_buffer))[ ( i * in_channels ) + j ];
		}
		else if ( format == mlt_audio_u8 )
		{
			uint8_t *p = (uint8_t*) out_buffer;
			for ( i = 0; i < samples; i++ )
				for ( j = 0; j < out_channels; j++ )
					p[ ( i * out_channels ) + j ]	= ((uint8_t*)(in_buffer))[ ( i * in_channels ) + j ];
		}
		else
		{
			// non-interleaved - s32 or float
			memcpy( out_buffer, in_buffer, mlt_audio_format_size( format, samples, out_channels ) );
		}
	}
}

static int filter_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Used to return number of channels in the source
	int channels_avail = *channels;

	// Get the producer's audio
	int error = mlt_frame_get_audio( frame, buffer, format, frequency, &channels_avail, samples );
	if ( error ) return error;

	if ( channels_avail == *channels )
	{
		return error;
	}

	int size = mlt_audio_format_size( *format, *samples, *channels );
	float *new_buffer = mlt_pool_alloc( size );
	char *convert = mlt_properties_get( MLT_FRAME_PROPERTIES( frame ), "audiochannel.convert" );
	if ( channels_avail > 8 || ( convert && !strcmp( convert, "dupdrop" ) ) )
	{
		dup_drop_channels( *buffer, new_buffer, channels_avail, *channels, *samples, *format );
	}
	else
	{
		mix_channels( *buffer, new_buffer, channels_avail, *channels, *samples, *format );
	}

	mlt_frame_set_audio( frame, new_buffer, *format, size, mlt_pool_release );
	*buffer = new_buffer;
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
