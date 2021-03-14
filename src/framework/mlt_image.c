/**
 * \file mlt_image.c
 * \brief Image class
 * \see mlt_mlt_image_s
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

#include "mlt_image.h"

#include "mlt_log.h"

#include <stdlib.h>
#include <string.h>

/** Allocate a new Image object.
 *
 * \return a new image object with default values set
 */

mlt_image mlt_image_new()
{
	mlt_image self = calloc( 1, sizeof(struct mlt_image_s) );
	self->close = free;
	return self;
}

/** Destroy an image object created by mlt_image_new().
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 */

void mlt_image_close( mlt_image self )
{
	if ( self)
	{
		if ( self->release_data )
		{
			self->release_data( self->data );
		}
		if ( self->release_alpha )
		{
			self->release_alpha( self->alpha );
		}
		if ( self->close )
		{
			self->close( self );
		}
	}
}

/** Set the most common values for the image.
 *
 * Less common values will be set to reasonable defaults.
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 * \param data the buffer that contains the image data
 * \param format the image format
 * \param width the width of the image
 * \param height the height of the image
 */

void mlt_image_set_values( mlt_image self, void* data, mlt_image_format format, int width, int height )
{
	self->data = data;
	self->format = format;
	self->width = width;
	self->height = height;
	self->colorspace = mlt_colorspace_unspecified;
	self->release_data = NULL;
	self->release_alpha = NULL;
	self->close = NULL;
	mlt_image_format_planes( self->format, self->width, self->height, self->data, self->planes, self->strides );
}

/** Get the most common values for the image.
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 * \param[out] data the buffer that contains the image data
 * \param[out] format the image format
 * \param[out] width the width of the image
 * \param[out] height the height of the image
 */

void mlt_image_get_values( mlt_image self, void** data, mlt_image_format* format, int* width, int* height )
{
	*data = self->data;
	*format = self->format;
	*width = self->width;
	*height = self->height;
}

/** Allocate the data field based on the other properties of the Image.
 *
 * If the data field is already set, and a destructor function exists, the data
 * will be released. Else, the data pointer will be overwritten without being
 * released.
 *
 * After this function call, the release_data field will be set and can be used
 * to release the data when necessary.
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 */

void mlt_image_alloc_data( mlt_image self )
{
	if ( !self ) return;

	if ( self->release_data )
	{
		self->release_data( self->data );
	}

	int size = mlt_image_calculate_size( self );
	self->data = mlt_pool_alloc( size );
	self->release_data = mlt_pool_release;
	mlt_image_format_planes( self->format, self->width, self->height, self->data, self->planes, self->strides );
}

/** Allocate the alpha field based on the other properties of the Image.
 *
 * If the alpha field is already set, and a destructor function exists, the data
 * will be released. Else, the data pointer will be overwritten without being
 * released.
 *
 * After this function call, the release_data field will be set and can be used
 * to release the data when necessary.
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 */

void mlt_image_alloc_alpha( mlt_image self )
{
	if ( !self ) return;

	if ( self->release_alpha )
	{
		self->release_alpha( self->alpha );
	}

	self->alpha = mlt_pool_alloc( self->width * self->height );
	self->release_alpha = mlt_pool_release;
	self->strides[3] = self->width;
	self->planes[3] = self->alpha;
}

/** Calculate the number of bytes needed for the Image data.
 *
 * \public \memberof mlt_image_s
 * \param self the Image object
 * \return the number of bytes
 */

int mlt_image_calculate_size( mlt_image self )
{
	switch ( self->format )
	{
		case mlt_image_rgb:
			return self->width * self->height * 3;
		case mlt_image_rgba:
			return self->width * self->height * 4;
		case mlt_image_yuv422:
			return self->width * self->height * 2;
		case mlt_image_yuv420p:
			return self->width * self->height * 3 / 2;
		case mlt_image_movit:
		case mlt_image_opengl_texture:
			return 4;
		case mlt_image_yuv422p16:
			return 4 * self->width * self->height;
	}
	return 0;
}


/** Get the short name for an image format.
 *
 * \public \memberof mlt_image_s
 * \param format the image format
 * \return a string
 */

const char * mlt_image_format_name( mlt_image_format format )
{
	switch ( format )
	{
		case mlt_image_none:           return "none";
		case mlt_image_rgb:            return "rgb";
		case mlt_image_rgba:           return "rgba";
		case mlt_image_yuv422:         return "yuv422";
		case mlt_image_yuv420p:        return "yuv420p";
		case mlt_image_movit:          return "glsl";
		case mlt_image_opengl_texture: return "opengl_texture";
		case mlt_image_yuv422p16:      return "yuv422p16";
		case mlt_image_invalid:        return "invalid";
	}
	return "invalid";
}

/** Get the id of image format from short name.
 *
 * \public \memberof mlt_image_s
 * \param name the image format short name
 * \return a image format
 */

