/*
 * mlt_frame.c -- interface for all frame classes
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "mlt_frame.h"
#include "mlt_producer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Constructor for a frame.
*/

mlt_frame mlt_frame_init( )
{
	// Allocate a frame
	mlt_frame this = calloc( sizeof( struct mlt_frame_s ), 1 );

	if ( this != NULL )
	{
		// Get the normalisation
		char *normalisation = getenv( "MLT_NORMALISATION" );

		// Initialise the properties
		mlt_properties properties = &this->parent;
		mlt_properties_init( properties, this );

		// Set default properties on the frame
		mlt_properties_set_position( properties, "_position", 0.0 );
		mlt_properties_set_data( properties, "image", NULL, 0, NULL, NULL );

		if ( normalisation == NULL || strcmp( normalisation, "NTSC" ) )
		{
			mlt_properties_set_int( properties, "width", 720 );
			mlt_properties_set_int( properties, "height", 576 );
			mlt_properties_set_int( properties, "normalised_width", 720 );
			mlt_properties_set_int( properties, "normalised_height", 576 );
		}
		else
		{
			mlt_properties_set_int( properties, "width", 720 );
			mlt_properties_set_int( properties, "height", 480 );
			mlt_properties_set_int( properties, "normalised_width", 720 );
			mlt_properties_set_int( properties, "normalised_height", 480 );
		}

		mlt_properties_set_double( properties, "aspect_ratio", 4.0 / 3.0 );
		mlt_properties_set_data( properties, "audio", NULL, 0, NULL, NULL );
		mlt_properties_set_data( properties, "alpha", NULL, 0, NULL, NULL );


	}
	return this;
}

/** Fetch the frames properties.
*/

mlt_properties mlt_frame_properties( mlt_frame this )
{
	return &this->parent;
}

/** Check if we have a way to derive something other than a test card.
*/

int mlt_frame_is_test_card( mlt_frame this )
{
	return mlt_properties_get_int( mlt_frame_properties( this ), "test_image" );
}

/** Check if we have a way to derive something than test audio.
*/

int mlt_frame_is_test_audio( mlt_frame this )
{
	return this->get_audio == NULL || mlt_properties_get_int( mlt_frame_properties( this ), "test_audio" );
}

/** Get the aspect ratio of the frame.
*/

double mlt_frame_get_aspect_ratio( mlt_frame this )
{
	return mlt_properties_get_double( mlt_frame_properties( this ), "aspect_ratio" );
}

/** Set the aspect ratio of the frame.
*/

int mlt_frame_set_aspect_ratio( mlt_frame this, double value )
{
	return mlt_properties_set_double( mlt_frame_properties( this ), "aspect_ratio", value );
}

/** Get the position of this frame.
*/

mlt_position mlt_frame_get_position( mlt_frame this )
{
	return mlt_properties_get_position( mlt_frame_properties( this ), "_position" );
}

/** Set the position of this frame.
*/

int mlt_frame_set_position( mlt_frame this, mlt_position value )
{
	return mlt_properties_set_position( mlt_frame_properties( this ), "_position", value );
}

/** Stack a get_image callback.
*/

int mlt_frame_push_get_image( mlt_frame this, mlt_get_image get_image )
{
	int ret = this->stack_get_image_size >= 10;
	if ( ret == 0 )
		this->stack_get_image[ this->stack_get_image_size ++ ] = get_image;
	return ret;
}

/** Pop a get_image callback.
*/

mlt_get_image mlt_frame_pop_get_image( mlt_frame this )
{
	mlt_get_image result = NULL;
	if ( this->stack_get_image_size > 0 )
		result = this->stack_get_image[ -- this->stack_get_image_size ];
	return result;
}

/** Push a frame.
*/

int mlt_frame_push_frame( mlt_frame this, mlt_frame that )
{
	int ret = this->stack_frame_size >= 10;
	if ( ret == 0 )
		this->stack_frame[ this->stack_frame_size ++ ] = that;
	return ret;
}

/** Pop a frame.
*/

