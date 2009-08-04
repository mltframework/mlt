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

/** Constructor for a frame.
*/

mlt_frame mlt_frame_init( mlt_service service )
{
	// Allocate a frame
	mlt_frame this = calloc( sizeof( struct mlt_frame_s ), 1 );

	if ( this != NULL )
	{
		mlt_profile profile = mlt_service_profile( service );

		// Initialise the properties
		mlt_properties properties = &this->parent;
		mlt_properties_init( properties, this );

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
		this->stack_image = mlt_deque_init( );
		this->stack_audio = mlt_deque_init( );
		this->stack_service = mlt_deque_init( );
	}

	return this;
}

/** Fetch the frames properties.
*/

mlt_properties mlt_frame_properties( mlt_frame this )
{
	return this != NULL ? &this->parent : NULL;
}

/** Check if we have a way to derive something other than a test card.
*/

int mlt_frame_is_test_card( mlt_frame this )
{
	return mlt_deque_count( this->stack_image ) == 0 || mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "test_image" );
}

/** Check if we have a way to derive something other than test audio.
*/

int mlt_frame_is_test_audio( mlt_frame this )
{
	return mlt_deque_count( this->stack_audio ) == 0 || mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "test_audio" );
}

/** Get the aspect ratio of the frame.
*/

double mlt_frame_get_aspect_ratio( mlt_frame this )
{
	return mlt_properties_get_double( MLT_FRAME_PROPERTIES( this ), "aspect_ratio" );
}

/** Set the aspect ratio of the frame.
*/

int mlt_frame_set_aspect_ratio( mlt_frame this, double value )
{
	return mlt_properties_set_double( MLT_FRAME_PROPERTIES( this ), "aspect_ratio", value );
}

/** Get the position of this frame.
*/

mlt_position mlt_frame_get_position( mlt_frame this )
{
	int pos = mlt_properties_get_position( MLT_FRAME_PROPERTIES( this ), "_position" );
	return pos < 0 ? 0 : pos;
}

/** Set the position of this frame.
*/

int mlt_frame_set_position( mlt_frame this, mlt_position value )
{
	return mlt_properties_set_position( MLT_FRAME_PROPERTIES( this ), "_position", value );
}

/** Stack a get_image callback.
*/

int mlt_frame_push_get_image( mlt_frame this, mlt_get_image get_image )
{
	return mlt_deque_push_back( this->stack_image, get_image );
}

/** Pop a get_image callback.
*/

mlt_get_image mlt_frame_pop_get_image( mlt_frame this )
{
	return mlt_deque_pop_back( this->stack_image );
}

/** Push a frame.
*/

int mlt_frame_push_frame( mlt_frame this, mlt_frame that )
{
	return mlt_deque_push_back( this->stack_image, that );
}

/** Pop a frame.
*/

mlt_frame mlt_frame_pop_frame( mlt_frame this )
{
	return mlt_deque_pop_back( this->stack_image );
}

/** Push a service.
*/

int mlt_frame_push_service( mlt_frame this, void *that )
{
	return mlt_deque_push_back( this->stack_image, that );
}

/** Pop a service.
*/

void *mlt_frame_pop_service( mlt_frame this )
{
	return mlt_deque_pop_back( this->stack_image );
}

/** Push a service.
*/

int mlt_frame_push_service_int( mlt_frame this, int that )
{
	return mlt_deque_push_back_int( this->stack_image, that );
}

/** Pop a service.
*/

int mlt_frame_pop_service_int( mlt_frame this )
{
	return mlt_deque_pop_back_int( this->stack_image );
}

/** Push an audio item on the stack.
*/

int mlt_frame_push_audio( mlt_frame this, void *that )
{
	return mlt_deque_push_back( this->stack_audio, that );
}

/** Pop an audio item from the stack
*/

void *mlt_frame_pop_audio( mlt_frame this )
{
	return mlt_deque_pop_back( this->stack_audio );
}

/** Return the service stack
*/

mlt_deque mlt_frame_service_stack( mlt_frame this )
{
	return this->stack_service;
}

/** Replace image stack with the information provided.

  	This might prove to be unreliable and restrictive - the idea is that a transition
	which normally uses two images may decide to only use the b frame (ie: in the case
	of a composite where the b frame completely obscures the a frame).

	The image must be writable and the destructor for the image itself must be taken
	care of on another frame and that frame cannot have a replace applied to it...
	Further it assumes that no alpha mask is in use.

	For these reasons, it can only be used in a specific situation - when you have
	multiple tracks each with their own transition and these transitions are applied
	in a strictly reversed order (ie: highest numbered [lowest track] is processed
	first).

	More reliable approach - the cases should be detected during the process phase
	and the upper tracks should simply not be invited to stack...
*/

