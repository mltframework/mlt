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

#include <stdlib.h>
#include <string.h>

/** Allocate a new Audio object.
 *
 * \return a new audio object with default values set
 */

mlt_audio mlt_audio_new()
{
	mlt_audio self = calloc( 1, sizeof(struct mlt_audio_s) );
	self->close = free;
	return self;
}

/** Destroy an audio object created by mlt_audio_new().
 *
 * \public \memberof mlt_audio_s
 * \param self the Audio object
 */

void mlt_audio_close( mlt_audio self )
{
	if ( self)
	{
		if ( self->release_data )
		{
			self->release_data( self->data );
		}
		if ( self->close )
		{
			self->close( self );
		}
	}
}

/** Set the most common values for the audio.
 *
 * Less common values will be set to reasonable defaults.
 *
 * You should use the \p mlt_audio_calculate_frame_samples to determine the number of samples you want.
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
	self->close = NULL;
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
	if ( !self ) return;

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
	if ( !self ) return 0;
	return mlt_audio_format_size( self->format, self->samples, self->channels );
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
			memmove( d, s, size );
			return;
		}
		// Interleaved 16bit formats
		case mlt_audio_s16:
		{
			int16_t* s = (int16_t*)src->data + ( src_start * src->channels );
			int16_t* d = (int16_t*)dst->data + ( dst_start * dst->channels );
			int size = src->channels * samples * sizeof(int16_t);
			memmove( d, s, size );
			return;
		}
		// Interleaved 32bit formats
		case mlt_audio_s32le:
		case mlt_audio_f32le:
		{
			int32_t* s = (int32_t*)src->data + ( src_start * src->channels );
			int32_t* d = (int32_t*)dst->data + ( dst_start * dst->channels );
			int size = src->channels * samples * sizeof(int32_t);
			memmove( d, s, size );
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
				memmove( d, s, size );
			}
			return;
		}
	}
}

/** Determine the number of samples that belong in a frame at a time position.
 *
 * \public \memberof mlt_frame_s
 * \param fps the frame rate
 * \param frequency the sample rate
 * \param position the time position
 * \return the number of samples per channel
 */

int mlt_audio_calculate_frame_samples( float fps, int frequency, int64_t position )
{
	/* Compute the cumulative number of samples until the start of this frame and the
	cumulative number of samples until the start of the next frame. Round each to the
	nearest integer and take the difference to determine the number of samples in
	this frame.

	This approach should prevent rounding errors that can accumulate over a large number
	of frames causing A/V sync problems. */
	return mlt_audio_calculate_samples_to_position( fps, frequency, position + 1 )
		 - mlt_audio_calculate_samples_to_position( fps, frequency, position );
}

/** Determine the number of samples that belong before a time position.
 *
 * \public \memberof mlt_frame_s
 * \param fps the frame rate
 * \param frequency the sample rate
 * \param position the time position
 * \return the number of samples per channel
 */

int64_t mlt_audio_calculate_samples_to_position( float fps, int frequency, int64_t position )
{
	int64_t samples = 0;

	if ( fps )
	{
		samples = (int64_t)( (double) position * (double) frequency / (double) fps +
			( position < 0 ? -0.5 : 0.5 ) );
	}

	return samples;
}

/** Get the short name for an audio format.
 *
 * You do not need to deallocate the returned string.
 * \public \memberof mlt_frame_s
 * \param format an audio format enum
 * \return a string for the name of the image format
 */

const char * mlt_audio_format_name( mlt_audio_format format )
{
	switch ( format )
	{
		case mlt_audio_none:   return "none";
		case mlt_audio_s16:    return "s16";
		case mlt_audio_s32:    return "s32";
		case mlt_audio_s32le:  return "s32le";
		case mlt_audio_float:  return "float";
		case mlt_audio_f32le:  return "f32le";
		case mlt_audio_u8:     return "u8";
	}
	return "invalid";
}

/** Get the amount of bytes needed for a block of audio.
  *
  * \public \memberof mlt_frame_s
  * \param format an audio format enum
  * \param samples the number of samples per channel
  * \param channels the number of channels
  * \return the number of bytes
  */

int mlt_audio_format_size( mlt_audio_format format, int samples, int channels )
{
	switch ( format )
	{
		case mlt_audio_none:   return 0;
		case mlt_audio_s16:    return samples * channels * sizeof( int16_t );
		case mlt_audio_s32le:
		case mlt_audio_s32:    return samples * channels * sizeof( int32_t );
		case mlt_audio_f32le:
		case mlt_audio_float:  return samples * channels * sizeof( float );
		case mlt_audio_u8:     return samples * channels;
	}
	return 0;
}

/** Get the short name for a channel layout.
 *
 * You do not need to deallocate the returned string.
 * \public \member of mlt_frame_s
 * \param layout the channel layout
 * \return a string for the name of the channel layout
 */

