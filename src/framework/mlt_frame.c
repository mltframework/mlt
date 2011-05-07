/**
 * \file mlt_frame.c
 * \brief interface for all frame classes
 * \see mlt_frame_s
 *
 * Copyright (C) 2003-2009 Ushodaya Enterprises Limited
 * \author Charles Yates <charles.yates@pandora.be>
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

#include "mlt_frame.h"
#include "mlt_producer.h"
#include "mlt_factory.h"
#include "mlt_profile.h"
#include "mlt_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Construct a frame object.
 *
 * \public \memberof mlt_frame_s
 * \param service the pointer to any service that can provide access to the profile
 * \return a frame object on success or NULL if there was an allocation error
 */

mlt_frame mlt_frame_init( mlt_service service )
{
	// Allocate a frame
	mlt_frame self = calloc( sizeof( struct mlt_frame_s ), 1 );

	if ( self != NULL )
	{
		mlt_profile profile = mlt_service_profile( service );

		// Initialise the properties
		mlt_properties properties = &self->parent;
		mlt_properties_init( properties, self );

		// Set default properties on the frame
		mlt_properties_set_position( properties, "_position", 0.0 );
		mlt_properties_set_data( properties, "image", NULL, 0, NULL, NULL );
		mlt_properties_set_int( properties, "width", profile? profile->width : 720 );
		mlt_properties_set_int( properties, "height", profile? profile->height : 576 );
		mlt_properties_set_int( properties, "normalised_width", profile? profile->width : 720 );
		mlt_properties_set_int( properties, "normalised_height", profile? profile->height : 576 );
		mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( NULL ) );
		mlt_properties_set_data( properties, "audio", NULL, 0, NULL, NULL );
		mlt_properties_set_data( properties, "alpha", NULL, 0, NULL, NULL );

		// Construct stacks for frames and methods
		self->stack_image = mlt_deque_init( );
		self->stack_audio = mlt_deque_init( );
		self->stack_service = mlt_deque_init( );
	}

	return self;
}

/** Get a frame's properties.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return the frame's properties or NULL if an invalid frame is supplied
 */

mlt_properties mlt_frame_properties( mlt_frame self )
{
	return self != NULL ? &self->parent : NULL;
}

/** Determine if the frame will produce a test card image.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return true (non-zero) if this will produce from a test card
 */

int mlt_frame_is_test_card( mlt_frame self )
{
	return mlt_deque_count( self->stack_image ) == 0 || mlt_properties_get_int( MLT_FRAME_PROPERTIES( self ), "test_image" );
}

/** Determine if the frame will produce audio from a test card.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return true (non-zero) if this will produce from a test card
 */

int mlt_frame_is_test_audio( mlt_frame self )
{
	return mlt_deque_count( self->stack_audio ) == 0 || mlt_properties_get_int( MLT_FRAME_PROPERTIES( self ), "test_audio" );
}

/** Get the sample aspect ratio of the frame.
 *
 * \public \memberof  mlt_frame_s
 * \param self a frame
 * \return the aspect ratio
 */

double mlt_frame_get_aspect_ratio( mlt_frame self )
{
	return mlt_properties_get_double( MLT_FRAME_PROPERTIES( self ), "aspect_ratio" );
}

/** Set the sample aspect ratio of the frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param value the new image sample aspect ratio
 * \return true if error
 */

int mlt_frame_set_aspect_ratio( mlt_frame self, double value )
{
	return mlt_properties_set_double( MLT_FRAME_PROPERTIES( self ), "aspect_ratio", value );
}

/** Get the time position of this frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return the position
 */

mlt_position mlt_frame_get_position( mlt_frame self )
{
	int pos = mlt_properties_get_position( MLT_FRAME_PROPERTIES( self ), "_position" );
	return pos < 0 ? 0 : pos;
}

/** Set the time position of this frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param value the position
 * \return true if error
 */

int mlt_frame_set_position( mlt_frame self, mlt_position value )
{
	return mlt_properties_set_position( MLT_FRAME_PROPERTIES( self ), "_position", value );
}