mlt_frame mlt_frame_pop_frame( mlt_frame this )
{
	mlt_frame result = NULL;
	if ( this->stack_frame_size > 0 )
		result = this->stack_frame[ -- this->stack_frame_size ];
	return result;
}

int mlt_frame_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_properties properties = mlt_frame_properties( this );
	mlt_get_image get_image = mlt_frame_pop_get_image( this );
	mlt_producer producer = mlt_properties_get_data( properties, "test_card_producer", NULL );
	
	if ( get_image != NULL )
	{
		return get_image( this, buffer, format, width, height, writable );
	}
	else if ( mlt_properties_get_data( properties, "image", NULL ) != NULL )
	{
		*format = mlt_image_yuv422;
		*buffer = mlt_properties_get_data( properties, "image", NULL );
		*width = mlt_properties_get_int( properties, "width" );
		*height = mlt_properties_get_int( properties, "height" );
	}
	else if ( producer != NULL )
	{
		mlt_frame test_frame = NULL;
		mlt_service_get_frame( mlt_producer_service( producer ), &test_frame, 0 );
		if ( test_frame != NULL )
		{
			mlt_properties test_properties = mlt_frame_properties( test_frame );
			mlt_properties_set_double( test_properties, "consumer_aspect_ratio", mlt_properties_get_double( properties, "consumer_aspect_ratio" ) );
			mlt_properties_set_double( test_properties, "consumer_scale", mlt_properties_get_double( properties, "consumer_scale" ) );
			mlt_properties_set( test_properties, "rescale.interp", "nearest" );
			mlt_frame_get_image( test_frame, buffer, format, width, height, writable );
			mlt_properties_inherit( properties, test_properties );
			mlt_properties_set_data( properties, "test_card_frame", test_frame, 0, ( mlt_destructor )mlt_frame_close, NULL );
			mlt_properties_set_data( properties, "image", *buffer, *width * *height * 2, NULL, NULL );
			mlt_properties_set_int( properties, "width", *width );
			mlt_properties_set_int( properties, "height", *height );
		}
		else
		{
			mlt_properties_set_data( properties, "test_card_producer", NULL, 0, NULL, NULL );
			mlt_frame_get_image( this, buffer, format, width, height, writable );
		}
	}
	else
	{
		uint8_t *p;
		uint8_t *q;
		int size = 0;

		*width = *width == 0 ? 720 : *width;
		*height = *height == 0 ? 576 : *height;
		size = *width * *height;

		mlt_properties_set_int( properties, "width", *width );
		mlt_properties_set_int( properties, "height", *height );

		switch( *format )
		{
			case mlt_image_none:
				size = 0;
				*buffer = NULL;
				break;
			case mlt_image_rgb24:
				// IRRIGATE ME
				size *= 3;
				size += *width * 3;
				*buffer = malloc( size );
				if ( *buffer )
					memset( *buffer, 255, size );
				break;
			case mlt_image_rgb24a:
				// IRRIGATE ME
				size *= 4;
				size += *width * 4;
				*buffer = malloc( size );
				if ( *buffer )
					memset( *buffer, 255, size );
				break;
			case mlt_image_yuv422:
				// IRRIGATE ME
				size *= 2;
				size += *width * 2;
				*buffer = malloc( size );
				p = *buffer;
				q = p + size;
				while ( p != NULL && p != q )
				{
					*p ++ = 255;
					*p ++ = 128;
				}
				break;
			case mlt_image_yuv420p:
				size = size * 3 / 2;
				*buffer = malloc( size );
					if ( *buffer )
					memset( *buffer, 255, size );
				break;
		}

		mlt_properties_set_data( properties, "image", *buffer, size, free, NULL );
		mlt_properties_set_int( properties, "test_image", 1 );
	}

	return 0;
}

uint8_t *mlt_frame_get_alpha_mask( mlt_frame this )
{
	if ( this->get_alpha_mask != NULL )
		return this->get_alpha_mask( this );
	return NULL;
}