const char* mlt_audio_channel_layout_name( mlt_channel_layout layout )
{
	switch ( layout )
	{
		case mlt_channel_auto:           return "auto";
		case mlt_channel_independent:    return "independent";
		case mlt_channel_mono:           return "mono";
		case mlt_channel_stereo:         return "stereo";
		case mlt_channel_2p1:            return "2.1";
		case mlt_channel_3p0:            return "3.0";
		case mlt_channel_3p0_back:       return "3.0(back)";
		case mlt_channel_4p0:            return "4.0";
		case mlt_channel_quad_back:      return "quad";
		case mlt_channel_quad_side:      return "quad(side)";
		case mlt_channel_3p1:            return "3.1";
		case mlt_channel_5p0_back:       return "5.0";
		case mlt_channel_5p0:            return "5.0(side)";
		case mlt_channel_4p1:            return "4.1";
		case mlt_channel_5p1_back:       return "5.1";
		case mlt_channel_5p1:            return "5.1(side)";
		case mlt_channel_6p0:            return "6.0";
		case mlt_channel_6p0_front:      return "6.0(front)";
		case mlt_channel_hexagonal:      return "hexagonal";
		case mlt_channel_6p1:            return "6.1";
		case mlt_channel_6p1_back:       return "6.1(back)";
		case mlt_channel_6p1_front:      return "6.1(front)";
		case mlt_channel_7p0:            return "7.0";
		case mlt_channel_7p0_front:      return "7.0(front)";
		case mlt_channel_7p1:            return "7.1";
		case mlt_channel_7p1_wide_side:  return "7.1(wide-side)";
		case mlt_channel_7p1_wide_back:  return "7.1(wide)";
	}
	return "invalid";
}

/** Get the id of channel layout from short name.
 *
 * \public \memberof mlt_frame_s
 * \param name the channel layout short name
 * \return a channel layout
 */

mlt_channel_layout mlt_audio_channel_layout_id( const char * name )
{
	if( name )
	{
		mlt_channel_layout c;
		for( c = mlt_channel_auto; c <= mlt_channel_7p1_wide_back; c++ )
		{
			const char * v = mlt_audio_channel_layout_name( c );
			if( !strcmp( v, name ) )
				return c;
		}
	}
	return mlt_channel_auto;
}

/** Get the number of channels for a channel layout.
 *
 * \public \memberof mlt_frame_s
 * \param layout the channel layout
 * \return the number of channels for the channel layout
 */

int mlt_audio_channel_layout_channels( mlt_channel_layout layout )
{
	switch ( layout )
	{
		case mlt_channel_auto:           return 0;
		case mlt_channel_independent:    return 0;
		case mlt_channel_mono:           return 1;
		case mlt_channel_stereo:         return 2;
		case mlt_channel_2p1:            return 3;
		case mlt_channel_3p0:            return 3;
		case mlt_channel_3p0_back:       return 3;
		case mlt_channel_4p0:            return 4;
		case mlt_channel_quad_back:      return 4;
		case mlt_channel_quad_side:      return 4;
		case mlt_channel_3p1:            return 4;
		case mlt_channel_5p0_back:       return 5;
		case mlt_channel_5p0:            return 5;
		case mlt_channel_4p1:            return 5;
		case mlt_channel_5p1_back:       return 6;
		case mlt_channel_5p1:            return 6;
		case mlt_channel_6p0:            return 6;
		case mlt_channel_6p0_front:      return 6;
		case mlt_channel_hexagonal:      return 6;
		case mlt_channel_6p1:            return 7;
		case mlt_channel_6p1_back:       return 7;
		case mlt_channel_6p1_front:      return 7;
		case mlt_channel_7p0:            return 7;
		case mlt_channel_7p0_front:      return 7;
		case mlt_channel_7p1:            return 8;
		case mlt_channel_7p1_wide_back:  return 8;
		case mlt_channel_7p1_wide_side:  return 8;
	}
	return 0;
}

/** Get a default channel layout for a given number of channels.
 *
 * \public \memberof mlt_frame_s
 * \param channels the number of channels
 * \return the default channel layout
 */

mlt_channel_layout mlt_audio_channel_layout_default( int channels )
{
	mlt_channel_layout c;
	for( c = mlt_channel_mono; c <= mlt_channel_7p1_wide_back; c++ )
	{
		if( mlt_audio_channel_layout_channels( c ) == channels )
			return c;
	}
	return mlt_channel_independent;
}

/** Determine the number of samples that belong in a frame at a time position.
 *
 * \deprecated since 6.22. Prefer mlt_audio_calculate_samples()
 */

int mlt_sample_calculator( float fps, int frequency, int64_t position )
{
	return mlt_audio_calculate_frame_samples( fps, frequency, position );
}

/** Determine the number of samples that belong before a time position.
 *
 * \deprecated since 6.22. Prefer mlt_audio_calculate_samples_to_position()
 */

int64_t mlt_sample_calculator_to_now( float fps, int frequency, int64_t position )
{
	return mlt_audio_calculate_samples_to_position( fps, frequency, position );
}

/** Get the short name for a channel layout.
 *
 * \deprecated since 6.22. Prefer mlt_audio_channel_layout_name()
 */

const char * mlt_channel_layout_name( mlt_channel_layout layout )
{
	return mlt_audio_channel_layout_name( layout );
}

/** Get the id of channel layout from short name.
 *
 * \deprecated since 6.22. Prefer mlt_audio_channel_layout_id()
 */

mlt_channel_layout mlt_channel_layout_id( const char * name )
{
	return mlt_audio_channel_layout_id( name );
}

/** Get the number of channels for a channel layout.
 *
 * \deprecated since 6.22. Prefer mlt_audio_channel_layout_channels()
 */

int mlt_channel_layout_channels( mlt_channel_layout layout )
{
	return mlt_audio_channel_layout_channels( layout );
}

/** Get a default channel layout for a given number of channels.
 *
 * \deprecated since 6.22. Prefer mlt_audio_channel_layout_default()
 */

mlt_channel_layout mlt_channel_layout_default( int channels )
{
	return mlt_audio_channel_layout_default( channels );
}