/** Stack a get_image callback.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param the get_image callback
 * \return true if error
 */

int mlt_frame_push_get_image( mlt_frame self, mlt_get_image get_image )
{
	return mlt_deque_push_back( self->stack_image, get_image );
}

/** Pop a get_image callback.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return the get_image callback
 */

mlt_get_image mlt_frame_pop_get_image( mlt_frame self )
{
	return mlt_deque_pop_back( self->stack_image );
}

/** Push a frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param that the frame to push onto \p self
 * \return true if error
 */

int mlt_frame_push_frame( mlt_frame self, mlt_frame that )
{
	return mlt_deque_push_back( self->stack_image, that );
}

/** Pop a frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return a frame that was previously pushed
 */

mlt_frame mlt_frame_pop_frame( mlt_frame self )
{
	return mlt_deque_pop_back( self->stack_image );
}

/** Push a service.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param that an opaque pointer
 * \return true if error
 */

int mlt_frame_push_service( mlt_frame self, void *that )
{
	return mlt_deque_push_back( self->stack_image, that );
}

/** Pop a service.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return an opaque pointer to something previously pushed
 */

void *mlt_frame_pop_service( mlt_frame self )
{
	return mlt_deque_pop_back( self->stack_image );
}

/** Push a number.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param that an integer
 * \return true if error
 */

int mlt_frame_push_service_int( mlt_frame self, int that )
{
	return mlt_deque_push_back_int( self->stack_image, that );
}

/** Pop a number.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return an integer that was previously pushed
 */

int mlt_frame_pop_service_int( mlt_frame self )
{
	return mlt_deque_pop_back_int( self->stack_image );
}

/** Push an audio item on the stack.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param that an opaque pointer
 * \return true if error
 */

int mlt_frame_push_audio( mlt_frame self, void *that )
{
	return mlt_deque_push_back( self->stack_audio, that );
}

/** Pop an audio item from the stack
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return an opaque pointer to something that was pushed onto the frame's audio stack
 */

void *mlt_frame_pop_audio( mlt_frame self )
{
	return mlt_deque_pop_back( self->stack_audio );
}

/** Return the service stack
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return the service stack
 */

mlt_deque mlt_frame_service_stack( mlt_frame self )
{
	return self->stack_service;
}

/** Set a new image on the frame.
  *
  * \public \memberof mlt_frame_s
  * \param self a frame
  * \param image a pointer to the raw image data
  * \param size the size of the image data in bytes (optional)
  * \param destroy a function to deallocate \p image when the frame is closed (optional)
  * \return true if error
  */

int mlt_frame_set_image( mlt_frame self, uint8_t *image, int size, mlt_destructor destroy )
{
	return mlt_properties_set_data( MLT_FRAME_PROPERTIES( self ), "image", image, size, destroy, NULL );
}

/** Set a new alpha channel on the frame.
  *
  * \public \memberof mlt_frame_s
  * \param self a frame
  * \param alpha a pointer to the alpha channel
  * \param size the size of the alpha channel in bytes (optional)
  * \param destroy a function to deallocate \p alpha when the frame is closed (optional)
  * \return true if error
  */

int mlt_frame_set_alpha( mlt_frame self, uint8_t *alpha, int size, mlt_destructor destroy )
{
	return mlt_properties_set_data( MLT_FRAME_PROPERTIES( self ), "alpha", alpha, size, destroy, NULL );
}

/** Replace image stack with the information provided.
 *
 * This might prove to be unreliable and restrictive - the idea is that a transition
 * which normally uses two images may decide to only use the b frame (ie: in the case
 * of a composite where the b frame completely obscures the a frame).
 *
 * The image must be writable and the destructor for the image itself must be taken
 * care of on another frame and that frame cannot have a replace applied to it...
 * Further it assumes that no alpha mask is in use.
 *
 * For these reasons, it can only be used in a specific situation - when you have
 * multiple tracks each with their own transition and these transitions are applied
 * in a strictly reversed order (ie: highest numbered [lowest track] is processed
 * first).
 *
 * More reliable approach - the cases should be detected during the process phase
 * and the upper tracks should simply not be invited to stack...
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param image a new image
 * \param format the image format
 * \param width the width of the new image
 * \param height the height of the new image
 */