int mlt_frame_get_audio( mlt_frame this, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	mlt_properties properties = mlt_frame_properties( this );

	if ( this->get_audio != NULL )
	{
		return this->get_audio( this, buffer, format, frequency, channels, samples );
	}
	else
	{
		int size = 0;
		*samples = *samples <= 0 ? 1920 : *samples;
		*channels = *channels <= 0 ? 2 : *channels;
		*frequency = *frequency <= 0 ? 48000 : *frequency;
		size = *samples * *channels * sizeof( int16_t );
		*buffer = malloc( size );
		if ( *buffer != NULL )
			memset( *buffer, 0, size );
		mlt_properties_set_data( properties, "audio", *buffer, size, free, NULL );
		mlt_properties_set_int( properties, "test_audio", 1 );
	}
	return 0;
}

void mlt_frame_close( mlt_frame this )
{
	mlt_properties_close( &this->parent );
	free( this );
}

/***** convenience functions *****/
#define RGB2YUV(r, g, b, y, u, v)\
  y = (306*r + 601*g + 117*b)  >> 10;\
  u = ((-172*r - 340*g + 512*b) >> 10)  + 128;\
  v = ((512*r - 429*g - 83*b) >> 10) + 128;\
  y = y < 0 ? 0 : y;\
  u = u < 0 ? 0 : u;\
  v = v < 0 ? 0 : v;\
  y = y > 255 ? 255 : y;\
  u = u > 255 ? 255 : u;\
  v = v > 255 ? 255 : v

int mlt_convert_rgb24a_to_yuv422( uint8_t *rgba, int width, int height, int stride, uint8_t *yuv, uint8_t *alpha )
{
	int ret = 0;
	register int y0, y1, u0, u1, v0, v1;
	register int r, g, b;
	register uint8_t *d = yuv;
	register int i, j;

	for ( i = 0; i < height; i++ )
	{
		register uint8_t *s = rgba + ( stride * i );
		for ( j = 0; j < ( width / 2 ); j++ )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			*alpha++ = *s++;
			RGB2YUV (r, g, b, y0, u0 , v0);
			r = *s++;
			g = *s++;
			b = *s++;
			*alpha++ = *s++;
			RGB2YUV (r, g, b, y1, u1 , v1);
			*d++ = y0;
			*d++ = (u0+u1) >> 1;
			*d++ = y1;
			*d++ = (v0+v1) >> 1;
		}
		if ( width % 2 )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			*alpha++ = *s++;
			RGB2YUV (r, g, b, y0, u0 , v0);
			*d++ = y0;
			*d++ = u0;
		}
	}
	return ret;
}

int mlt_convert_rgb24_to_yuv422( uint8_t *rgb, int width, int height, int stride, uint8_t *yuv )
{
	int ret = 0;
	register int y0, y1, u0, u1, v0, v1;
	register int r, g, b;
	register uint8_t *d = yuv;
	register int i, j;

	for ( i = 0; i < height; i++ )
	{
		register uint8_t *s = rgb + ( stride * i );
		for ( j = 0; j < ( width / 2 ); j++ )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			RGB2YUV (r, g, b, y0, u0 , v0);
			r = *s++;
			g = *s++;
			b = *s++;
			RGB2YUV (r, g, b, y1, u1 , v1);
			*d++ = y0;
			*d++ = (u0+u1) >> 1;
			*d++ = y1;
			*d++ = (v0+v1) >> 1;
		}
		if ( width % 2 )
		{
			r = *s++;
			g = *s++;
			b = *s++;
			RGB2YUV (r, g, b, y0, u0 , v0);
			*d++ = y0;
			*d++ = u0;
		}
	}
	return ret;
}

int mlt_convert_yuv420p_to_yuv422( uint8_t *yuv420p, int width, int height, int stride, uint8_t *yuv )
{
	int ret = 0;
	register int i, j;

	int half = width >> 1;

	uint8_t *Y = yuv420p;
	uint8_t *U = Y + width * height;
	uint8_t *V = U + width * height / 4;

	register uint8_t *d = yuv;

	for ( i = 0; i < height; i++ )
	{
		register uint8_t *u = U + ( i / 2 ) * ( half );
		register uint8_t *v = V + ( i / 2 ) * ( half );

		for ( j = 0; j < half; j++ )
		{
			*d ++ = *Y ++;
			*d ++ = *u ++;
			*d ++ = *Y ++;
			*d ++ = *v ++;
		}
	}
	return ret;
}