mlt_image_format mlt_image_format_id( const char * name )
{
	mlt_image_format f;

	for ( f = mlt_image_none; name && f < mlt_image_invalid; f++ )
	{
		const char * v = mlt_image_format_name( f );
		if ( !strcmp( v, name ) )
			return f;
	}

	return mlt_image_invalid;
}

/** Fill an image with black.
  *
  * \public \memberof mlt_image_s
  */
void mlt_image_fill_black( mlt_image self )
{
	if ( !self->data) return;

	switch(  self->format )
	{
		case mlt_image_none:
		case mlt_image_movit:
		case mlt_image_opengl_texture:
			return;
		case mlt_image_rgb:
		case mlt_image_rgba:
		{
			int size = mlt_image_calculate_size( self );
			memset( self->planes[0], 255, size );
			break;
		}
		case mlt_image_yuv422:
		{
			int size = mlt_image_calculate_size( self );
			register uint8_t *p = self->planes[0];
			register uint8_t *q = p + size;
			while ( p != NULL && p != q )
			{
				*p ++ = 235;
				*p ++ = 128;
			}
		}
		break;
		case mlt_image_yuv422p16:
		{
			for ( int plane = 0; plane < 3; plane++ )
			{
				uint16_t value = 235 << 8;
				size_t width = self->width;
				if ( plane > 0 )
				{
					value = 128 << 8;
					width = self->width / 2;
				}
				uint16_t* pRow = (uint16_t*)self->planes[plane];
				for ( int i = 0; i < self->height; i++ )
				{
					uint16_t* p = pRow;
					for ( int j = 0; j < width; j++ )
					{
						*p++ = value;
					}
					pRow += self->strides[plane];
				}
			}
		}
		break;
		case mlt_image_yuv420p:
		{
			memset(self->planes[0], 235, self->height * self->strides[0]);
			memset(self->planes[1], 128, self->height * self->strides[1] / 2);
			memset(self->planes[2], 128, self->height * self->strides[2] / 2);
		}
		break;
	}

 }

/** Get the number of bytes needed for an image.
  *
  * \public \memberof mlt_image_s
  * \param format the image format
  * \param width width of the image in pixels
  * \param height height of the image in pixels
  * \param[out] bpp the number of bytes per pixel (optional)
  * \return the number of bytes
  */
int mlt_image_format_size( mlt_image_format format, int width, int height, int *bpp )
{
	switch ( format )
	{
		case mlt_image_rgb:
			if ( bpp ) *bpp = 3;
			return width * height * 3;
		case mlt_image_rgba:
			if ( bpp ) *bpp = 4;
			return width * height * 4;
		case mlt_image_yuv422:
			if ( bpp ) *bpp = 2;
			return width * height * 2;
		case mlt_image_yuv420p:
			if ( bpp ) *bpp = 3 / 2;
			return width * height * 3 / 2;
		case mlt_image_movit:
		case mlt_image_opengl_texture:
			if ( bpp ) *bpp = 0;
			return 4;
		case mlt_image_yuv422p16:
			if ( bpp ) *bpp = 0;
			return 4 * height * width ;
		default:
			if ( bpp ) *bpp = 0;
			return 0;
	}
	return 0;
}

/** Build a planes pointers of image mapping
 *
 * For proper and unified planar image processing, planes sizes and planes pointers should
 * be provides to processing code.
 *
 * \public \memberof mlt_image_s
 * \param format the image format
 * \param width width of the image in pixels
 * \param height height of the image in pixels
 * \param[in] data pointer to allocated image
 * \param[out] planes pointers to plane's pointers will be set
 * \param[out] strides pointers to plane's strides will be set
 */
void mlt_image_format_planes( mlt_image_format format, int width, int height, void* data, uint8_t* planes[4], int strides[4])
{
	if ( mlt_image_yuv422p16 == format )
	{
		strides[0] = width * 2;
		strides[1] = width;
		strides[2] = width;
		strides[3] = 0;

		planes[0] = (unsigned char*)data;
		planes[1] = planes[0] + height * strides[0];
		planes[2] = planes[1] + height * strides[1];
		planes[3] = 0;
	}
	else if ( mlt_image_yuv420p == format )
	{
		strides[0] = width;
		strides[1] = width >> 1;
		strides[2] = width >> 1;
		strides[3] = 0;

		planes[0] = (unsigned char*)data;
		planes[1] = (unsigned char*)data + width * height;
		planes[2] = (unsigned char*)data + ( 5 * width * height ) / 4;
		planes[3] = 0;
	}
	else
	{
		int bpp;

		mlt_image_format_size( format, width, height, &bpp );

		planes[0] = data;
		planes[1] = 0;
		planes[2] = 0;
		planes[3] = 0;

		strides[0] = bpp * width;
		strides[1] = 0;
		strides[2] = 0;
		strides[3] = 0;
	};
}