void mlt_frame_replace_image( mlt_frame self, uint8_t *image, mlt_image_format format, int width, int height )
{
	// Remove all items from the stack
	while( mlt_deque_pop_back( self->stack_image ) ) ;

	// Update the information
	mlt_properties_set_data( MLT_FRAME_PROPERTIES( self ), "image", image, 0, NULL, NULL );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( self ), "width", width );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( self ), "height", height );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( self ), "format", format );
	self->get_alpha_mask = NULL;
}

/** Get the short name for an image format.
 *
 * \public \memberof mlt_frame_s
 * \param format the image format
 * \return a string
 */

const char * mlt_image_format_name( mlt_image_format format )
{
	switch ( format )
	{
		case mlt_image_none:    return "none";
		case mlt_image_rgb24:   return "rgb24";
		case mlt_image_rgb24a:  return "rgb24a";
		case mlt_image_yuv422:  return "yuv422";
		case mlt_image_yuv420p: return "yuv420p";
		case mlt_image_opengl:  return "opengl";
	}
	return "invalid";
}

/** Get the number of bytes needed for an image.
  *
  * \public \memberof mlt_frame_s
  * \param format the image format
  * \param width width of the image in pixels
  * \param height height of the image in pixels
  * \param[out] bpp the number of bytes per pixel (optional)
  * \return the number of bytes
  */
int mlt_image_format_size( mlt_image_format format, int width, int height, int *bpp )
{
	height += 1;
	switch ( format )
	{
		case mlt_image_none:
			if ( bpp ) *bpp = 0;
			return 0;
		case mlt_image_rgb24:
			if ( bpp ) *bpp = 3;
			return width * height * 3;
		case mlt_image_opengl:
		case mlt_image_rgb24a:
			if ( bpp ) *bpp = 4;
			return width * height * 4;
		case mlt_image_yuv422:
			if ( bpp ) *bpp = 2;
			return width * height * 2;
		case mlt_image_yuv420p:
			if ( bpp ) *bpp = 3 / 2;
			return width * height * 3 / 2;
	}
	return 0;
}

/** Get the image associated to the frame.
 *
 * You should express the desired format, width, and height as inputs. As long
 * as the loader producer was used to generate this or the imageconvert filter
 * was attached, then you will get the image back in the format you desire.
 * However, you do not always get the width and height you request depending
 * on properties and filters. You do not need to supply a pre-allocated
 * buffer, but you should always supply the desired image format.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param[out] buffer an image buffer
 * \param[in,out] format the image format
 * \param[in,out] width the horizontal size in pixels
 * \param[in,out] height the vertical size in pixels
 * \param writable whether or not you will need to be able to write to the memory returned in \p buffer
 * \return true if error
 * \todo Better describe the width and height as inputs.
 */