void mlt_resize_yuv422( uint8_t *output, int owidth, int oheight, uint8_t *input, int iwidth, int iheight )
{
	// Calculate strides
	int istride = iwidth * 2;
	int ostride = owidth * 2;

	iwidth = iwidth - ( iwidth % 4 );
	owidth = owidth - ( owidth % 4 );
	iheight = iheight - ( iheight % 2 );
	oheight = oheight - ( oheight % 2 );

	// Optimisation point
	if ( iwidth == owidth && iheight == oheight )
		memcpy( output, input, iheight * istride );

   	// Coordinates (0,0 is middle of output)
   	int y;

   	// Calculate ranges
   	int out_x_range = owidth / 2;
   	int out_y_range = oheight / 2;
   	int in_x_range = iwidth / 2 < out_x_range ? iwidth / 2 : out_x_range;
   	int in_y_range = iheight / 2 < out_y_range ? iheight / 2 : out_y_range;

   	// Output pointers
   	uint8_t *out_line = output;
   	uint8_t *out_ptr = out_line;

   	// Calculate a middle and possibly invalid pointer in the input
   	uint8_t *in_middle = input + istride * ( iheight / 2 ) + ( iwidth / 2 ) * 2;
   	int in_line = - in_y_range * istride - in_x_range * 2;

	uint8_t black[ 2 ] = { 0, 128 };
	int elements;

	// Fill whole section with black
	y = out_y_range - ( iheight / 2 );
	int blank_elements = ostride * y / 2;
	elements = blank_elements;
	while ( elements -- )
	{
		*out_line ++ = black[ 0 ];
		*out_line ++ = black[ 1 ];
	}

	int active_width = 2 * iwidth;
	int inactive_width = out_x_range - in_x_range;

   	// Loop for the entirety of our output height.
	while ( iheight -- )
   	{
       	// Start at the beginning of the line
       	out_ptr = out_line;

		// Fill the outer part with black
		elements = inactive_width;
		while ( elements -- )
		{
			*out_ptr ++ = black[ 0 ];
			*out_ptr ++ = black[ 1 ];
		}

   		// We're in the input range for this row.
		memcpy( out_ptr, in_middle + in_line, active_width );
		out_ptr += active_width;

		// Fill the outer part with black
		elements = inactive_width;
		while ( elements -- )
		{
			*out_ptr ++ = black[ 0 ];
			*out_ptr ++ = black[ 1 ];
		}

  		// Move to next input line
   		in_line += istride;

       	// Move to next output line
       	out_line += ostride;
   	}

	// Fill whole section with black
	elements = blank_elements;
	while ( elements -- )
	{
		*out_line ++ = black[ 0 ];
		*out_line ++ = black[ 1 ];
	}
}

/** A resizing function for yuv422 frames - this does not rescale, but simply
	resizes. It assumes yuv422 images available on the frame so use with care.
*/

uint8_t *mlt_frame_resize_yuv422( mlt_frame this, int owidth, int oheight )
{
	// Get properties
	mlt_properties properties = mlt_frame_properties( this );

	// Get the input image, width and height
	uint8_t *input = mlt_properties_get_data( properties, "image", NULL );
	int iwidth = mlt_properties_get_int( properties, "width" );
	int iheight = mlt_properties_get_int( properties, "height" );

	// If width and height are correct, don't do anything
	if ( iwidth != owidth || iheight != oheight )
	{
		// Create the output image
		// IRRIGATE ME
		uint8_t *output = malloc( owidth * ( oheight + 1 ) * 2 );

		// Call the generic resize
		mlt_resize_yuv422( output, owidth, oheight, input, iwidth, iheight );

		// Now update the frame
		mlt_properties_set_data( properties, "image", output, owidth * ( oheight + 1 ) * 2, free, NULL );
		mlt_properties_set_int( properties, "width", owidth );
		mlt_properties_set_int( properties, "height", oheight );

		// Return the output
		return output;
	}
	// No change, return input
	return input;
}