void mlt_frame_replace_image( mlt_frame this, uint8_t *image, mlt_image_format format, int width, int height )
{
	// Remove all items from the stack
	while( mlt_deque_pop_back( this->stack_image ) ) ;

	// Update the information
	mlt_properties_set_data( MLT_FRAME_PROPERTIES( this ), "image", image, 0, NULL, NULL );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( this ), "width", width );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( this ), "height", height );
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( this ), "format", format );
	this->get_alpha_mask = NULL;
}

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

/** Get the image associated to the frame.
*/

int mlt_frame_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );
	mlt_get_image get_image = mlt_frame_pop_get_image( this );
	mlt_producer producer = mlt_properties_get_data( properties, "test_card_producer", NULL );
	mlt_image_format requested_format = *format;
	int error = 0;

	if ( get_image )
	{
		mlt_properties_set_int( properties, "image_count", mlt_properties_get_int( properties, "image_count" ) - 1 );
		mlt_position position = mlt_frame_get_position( this );
		error = get_image( this, buffer, format, width, height, writable );
		mlt_properties_set_int( properties, "width", *width );
		mlt_properties_set_int( properties, "height", *height );
		mlt_properties_set_int( properties, "format", *format );
		mlt_frame_set_position( this, position );
		if ( this->convert_image )
			this->convert_image( this, buffer, format, requested_format );
	}
	else if ( mlt_properties_get_data( properties, "image", NULL ) )
	{
		*format = mlt_properties_get_int( properties, "format" );
		*buffer = mlt_properties_get_data( properties, "image", NULL );
		*width = mlt_properties_get_int( properties, "width" );
		*height = mlt_properties_get_int( properties, "height" );
		if ( this->convert_image )
			this->convert_image( this, buffer, format, requested_format );
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
			mlt_frame_get_image( this, buffer, format, width, height, writable );
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

	mlt_properties_set_int( properties, "scaled_width", *width );
	mlt_properties_set_int( properties, "scaled_height", *height );

	return error;
}

uint8_t *mlt_frame_get_alpha_mask( mlt_frame this )
{
	uint8_t *alpha = NULL;
	if ( this != NULL )
	{
		if ( this->get_alpha_mask != NULL )
			alpha = this->get_alpha_mask( this );
		if ( alpha == NULL )
			alpha = mlt_properties_get_data( &this->parent, "alpha", NULL );
		if ( alpha == NULL )
		{
			int size = mlt_properties_get_int( &this->parent, "scaled_width" ) * mlt_properties_get_int( &this->parent, "scaled_height" );
			alpha = mlt_pool_alloc( size );
			memset( alpha, 255, size );
			mlt_properties_set_data( &this->parent, "alpha", alpha, size, mlt_pool_release, NULL );
		}
	}
	return alpha;
}

const char * mlt_audio_format_name( mlt_audio_format format )
{
	switch ( format )
	{
		case mlt_audio_none:   return "none";
		case mlt_audio_s16:    return "s16";
		case mlt_audio_s32:    return "s32";
		case mlt_audio_float:  return "float";
	}
	return "invalid";
}

int mlt_frame_get_audio( mlt_frame this, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_get_audio get_audio = mlt_frame_pop_audio( this );
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );
	int hide = mlt_properties_get_int( properties, "test_audio" );
	mlt_audio_format requested_format = *format;

	if ( hide == 0 && get_audio != NULL )
	{
		mlt_position position = mlt_frame_get_position( this );
		get_audio( this, buffer, format, frequency, channels, samples );
		mlt_frame_set_position( this, position );
		mlt_properties_set_int( properties, "audio_frequency", *frequency );
		mlt_properties_set_int( properties, "audio_channels", *channels );
		mlt_properties_set_int( properties, "audio_samples", *samples );
		mlt_properties_set_int( properties, "audio_format", *format );
		if ( this->convert_audio )
			this->convert_audio( this, buffer, format, requested_format );
	}
	else if ( mlt_properties_get_data( properties, "audio", NULL ) )
	{
		*buffer = mlt_properties_get_data( properties, "audio", NULL );
		*format = mlt_properties_get_int( properties, "audio_format" );
		*frequency = mlt_properties_get_int( properties, "audio_frequency" );
		*channels = mlt_properties_get_int( properties, "audio_channels" );
		*samples = mlt_properties_get_int( properties, "audio_samples" );
		if ( this->convert_audio )
			this->convert_audio( this, buffer, format, requested_format );
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

int mlt_frame_set_audio( mlt_frame this, void *buffer, mlt_audio_format format, int size, mlt_destructor destructor )
{
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( this ), "audio_format", format );
	return mlt_properties_set_data( MLT_FRAME_PROPERTIES( this ), "audio", buffer, size, destructor, NULL );
}