int mlt_frame_get_image( mlt_frame self, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_properties properties = MLT_FRAME_PROPERTIES( self );
	mlt_get_image get_image = mlt_frame_pop_get_image( self );
	mlt_producer producer = mlt_properties_get_data( properties, "test_card_producer", NULL );
	mlt_image_format requested_format = *format;
	int error = 0;

	if ( get_image )
	{
		mlt_properties_set_int( properties, "image_count", mlt_properties_get_int( properties, "image_count" ) - 1 );
		error = get_image( self, buffer, format, width, height, writable );
		if ( !error && *buffer )
		{
			mlt_properties_set_int( properties, "width", *width );
			mlt_properties_set_int( properties, "height", *height );
			if ( self->convert_image && *buffer )
				self->convert_image( self, buffer, format, requested_format );
			mlt_properties_set_int( properties, "format", *format );
		}
		else
		{
			// Cause the image to be loaded from test card or fallback (white) below.
			mlt_frame_get_image( self, buffer, format, width, height, writable );
		}
	}
	else if ( mlt_properties_get_data( properties, "image", NULL ) )
	{
		*format = mlt_properties_get_int( properties, "format" );
		*buffer = mlt_properties_get_data( properties, "image", NULL );
		*width = mlt_properties_get_int( properties, "width" );
		*height = mlt_properties_get_int( properties, "height" );
		if ( self->convert_image && *buffer )
		{
			self->convert_image( self, buffer, format, requested_format );
			mlt_properties_set_int( properties, "format", *format );
		}
	}
	else if ( producer )
	{
		mlt_frame test_frame = NULL;
		mlt_service_get_frame( MLT_PRODUCER_SERVICE( producer ), &test_frame, 0 );
		if ( test_frame )
		{
			mlt_properties test_properties = MLT_FRAME_PROPERTIES( test_frame );
			mlt_properties_set_double( test_properties, "consumer_aspect_ratio", mlt_properties_get_double( properties, "consumer_aspect_ratio" ) );
			mlt_properties_set( test_properties, "rescale.interp", mlt_properties_get( properties, "rescale.interp" ) );
			mlt_frame_get_image( test_frame, buffer, format, width, height, writable );
			mlt_properties_set_data( properties, "test_card_frame", test_frame, 0, ( mlt_destructor )mlt_frame_close, NULL );
			mlt_properties_set_double( properties, "aspect_ratio", mlt_frame_get_aspect_ratio( test_frame ) );
// 			mlt_properties_set_data( properties, "image", *buffer, *width * *height * 2, NULL, NULL );
// 			mlt_properties_set_int( properties, "width", *width );
// 			mlt_properties_set_int( properties, "height", *height );
// 			mlt_properties_set_int( properties, "format", *format );
		}
		else
		{
			mlt_properties_set_data( properties, "test_card_producer", NULL, 0, NULL, NULL );
			mlt_frame_get_image( self, buffer, format, width, height, writable );
		}
	}
	else
	{
		register uint8_t *p;
		register uint8_t *q;
		int size = 0;

		*width = *width == 0 ? 720 : *width;
		*height = *height == 0 ? 576 : *height;
		size = *width * *height;

		mlt_properties_set_int( properties, "format", *format );
		mlt_properties_set_int( properties, "width", *width );
		mlt_properties_set_int( properties, "height", *height );
		mlt_properties_set_int( properties, "aspect_ratio", 0 );

		switch( *format )
		{
			case mlt_image_none:
				size = 0;
				*buffer = NULL;
				break;
			case mlt_image_rgb24:
				size *= 3;
				size += *width * 3;
				*buffer = mlt_pool_alloc( size );
				if ( *buffer )
					memset( *buffer, 255, size );
				break;
			case mlt_image_rgb24a:
			case mlt_image_opengl:
				size *= 4;
				size += *width * 4;
				*buffer = mlt_pool_alloc( size );
				if ( *buffer )
					memset( *buffer, 255, size );
				break;
			case mlt_image_yuv422:
				size *= 2;
				size += *width * 2;
				*buffer = mlt_pool_alloc( size );
				p = *buffer;
				q = p + size;
				while ( p != NULL && p != q )
				{
					*p ++ = 235;
					*p ++ = 128;
				}
				break;
			case mlt_image_yuv420p:
				size = size * 3 / 2;
				*buffer = mlt_pool_alloc( size );
				if ( *buffer )
					memset( *buffer, 255, size );
				break;
		}

		mlt_properties_set_data( properties, "image", *buffer, size, ( mlt_destructor )mlt_pool_release, NULL );
		mlt_properties_set_int( properties, "test_image", 1 );
	}

	return error;
}

/** Get the alpha channel associated to the frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return the alpha channel
 */

