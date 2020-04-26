/**
 * \file mlt_audio.c
 * \brief Audio class
 * \see mlt_mlt_audio_s
 *
 * Copyright (C) 2020 Meltytech, LLC
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

#include "mlt_audio.h"

#include "mlt_log.h"

#include <string.h>

/** Set the most common values for the audio.
 *
 * Less common values will be set to reasonable defaults.
 *
 * You should use the \p mlt_sample_calculator to determine the number of samples you want.
 * \public \memberof mlt_audio_s
 * \param self the Audio object
 * \param data the buffer that contains the audio data
 * \param frequency the sample rate
 * \param format the audio format
 * \param samples the number of samples in the data
 * \param channels the number of audio channels
 */

void mlt_audio_set_values( mlt_audio self, void* data, int frequency, mlt_audio_format format, int samples, int channels )
{
	self->data = data;
	self->frequency = frequency;
	self->format = format;
	self->samples = samples;
	self->channels = channels;
	self->layout = mlt_channel_auto;
	self->release_data = NULL;
}

/** Get the most common values for the audio.
 *
 * \public \memberof mlt_audio_s
 * \param self the Audio object
 * \param[out] data the buffer that contains the audio data
 * \param[out] frequency the sample rate
 * \param[out] format the audio format
 * \param[out] samples the number of samples in the data
 * \param[out] channels the number of audio channels
 */

void mlt_audio_get_values( mlt_audio self, void** data, int* frequency, mlt_audio_format* format, int* samples, int* channels )
{
	*data = self->data;
	*frequency = self->frequency;
	*format = self->format;
	*samples = self->samples;
	*channels = self->channels;
}

/** Allocate the data field based on the other properties of the Audio.
 *
 * If the data field is already set, and a destructor function exists, the data
 * will be released. Else, the data pointer will be overwritten without being
 * released.
 *
 * After this function call, the release_data field will be set and can be used
 * to release the data when necessary.
 *
 * \public \memberof mlt_audio_s
 * \param self the Audio object
 */

void mlt_audio_alloc_data( mlt_audio self )
{
	if (!self ) return;

	if ( self->release_data )
	{
		self->release_data( self->data );
	}

	int size = mlt_audio_calculate_size( self );
	self->data = mlt_pool_alloc( size );
	self->release_data = mlt_pool_release;
}

/** Calculate the number of bytes needed for the Audio data.
 *
 * \public \memberof mlt_audio_s
 * \param self the Audio object
 * \return the number of bytes
 */

int mlt_audio_calculate_size( mlt_audio self )
{
	if (!self ) return 0;

	switch ( self->format )
	{
		case mlt_audio_none:   return 0;
		case mlt_audio_s16:    return self->samples * self->channels * sizeof( int16_t );
		case mlt_audio_s32le:
		case mlt_audio_s32:    return self->samples * self->channels * sizeof( int32_t );
		case mlt_audio_f32le:
		case mlt_audio_float:  return self->samples * self->channels * sizeof( float );
		case mlt_audio_u8:     return self->samples * self->channels;
	}
	return 0;
}

/** Get the number of planes for the audio type
 *
 * \public \memberof mlt_audio_s
 * \param self the Audio object
 * \return the number of planes.
 */

int mlt_audio_plane_count( mlt_audio self )
{
	switch ( self->format )
	{
		case mlt_audio_none:  return 0;
		case mlt_audio_s16:   return 1;
		case mlt_audio_s32le: return 1;
		case mlt_audio_s32:   return self->channels;
		case mlt_audio_f32le: return 1;
		case mlt_audio_float: return self->channels;
		case mlt_audio_u8:    return 1;
	}
	return 0;
}

/** Get the size of an audio plane.
 *
 * \public \memberof mlt_audio_s
 * \param self the Audio object
 * \return the size of a plane.
 */

int mlt_audio_plane_size( mlt_audio self )
{
	switch ( self->format )
	{
		case mlt_audio_none:  return 0;
		case mlt_audio_s16:   return self->samples * self->channels * sizeof( int16_t );
		case mlt_audio_s32le: return self->samples * self->channels * sizeof( int32_t );
		case mlt_audio_s32:   return self->samples * sizeof( int32_t );
		case mlt_audio_f32le: return self->samples * self->channels * sizeof( float );
		case mlt_audio_float: return self->samples * sizeof( float );
		case mlt_audio_u8:    return self->samples * self->channels;
	}
	return 0;
}

/** Populate an array of pointers each pointing to the beginning of an audio plane.
 *
 * \public \memberof mlt_audio_s
 * \param self the Audio object
 * \param[out] planes the array of pointers to populate
 */


void mlt_audio_get_planes( mlt_audio self, uint8_t** planes )
{
	int plane_count = mlt_audio_plane_count( self );
	int plane_size = mlt_audio_plane_size( self );
	int p = 0;
	for( p = 0; p < plane_count; p++ )
	{
		planes[p] = (uint8_t*)self->data + ( p * plane_size );
	}
}

/** Shrink the audio to the new number of samples.
 *
 * Existing samples will be moved as necessary to ensure that the audio planes
 * immediately follow each other. The samples field will be updated to match the
 * new number.
 *
 * \public \memberof mlt_audio_s
 * \param self the Audio object
 * \param samples the new number of samples to shrink to
 */