/** A rescaling function for yuv422 frames - low quality, and provided for testing
 	only. It assumes yuv422 images available on the frame so use with care.
*/

uint8_t *mlt_frame_rescale_yuv422( mlt_frame this, int owidth, int oheight )
{
	// Get properties
	mlt_properties properties = mlt_frame_properties( this );

	// Get the input image, width and height
	uint8_t *input = mlt_properties_get_data( properties, "image", NULL );
	int iwidth = mlt_properties_get_int( properties, "width" );
	int iheight = mlt_properties_get_int( properties, "height" );

	// If width and height are correct, don't do anything
	if ( iwidth != owidth || iheight != oheight )
	{
		// Create the output image
		// IRRIGATE ME
		uint8_t *output = malloc( owidth * ( oheight + 1 ) * 2 );

		// Calculate strides
		int istride = iwidth * 2;
		int ostride = owidth * 2;

		iwidth = iwidth - ( iwidth % 4 );

    	// Coordinates (0,0 is middle of output)
    	int y, x;

		// Derived coordinates
		int dy, dx;

    	// Calculate ranges
    	int out_x_range = owidth / 2;
    	int out_y_range = oheight / 2;
    	int in_x_range = iwidth / 2;
    	int in_y_range = iheight / 2;

    	// Output pointers
    	uint8_t *out_line = output;
    	uint8_t *out_ptr;

    	// Calculate a middle pointer
    	uint8_t *in_middle = input + istride * in_y_range + in_x_range * 2;
    	uint8_t *in_line;
		uint8_t *in_ptr;

		// Generate the affine transform scaling values
		int scale_width = ( iwidth << 16 ) / owidth;
		int scale_height = ( iheight << 16 ) / oheight;

    	// Loop for the entirety of our output height.
    	for ( y = - out_y_range; y < out_y_range ; y ++ )
    	{
			// Calculate the derived y value
			dy = ( scale_height * y ) >> 16;

        	// Start at the beginning of the line
        	out_ptr = out_line;
	
        	// Pointer to the middle of the input line
        	in_line = in_middle + dy * istride;
	
        	// Loop for the entirety of our output row.
        	for ( x = - out_x_range; x < out_x_range; x += 1 )
        	{
				// Calculated the derived x
				dx = ( scale_width * x ) >> 16;

               	// We're in the input range for this row.
				in_ptr = in_line + ( dx << 1 );
               	*out_ptr ++ = *in_ptr ++;
				in_ptr = in_line + ( ( dx >> 1 ) << 2 ) + ( ( x & 1 ) << 1 ) + 1;
               	*out_ptr ++ = *in_ptr;
        	}

        	// Move to next output line
        	out_line += ostride;
    	}

		// Now update the frame
		mlt_properties_set_data( properties, "image", output, owidth * oheight * 2, free, NULL );
		mlt_properties_set_int( properties, "width", owidth );
		mlt_properties_set_int( properties, "height", oheight );

		// Return the output
		return output;
	}

	// No change, return input
	return input;
}