uint8_t *mlt_frame_get_alpha_mask( mlt_frame self )
{
	uint8_t *alpha = NULL;
	if ( self != NULL )
	{
		if ( self->get_alpha_mask != NULL )
			alpha = self->get_alpha_mask( self );
		if ( alpha == NULL )
			alpha = mlt_properties_get_data( &self->parent, "alpha", NULL );
		if ( alpha == NULL )
		{
			int size = mlt_properties_get_int( &self->parent, "width" ) * mlt_properties_get_int( &self->parent, "height" );
			alpha = mlt_pool_alloc( size );
			memset( alpha, 255, size );
			mlt_properties_set_data( &self->parent, "alpha", alpha, size, mlt_pool_release, NULL );
		}
	}
	return alpha;
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
	}
	return 0;
}

/** Get the audio associated to the frame.
 *
 * You should express the desired format, frequency, channels, and samples as inputs. As long
 * as the loader producer was used to generate this or the audioconvert filter
 * was attached, then you will get the audio back in the format you desire.
 * However, you do not always get the channels and samples you request depending
 * on properties and filters. You do not need to supply a pre-allocated
 * buffer, but you should always supply the desired audio format.
 * The audio is always in interleaved format.
 * You should use the \p mlt_sample_calculator to determine the number of samples you want.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param[out] buffer an audio buffer
 * \param[in,out] format the audio format
 * \param[in,out] frequency the sample rate
 * \param[in,out] channels
 * \param[in,out] samples the number of samples per frame
 * \return true if error
 */

int mlt_frame_get_audio( mlt_frame self, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_get_audio get_audio = mlt_frame_pop_audio( self );
	mlt_properties properties = MLT_FRAME_PROPERTIES( self );
	int hide = mlt_properties_get_int( properties, "test_audio" );
	mlt_audio_format requested_format = *format;

	if ( hide == 0 && get_audio != NULL )
	{
		get_audio( self, buffer, format, frequency, channels, samples );
		mlt_properties_set_int( properties, "audio_frequency", *frequency );
		mlt_properties_set_int( properties, "audio_channels", *channels );
		mlt_properties_set_int( properties, "audio_samples", *samples );
		mlt_properties_set_int( properties, "audio_format", *format );
		if ( self->convert_audio )
			self->convert_audio( self, buffer, format, requested_format );
	}
	else if ( mlt_properties_get_data( properties, "audio", NULL ) )
	{
		*buffer = mlt_properties_get_data( properties, "audio", NULL );
		*format = mlt_properties_get_int( properties, "audio_format" );
		*frequency = mlt_properties_get_int( properties, "audio_frequency" );
		*channels = mlt_properties_get_int( properties, "audio_channels" );
		*samples = mlt_properties_get_int( properties, "audio_samples" );
		if ( self->convert_audio )
			self->convert_audio( self, buffer, format, requested_format );
	}
	else
	{
		int size = 0;
		*samples = *samples <= 0 ? 1920 : *samples;
		*channels = *channels <= 0 ? 2 : *channels;
		*frequency = *frequency <= 0 ? 48000 : *frequency;
		mlt_properties_set_int( properties, "audio_frequency", *frequency );
		mlt_properties_set_int( properties, "audio_channels", *channels );
		mlt_properties_set_int( properties, "audio_samples", *samples );
		mlt_properties_set_int( properties, "audio_format", *format );

		switch( *format )
		{
			case mlt_image_none:
				size = 0;
				*buffer = NULL;
				break;
			case mlt_audio_s16:
				size = *samples * *channels * sizeof( int16_t );
				break;
			case mlt_audio_s32:
				size = *samples * *channels * sizeof( int32_t );
				break;
			case mlt_audio_float:
				size = *samples * *channels * sizeof( float );
				break;
			default:
				break;
		}
		if ( size )
			*buffer = mlt_pool_alloc( size );
		if ( *buffer )
			memset( *buffer, 0, size );
		mlt_properties_set_data( properties, "audio", *buffer, size, ( mlt_destructor )mlt_pool_release, NULL );
		mlt_properties_set_int( properties, "test_audio", 1 );
	}

	// TODO: This does not belong here
	if ( *format == mlt_audio_s16 && mlt_properties_get( properties, "meta.volume" ) )
	{
		double value = mlt_properties_get_double( properties, "meta.volume" );

		if ( value == 0.0 )
		{
			memset( *buffer, 0, *samples * *channels * 2 );
		}
		else if ( value != 1.0 )
		{
			int total = *samples * *channels;
			int16_t *p = *buffer;
			while ( total -- )
			{
				*p = *p * value;
				p ++;
			}
		}

		mlt_properties_set( properties, "meta.volume", NULL );
	}

	return 0;
}