void mlt_audio_shrink( mlt_audio self , int samples )
{
	int plane_count = mlt_audio_plane_count( self );
	if ( samples >= self->samples || samples < 0 )
	{
		// Nothing to do;
	}
	else if ( plane_count == 1 || samples == 0 )
	{
		// No need to move any data.
		self->samples = samples;
	}
	if ( plane_count > 1 )
	{
		// Move data to remove gaps between planes.
		size_t src_plane_size = mlt_audio_plane_size( self );
		self->samples = samples;
		size_t dst_plane_size = mlt_audio_plane_size( self );
		// The first channel will always be in the correct place (0).
		int p = 1;
		for( p = 1; p < plane_count; p++ )
		{
			uint8_t* src = (uint8_t*)self->data + ( p * src_plane_size );
			uint8_t* dst = (uint8_t*)self->data + ( p * dst_plane_size );
			memmove( dst, src, dst_plane_size );
		}
	}
}

/** Reverse the audio samples.
 *
 * \public \memberof mlt_audio_s
 * \param self the Audio object
 */

void mlt_audio_reverse( mlt_audio self )
{
	int c = 0;

	if ( !self || !self->data || self->samples <= 0 ) return;

	switch ( self->format )
	{
		// Interleaved 8bit formats
		case mlt_audio_u8:
		{
			int8_t tmp;
			for ( c = 0; c < self->channels; c++ )
			{
				// Pointer to first sample
				int8_t* a = (int8_t*)self->data + c;
				// Pointer to last sample
				int8_t* b = (int8_t*)self->data + ((self->samples - 1) * self->channels) + c;
				while( a < b )
				{
					tmp = *a;
					*a = *b;
					*b = tmp;
					a += self->channels;
					b -= self->channels;
				}
			}
			break;
		}
		// Interleaved 16bit formats
		case mlt_audio_s16:
		{
			int16_t tmp;
			for ( c = 0; c < self->channels; c++ )
			{
				// Pointer to first sample
				int16_t *a = (int16_t*)self->data + c;
				// Pointer to last sample
				int16_t *b = (int16_t*)self->data + ((self->samples - 1) * self->channels) + c;
				while( a < b )
				{
					tmp = *a;
					*a = *b;
					*b = tmp;
					a += self->channels;
					b -= self->channels;
				}
			}
			break;
		}
		// Interleaved 32bit formats
		case mlt_audio_s32le:
		case mlt_audio_f32le:
		{
			int32_t tmp;
			for ( c = 0; c < self->channels; c++ )
			{
				// Pointer to first sample
				int32_t *a = (int32_t*)self->data + c;
				// Pointer to last sample
				int32_t *b = (int32_t*)self->data + ((self->samples - 1)* self->channels) + c;
				while( a < b )
				{
					tmp = *a;
					*a = *b;
					*b = tmp;
					a += self->channels;
					b -= self->channels;
				}
			}
			break;
		}
		// Planer 32bit formats
		case mlt_audio_s32:
		case mlt_audio_float:
		{
			int32_t tmp;
			for ( c = 0; c < self->channels; c++ )
			{
				// Pointer to first sample
				int32_t *a = (int32_t*)self->data + (c * self->samples);
				// Pointer to last sample
				int32_t *b = (int32_t*)self->data + ((c + 1) * self->samples) - 1;
				while( a < b )
				{
					tmp = *a;
					*a = *b;
					*b = tmp;
					a++;
					b--;
				}
			}
			break;
		}
		case mlt_audio_none:
			break;
	}
}

/** Copy audio samples from src to dst.
 *
 * \public \memberof mlt_frame_s
 * \param dst the destination object
 * \param src the source object
 * \param samples the number of samples to copy
 * \param src_offset the number of samples to skip from the source
 * \param dst_offset the number of samples to skip from the destination
 * \return none
 */

void mlt_audio_copy( mlt_audio dst, mlt_audio src, int samples, int src_start, int dst_start )
{
	if ( ( dst_start + samples ) > dst->samples )
	{
		mlt_log_error( NULL, "mlt_audio_copy: avoid dst buffer overrun\n" );
		return;
	}

	if ( ( src_start + samples ) > src->samples )
	{
		mlt_log_error( NULL, "mlt_audio_copy: avoid src buffer overrun\n" );
		return;
	}

	if ( src->format != dst->format || src->channels != dst->channels )
	{
		mlt_log_error( NULL, "mlt_audio_copy: src/dst mismatch\n" );
		return;
	}

	switch ( src->format )
	{
		case mlt_audio_none:
			mlt_log_error( NULL, "mlt_audio_copy: mlt_audio_none\n" );
			return;
		// Interleaved 8bit formats
		case mlt_audio_u8:
		{
			int8_t* s = (int8_t*)src->data + ( src_start * src->channels );
			int8_t* d = (int8_t*)dst->data + ( dst_start * dst->channels );
			int size = src->channels * samples * sizeof(int8_t);
			memcpy( d, s, size );
			return;
		}
		// Interleaved 16bit formats
		case mlt_audio_s16:
		{
			int16_t* s = (int16_t*)src->data + ( src_start * src->channels );
			int16_t* d = (int16_t*)dst->data + ( dst_start * dst->channels );
			int size = src->channels * samples * sizeof(int16_t);
			memcpy( d, s, size );
			return;
		}
		// Interleaved 32bit formats
		case mlt_audio_s32le:
		case mlt_audio_f32le:
		{
			int32_t* s = (int32_t*)src->data + ( src_start * src->channels );
			int32_t* d = (int32_t*)dst->data + ( dst_start * dst->channels );
			int size = src->channels * samples * sizeof(int32_t);
			memcpy( d, s, size );
			return;
		}
		// Planer 32bit formats
		case mlt_audio_s32:
		case mlt_audio_float:
		{
			int p = 0;
			for ( p = 0; p < src->channels; p++ )
			{
				int32_t* s = (int32_t*)src->data + (p * src->samples) + src_start;
				int32_t* d = (int32_t*)dst->data + (p * dst->samples) + dst_start;
				int size = samples * sizeof(int32_t);
				memcpy( d, s, size );
			}
			return;
		}
	}
}