unsigned char *mlt_frame_get_waveform( mlt_frame this, int w, int h )
{
	int16_t *pcm = NULL;
	mlt_properties properties = MLT_FRAME_PROPERTIES( this );
	mlt_audio_format format = mlt_audio_s16;
	int frequency = 32000; // lower frequency available?
	int channels = 2;
	double fps = mlt_profile_fps( NULL );
	int samples = mlt_sample_calculator( fps, frequency, mlt_frame_get_position( this ) );

	// Get the pcm data
	mlt_frame_get_audio( this, (void**)&pcm, &format, &frequency, &channels, &samples );

	// Make an 8-bit buffer large enough to hold rendering
	int size = w * h;
	unsigned char *bitmap = ( unsigned char* )mlt_pool_alloc( size );
	if ( bitmap != NULL )
		memset( bitmap, 0, size );
	mlt_properties_set_data( properties, "waveform", bitmap, size, ( mlt_destructor )mlt_pool_release, NULL );

	// Render vertical lines
	int16_t *ubound = pcm + samples * channels;
	int skip = samples / w - 1;
	int i, j, k;

	// Iterate sample stream and along x coordinate
	for ( i = 0; i < w && pcm < ubound; i++ )
	{
		// pcm data has channels interleaved
		for ( j = 0; j < channels; j++ )
		{
			// Determine sample's magnitude from 2s complement;
			int pcm_magnitude = *pcm < 0 ? ~(*pcm) + 1 : *pcm;
			// The height of a line is the ratio of the magnitude multiplied by
			// half the vertical resolution
			int height = ( int )( ( double )( pcm_magnitude ) / 32768 * h / 2 );
			// Determine the starting y coordinate - left channel above center,
			// right channel below - currently assumes 2 channels
			int displacement = ( h / 2 ) - ( 1 - j ) * height;
			// Position buffer pointer using y coordinate, stride, and x coordinate
			unsigned char *p = &bitmap[ i + displacement * w ];

			// Draw vertical line
			for ( k = 0; k < height; k++ )
				p[ w * k ] = 0xFF;

			pcm++;
		}
		pcm += skip * channels;
	}

	return bitmap;
}

mlt_producer mlt_frame_get_original_producer( mlt_frame this )
{
	if ( this != NULL )
		return mlt_properties_get_data( MLT_FRAME_PROPERTIES( this ), "_producer", NULL );
	return NULL;
}

void mlt_frame_close( mlt_frame this )
{
	if ( this != NULL && mlt_properties_dec_ref( MLT_FRAME_PROPERTIES( this ) ) <= 0 )
	{
		mlt_deque_close( this->stack_image );
		mlt_deque_close( this->stack_audio );
		while( mlt_deque_peek_back( this->stack_service ) )
			mlt_service_close( mlt_deque_pop_back( this->stack_service ) );
		mlt_deque_close( this->stack_service );
		mlt_properties_close( &this->parent );
		free( this );
	}
}

/***** convenience functions *****/

/* Will this break when mlt_position is converted to double? -Zach */
int mlt_sample_calculator( float fps, int frequency, int64_t position )
{
	int samples = 0;

	if ( ( int )( fps * 100 ) == 2997 )
	{
		samples = frequency / 30;

		switch ( frequency )
		{
			case 48000:
				if ( position % 5 != 0 )
					samples += 2;
				break;
			case 44100:
				if ( position % 300 == 0 )
					samples = 1471;
				else if ( position % 30 == 0 )
					samples = 1470;
				else if ( position % 2 == 0 )
					samples = 1472;
				else
					samples = 1471;
				break;
			case 32000:
				if ( position % 30 == 0 )
					samples = 1068;
				else if ( position % 29 == 0 )
					samples = 1067;
				else if ( position % 4 == 2 )
					samples = 1067;
				else
					samples = 1068;
				break;
			default:
				samples = 0;
		}
	}
	else if ( fps != 0 )
	{
		samples = frequency / fps;
	}

	return samples;
}

int64_t mlt_sample_calculator_to_now( float fps, int frequency, int64_t frame )
{
	int64_t samples = 0;

	// TODO: Correct rules for NTSC and drop the * 100 hack
	if ( ( int )( fps * 100 ) == 2997 )
	{
		samples = ( ( double )( frame * frequency ) / 30 );
		switch( frequency )
		{
			case 48000:
				samples += 2 * ( frame / 5 );
				break;
			case 44100:
				samples += frame + ( frame / 2 ) - ( frame / 30 ) + ( frame / 300 );
				break;
			case 32000:
				samples += ( 2 * frame ) - ( frame / 4 ) - ( frame / 29 );
				break;
		}
	}
	else if ( fps != 0 )
	{
		samples = ( ( frame * frequency ) / ( int )fps );
	}

	return samples;
}