/** Set the audio on a frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param buffer an buffer containing audio samples
 * \param format the format of the audio in the \p buffer
 * \param size the total size of the buffer (optional)
 * \param destructor a function that releases or deallocates the \p buffer
 * \return true if error
 */

int mlt_frame_set_audio( mlt_frame self, void *buffer, mlt_audio_format format, int size, mlt_destructor destructor )
{
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( self ), "audio_format", format );
	return mlt_properties_set_data( MLT_FRAME_PROPERTIES( self ), "audio", buffer, size, destructor, NULL );
}

/** Get audio on a frame as a waveform image.
 *
 * This generates an 8-bit grayscale image representation of the audio in a
 * frame. Currently, this only really works for 2 channels.
 * This allocates the bitmap using mlt_pool so you should release the return
 * value with \p mlt_pool_release.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param w the width of the image
 * \param h the height of the image to create
 * \return a pointer to a new bitmap
 */

unsigned char *mlt_frame_get_waveform( mlt_frame self, int w, int h )
{
	int16_t *pcm = NULL;
	mlt_properties properties = MLT_FRAME_PROPERTIES( self );
	mlt_audio_format format = mlt_audio_s16;
	int frequency = 16000;
	int channels = 2;
	mlt_producer producer = mlt_frame_get_original_producer( self );
	double fps = mlt_producer_get_fps( mlt_producer_cut_parent( producer ) );
	int samples = mlt_sample_calculator( fps, frequency, mlt_frame_get_position( self ) );

	// Increase audio resolution proportional to requested image size
	while ( samples < w )
	{
		frequency += 16000;
		samples = mlt_sample_calculator( fps, frequency, mlt_frame_get_position( self ) );
	}

	// Get the pcm data
	mlt_frame_get_audio( self, (void**)&pcm, &format, &frequency, &channels, &samples );

	// Make an 8-bit buffer large enough to hold rendering
	int size = w * h;
	unsigned char *bitmap = ( unsigned char* )mlt_pool_alloc( size );
	if ( bitmap != NULL )
		memset( bitmap, 0, size );
	mlt_properties_set_data( properties, "waveform", bitmap, size, ( mlt_destructor )mlt_pool_release, NULL );

	// Render vertical lines
	int16_t *ubound = pcm + samples * channels;
	int skip = samples / w;
	skip = !skip ? 1 : skip;
	unsigned char gray = 0xFF / skip;
	int i, j, k;

	// Iterate sample stream and along x coordinate
	for ( i = 0; pcm < ubound; i++ )
	{
		// pcm data has channels interleaved
		for ( j = 0; j < channels; j++, pcm++ )
		{
			// Determine sample's magnitude from 2s complement;
			int pcm_magnitude = *pcm < 0 ? ~(*pcm) + 1 : *pcm;
			// The height of a line is the ratio of the magnitude multiplied by
			// the vertical resolution of a single channel
				int height = h * pcm_magnitude / channels / 2 / 32768;
			// Determine the starting y coordinate - left top, right bottom
			int displacement = h * (j * 2 + 1) / channels / 2 - ( *pcm < 0 ? 0 : height );
			// Position buffer pointer using y coordinate, stride, and x coordinate
			unsigned char *p = bitmap + i / skip + displacement * w;

			// Draw vertical line
			for ( k = 0; k < height + 1; k++ )
				if ( *pcm < 0 )
					p[ w * k ] = ( k == 0 ) ? 0xFF : p[ w * k ] + gray;
				else
					p[ w * k ] = ( k == height ) ? 0xFF : p[ w * k ] + gray;
		}
	}

	return bitmap;
}

/** Get the end service that produced self frame.
 *
 * This fetches the first producer of the frame and not any producers that
 * encapsulate it.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return a producer
 */

