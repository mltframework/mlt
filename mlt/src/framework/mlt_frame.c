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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	mlt_image_format vfmt;
	int width;
	int height;
	uint8_t *image;
	uint8_t *alpha;
	mlt_audio_format afmt;
	int16_t *audio;
}
frame_test;

static frame_test test_card = { mlt_image_none, 0, 0, NULL, NULL, mlt_audio_none, NULL };

/** Constructor for a frame.
*/

mlt_frame mlt_frame_init( )
{
	// Allocate a frame
	mlt_frame this = calloc( sizeof( struct mlt_frame_s ), 1 );

	if ( this != NULL )
	{
		// Initialise the properties
		mlt_properties properties = &this->parent;
		mlt_properties_init( properties, this );

		// Set default properties on the frame
		mlt_properties_set_timecode( properties, "timecode", 0.0 );
		mlt_properties_set_data( properties, "image", NULL, 0, NULL, NULL );
		mlt_properties_set_int( properties, "width", 720 );
		mlt_properties_set_int( properties, "height", 576 );
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
	return this->stack_get_image_size == 0;
}

/** Get the aspect ratio of the frame.
*/

double mlt_frame_get_aspect_ratio( mlt_frame this )
{
	mlt_properties properties = mlt_frame_properties( this );
	return mlt_properties_get_double( properties, "aspect_ratio" );
}

/** Set the aspect ratio of the frame.
*/

int mlt_frame_set_aspect_ratio( mlt_frame this, double value )
{
	mlt_properties properties = mlt_frame_properties( this );
	return mlt_properties_set_double( properties, "aspect_ratio", value );
}

/** Get the timecode of this frame.
*/

mlt_timecode mlt_frame_get_timecode( mlt_frame this )
{
	mlt_properties properties = mlt_frame_properties( this );
	return mlt_properties_get_timecode( properties, "timecode" );
}

/** Set the timecode of this frame.
*/

int mlt_frame_set_timecode( mlt_frame this, mlt_timecode value )
{
	mlt_properties properties = mlt_frame_properties( this );
	return mlt_properties_set_timecode( properties, "timecode", value );
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
	else
	{
		if ( test_card.vfmt != *format )
		{
			uint8_t *p;
			uint8_t *q;
			
			test_card.vfmt = *format;
			test_card.width = 720;
			test_card.height = 576;

			switch( *format )
			{
				case mlt_image_none:
					break;
				case mlt_image_rgb24:
					test_card.image = realloc( test_card.image, test_card.width * test_card.height * 3 );
					memset( test_card.image, 255, test_card.width * test_card.height * 3 );
					break;
				case mlt_image_rgb24a:
					test_card.image = realloc( test_card.image, test_card.width * test_card.height * 4 );
					memset( test_card.image, 255, test_card.width * test_card.height * 4 );
					break;
				case mlt_image_yuv422:
					test_card.image = realloc( test_card.image, test_card.width * test_card.height * 2 );
					p = test_card.image;
					q = test_card.image + test_card.width * test_card.height * 2;
					while ( p != q )
					{
						*p ++ = 255;
						*p ++ = 128;
					}
					break;
				case mlt_image_yuv420p:
					test_card.image = realloc( test_card.image, test_card.width * test_card.height * 3 / 2 );
					memset( test_card.image, 255, test_card.width * test_card.height * 3 / 2 );
					break;
			}
		}

		*width = test_card.width;
		*height = test_card.height;
		*buffer = test_card.image;
	}

	return 0;
}

uint8_t *mlt_frame_get_alpha_mask( mlt_frame this )
{
	if ( this->get_alpha_mask != NULL )
		return this->get_alpha_mask( this );
	return test_card.alpha;
}

int mlt_frame_get_audio( mlt_frame this, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	if ( this->get_audio != NULL )
	{
		return this->get_audio( this, buffer, format, frequency, channels, samples );
	}
	else
	{
		if ( test_card.afmt != *format )
		{
			test_card.afmt = *format;
			test_card.audio = realloc( test_card.audio, 1920 * 2 * sizeof( int16_t ) );
			memset( test_card.audio, 0, 1920 * 2 * sizeof( int16_t ) );
		}
		
		*buffer = test_card.audio;
		*frequency = 48000;
		*channels = 2;
		*samples = 1920;
	}
	return 0;
}

void mlt_frame_close( mlt_frame this )
{
	mlt_frame frame = mlt_frame_pop_frame( this );
	
	while ( frame != NULL )
	{
		mlt_frame_close( frame);
		frame = mlt_frame_pop_frame( this );
	}
	
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

int mlt_frame_composite_yuv( mlt_frame this, mlt_frame that, int x, int y, float weight )
{
	int ret = 0;
	int x_start = 0;
	int width_src, height_src;
	int width_dest, height_dest;
	mlt_image_format format_src, format_dest;
	uint8_t *p_src, *p_dest;
	int x_end;
	int i, j;
	int stride_src;
	int stride_dest;

	format_src = mlt_image_yuv422;
	format_dest = mlt_image_yuv422;
	
	mlt_frame_get_image( this, &p_dest, &format_dest, &width_dest, &height_dest, 1 /* writable */ );
	mlt_frame_get_image( that, &p_src, &format_src, &width_src, &height_src, 0 /* writable */ );
	
	x_end = width_src;
	
	stride_src = width_src * 2;
	stride_dest = width_dest * 2;
	uint8_t *lower = p_dest;
	uint8_t *upper = p_dest + height_dest * stride_dest;

	p_dest += ( y * stride_dest ) + ( x * 2 );

	if ( x < 0 )
	{
		x_start = -x;
		x_end += x_start;
	}

	uint8_t *z = mlt_frame_get_alpha_mask( that );

	for ( i = 0; i < height_src; i++ )
	{
		uint8_t *p = p_src;
		uint8_t *q = p_dest;
		uint8_t *o = p_dest;

		for ( j = 0; j < width_src; j ++ )
		{
			if ( q >= lower && q < upper && j >= x_start && j < x_end )
			{
				uint8_t y = *p ++;
				uint8_t uv = *p ++;
				uint8_t a = ( z == NULL ) ? 255 : *z ++;
				float value = ( weight * ( float ) a / 255.0 );
				*o ++ = (uint8_t)( y * value + *q++ * ( 1 - value ) );
				*o ++ = (uint8_t)( uv * value + *q++ * ( 1 - value ) );
			}
			else
			{
				p += 2;
				o += 2;
				q += 2;
				if ( z != NULL )
					z += 1;
			}
		}

		p_src += stride_src;
		p_dest += stride_dest;
	}

	return ret;
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
		uint8_t *output = malloc( owidth * oheight * 2 );

		// Calculate strides
		int istride = iwidth * 2;
		int ostride = owidth * 2;

    	// Coordinates (0,0 is middle of output)
    	int y, x;

    	// Calculate ranges
    	int out_x_range = owidth / 2;
    	int out_y_range = oheight / 2;
    	int in_x_range = iwidth / 2;
    	int in_y_range = iheight / 2;

    	// Output pointers
    	uint8_t *out_line = output;
    	uint8_t *out_ptr;

    	// Calculate a middle and possibly invalid pointer in the input
    	uint8_t *in_middle = input + istride * in_y_range + in_x_range * 2;
    	int in_line = - out_y_range * istride - out_x_range * 2;
    	int in_ptr;

    	// Loop for the entirety of our output height.
    	for ( y = - out_y_range; y < out_y_range ; y ++ )
    	{
        	// Start at the beginning of the line
        	out_ptr = out_line;
	
        	// Point the start of the current input line (NB: can be out of range)
        	in_ptr = in_line;
	
        	// Loop for the entirety of our output row.
        	for ( x = - out_x_range; x < out_x_range; x ++ )
        	{
            	// Check if x and y are in the valid input range.
            	if ( abs( x ) < in_x_range && abs( y ) < in_y_range  )
            	{
                	// We're in the input range for this row.
                	*out_ptr ++ = *( in_middle + in_ptr ++ );
                	*out_ptr ++ = *( in_middle + in_ptr ++ );
            	}
            	else
            	{
                	// We're not in the input range for this row.
                	*out_ptr ++ = 16;
                	*out_ptr ++ = 128;
                	in_ptr += 2;
            	}
        	}

        	// Move to next output line
        	out_line += ostride;

        	// Move to next input line
        	in_line += istride;
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
		uint8_t *output = malloc( owidth * oheight * 2 );

		// Calculate strides
		int istride = iwidth * 2;
		int ostride = owidth * 2;

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
		float scale_width = ( float )iwidth / ( float )owidth;
		float scale_height = ( float )iheight / ( float )oheight;

    	// Loop for the entirety of our output height.
    	for ( y = - out_y_range; y < out_y_range ; y ++ )
    	{
			// Calculate the derived y value
			dy = scale_height * y;

        	// Start at the beginning of the line
        	out_ptr = out_line;
	
        	// Pointer to the middle of the input line
        	in_line = in_middle + dy * istride;
	
        	// Loop for the entirety of our output row.
        	for ( x = - out_x_range; x < out_x_range; x += 2 )
        	{
				// Calculated the derived x
				dx = scale_width * x;

            	// Check if x and y are in the valid input range.
            	if ( abs( dx ) < in_x_range && abs( dy ) < in_y_range  )
            	{
                	// We're in the input range for this row.
					in_ptr = in_line + ( dx >> 1 ) * 4;
                	*out_ptr ++ = *in_ptr ++;
                	*out_ptr ++ = *in_ptr ++;
                	*out_ptr ++ = *in_ptr ++;
                	*out_ptr ++ = *in_ptr ++;
            	}
            	else
            	{
                	// We're not in the input range for this row.
                	*out_ptr ++ = 16;
                	*out_ptr ++ = 128;
                	*out_ptr ++ = 16;
                	*out_ptr ++ = 128;
            	}
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