int mlt_frame_mix_audio( mlt_frame this, mlt_frame that, float weight_start, float weight_end, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	int ret = 0;
	int16_t *p_src, *p_dest;
	int16_t *src, *dest;
	//static int16_t *extra_src = NULL, *extra_dest = NULL;
	static int extra_src_samples = 0, extra_dest_samples = 0;
	int frequency_src = *frequency, frequency_dest = *frequency;
	int channels_src = *channels, channels_dest = *channels;
	int samples_src = *samples, samples_dest = *samples;
	int i, j;
	double d = 0, s = 0;

	mlt_frame_get_audio( this, &p_dest, format, &frequency_dest, &channels_dest, &samples_dest );
	//fprintf( stderr, "mix: frame dest samples %d channels %d position %lld\n", samples_dest, channels_dest, mlt_properties_get_position( mlt_frame_properties( this ), "_position" ) );
	mlt_frame_get_audio( that, &p_src, format, &frequency_src, &channels_src, &samples_src );
	//fprintf( stderr, "mix: frame src  samples %d channels %d\n", samples_src, channels_src );
	src = p_src;
	dest = p_dest;
	if ( channels_src > 6 )
		channels_src = 0;
	if ( channels_dest > 6 )
		channels_dest = 0;
	if ( samples_src > 4000 )
		samples_src = 0;
	if ( samples_dest > 4000 )
		samples_dest = 0;

#if 0
	// Append new samples to leftovers
	if ( extra_dest_samples > 0 )
	{
		fprintf( stderr, "prepending %d samples to dest\n", extra_dest_samples );
		dest = realloc( extra_dest, ( samples_dest + extra_dest_samples ) * 2 * channels_dest );
		memcpy( &extra_dest[ extra_dest_samples * channels_dest ], p_dest, samples_dest * 2 * channels_dest );
	}
	else
		dest = p_dest;
	if ( extra_src_samples > 0 )
	{
		fprintf( stderr, "prepending %d samples to src\n", extra_src_samples );
		src = realloc( extra_src, ( samples_src + extra_src_samples ) * 2 * channels_src );
		memcpy( &extra_src[ extra_src_samples * channels_src ], p_src, samples_src * 2 * channels_src );
	}
	else
		src = p_src;
#endif

	// determine number of samples to process	
	if ( samples_src + extra_src_samples < samples_dest + extra_dest_samples )
		*samples = samples_src + extra_src_samples;
	else if ( samples_dest + extra_dest_samples < samples_src + extra_src_samples )
		*samples = samples_dest + extra_dest_samples;
	
	*channels = channels_src < channels_dest ? channels_src : channels_dest;
	*buffer = p_dest;
	*frequency = frequency_dest;

	// Compute a smooth ramp over start to end
	float weight = weight_start;
	float weight_step = ( weight_end - weight_start ) / *samples;

	// Mixdown
	for ( i = 0; i < *samples; i++ )
	{
		for ( j = 0; j < *channels; j++ )
		{
			if ( j < channels_dest )
				d = (double) dest[ i * channels_dest + j ];
			if ( j < channels_src )
				s = (double) src[ i * channels_src + j ];
			dest[ i * channels_dest + j ] = s * weight + d * ( 1.0 - weight );
		}
		weight += weight_step;
	}

	// We have to copy --sigh
	if ( dest != p_dest )
		memcpy( p_dest, dest, *samples * 2 * *channels );

#if 0
	// Store the leftovers
	if ( samples_src + extra_src_samples < samples_dest + extra_dest_samples )
	{
		extra_dest_samples = ( samples_dest + extra_dest_samples ) - ( samples_src + extra_src_samples );
		size_t size = extra_dest_samples * 2 * channels_dest;
		fprintf( stderr, "storing %d samples from dest\n", extra_dest_samples );
		if ( extra_dest )
			free( extra_dest );
		extra_dest = malloc( size );
		if ( extra_dest )
			memcpy( extra_dest, &p_dest[ ( samples_dest - extra_dest_samples - 1 ) * channels_dest ], size );
		else
			extra_dest_samples = 0;
	}
	else if ( samples_dest + extra_dest_samples < samples_src + extra_src_samples )
	{
		extra_src_samples = ( samples_src + extra_src_samples ) - ( samples_dest + extra_dest_samples );
		size_t size = extra_src_samples * 2 * channels_src;
		fprintf( stderr, "storing %d samples from src\n", extra_dest_samples );
		if ( extra_src )
			free( extra_src );
		extra_src = malloc( size );
		if ( extra_src )
			memcpy( extra_src, &p_src[ ( samples_src - extra_src_samples - 1 ) * channels_src ], size );
		else
			extra_src_samples = 0;
	}
#endif
	
	return ret;
}

int mlt_sample_calculator( float fps, int frequency, int64_t position )
{
	int samples = 0;

	if ( fps > 29 && fps <= 30 )
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