mlt_producer mlt_frame_get_original_producer( mlt_frame self )
{
	if ( self != NULL )
		return mlt_properties_get_data( MLT_FRAME_PROPERTIES( self ), "_producer", NULL );
	return NULL;
}

/** Destroy the frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 */

void mlt_frame_close( mlt_frame self )
{
	if ( self != NULL && mlt_properties_dec_ref( MLT_FRAME_PROPERTIES( self ) ) <= 0 )
	{
		mlt_deque_close( self->stack_image );
		mlt_deque_close( self->stack_audio );
		while( mlt_deque_peek_back( self->stack_service ) )
			mlt_service_close( mlt_deque_pop_back( self->stack_service ) );
		mlt_deque_close( self->stack_service );
		mlt_properties_close( &self->parent );
		free( self );
	}
}

/***** convenience functions *****/

/** Determine the number of samples that belong in a frame at a time position.
 *
 * \public \memberof mlt_frame_s
 * \param fps the frame rate
 * \param frequency the sample rate
 * \param position the time position
 * \return the number of samples per channel
 */

int mlt_sample_calculator( float fps, int frequency, int64_t position )
{
	/* Compute the cumulative number of samples until the start of this frame and the
	cumulative number of samples until the start of the next frame. Round each to the
	nearest integer and take the difference to determine the number of samples in
	this frame.

	This approach should prevent rounding errors that can accumulate over a large number
	of frames causing A/V sync problems. */
	return mlt_sample_calculator_to_now( fps, frequency, position + 1 )
		 - mlt_sample_calculator_to_now( fps, frequency, position );
}

/** Determine the number of samples that belong before a time position.
 *
 * \public \memberof mlt_frame_s
 * \param fps the frame rate
 * \param frequency the sample rate
 * \param position the time position
 * \return the number of samples per channel
 * \bug Will this break when mlt_position is converted to double?
 */

int64_t mlt_sample_calculator_to_now( float fps, int frequency, int64_t position )
{
	int64_t samples = 0;

	if ( fps )
	{
		samples = (int64_t)( (double) position * (double) frequency / (double) fps +
			( position < 0 ? -0.5 : 0.5 ) );
	}

	return samples;
}

void mlt_frame_write_ppm( mlt_frame frame )
{
	int width;
	int height;
	mlt_image_format format = mlt_image_rgb24;
	uint8_t *image;
	
	if ( mlt_frame_get_image( frame, &image, &format, &width, &height, 0 ) == 0 )
	{
		FILE *file;
		char filename[16];
		
		sprintf( filename, "frame-%05d.ppm", mlt_frame_get_position( frame ) );
		file = fopen( filename, "wb" );
		if ( !file )
			return;
		fprintf( file, "P6\n%d %d\n255\n", width, height);
		fwrite( image, width * height * 3, 1, file );
		fclose( file );
	}
}

/** Get or create a properties object unique to this service instance.
 *
 * Use this function to hold a service's processing parameters for this
 * particular frame. Set the parameters in the service's process function.
 * Then, get the parameters in the function it pushes to the frame's audio
 * or image stack. This makes the service more parallel by reducing race
 * conditions and less sensitive to multiple instances (by not setting a
 * non-unique property on the frame). Creation and destruction of the
 * properties object is handled automatically.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param service a service
 * \return a properties object
 */

mlt_properties mlt_frame_unique_properties( mlt_frame self, mlt_service service )
{
	mlt_properties frame_props = MLT_FRAME_PROPERTIES( self );
	mlt_properties service_props = MLT_SERVICE_PROPERTIES( service );
	char *unique = mlt_properties_get( service_props, "_unique_id" );
	mlt_properties instance_props = mlt_properties_get_data( frame_props, unique, NULL );
	
	if ( !instance_props )
	{
		instance_props = mlt_properties_new();
		mlt_properties_set_data( frame_props, unique, instance_props, 0, (mlt_destructor) mlt_properties_close, NULL );
	}

	return instance_props;
}
