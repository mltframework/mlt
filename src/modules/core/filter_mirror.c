/*
 * filter_mirror.c -- mirror filter
 * Copyright (C) 2003-2021 Meltytech, LLC
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
#include <framework/mlt_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	struct mlt_image_s img;
	int i;

	// Pop the mirror filter from the stack
	mlt_filter filter = mlt_frame_pop_service( frame );

	// Get the mirror type
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

	// Get the properties
	char *mirror = mlt_properties_get( properties, "mirror" );

	// Determine if reverse is required
	int reverse = mlt_properties_get_int( properties, "reverse" );

	// Get the image
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// If we have an image of the right colour space
	if ( error == 0 && *format == mlt_image_yuv422 )
	{
		mlt_image_set_values( &img, *image, *format, *width, *height );
		if ( mlt_frame_get_alpha( frame ) )
		{
			img.planes[3] = mlt_frame_get_alpha( frame );
			img.strides[3] = img.width;
		}

		if ( !strcmp( mirror, "horizontal" ) )
		{
			int uneven_w = ( img.width % 2 ) * 2;
			for ( i = 0; i < img.height; i ++ )
			{
				uint8_t* p = img.planes[0] + img.strides[0] * i;
				uint8_t* q = p + img.width * 2;
				if ( !reverse )
				{
					while ( p < q )
					{
						*p ++ = *( q - 2 );
						*p ++ = *( q - 3 - uneven_w );
						*p ++ = *( q - 4 );
						*p ++ = *( q - 1 - uneven_w );
						q -= 4;
					}
				}
				else
				{
					while ( p < q )
					{
						*( q - 2 ) = *p ++;
						*( q - 3 - uneven_w ) = *p ++;
						*( q - 4 ) = *p ++;
						*( q - 1 - uneven_w ) = *p ++;
						q -= 4;
					}
				}
			}
			if ( img.planes[3] )
			{
				for ( i = 0; i < img.height; i ++ )
				{
					uint8_t* a = img.planes[3] + img.strides[3] * i;
					uint8_t* b = a + img.width - 1;
					if ( !reverse )
					{
						while ( a < b )
						{
							*a ++ = *b --;
							*a ++ = *b --;
						}
					}
					else
					{
						while ( a < b )
						{
							*b -- = *a ++;
							*b -- = *a ++;
						}
					}
				}
			}

		}
		else if ( !strcmp( mirror, "vertical" ) )
		{
			int hh = img.height / 2;
			for ( i = 0; i < hh; i ++ )
			{
				uint16_t* p = (uint16_t*)(img.planes[0] + (img.strides[0] * i));
				uint16_t* q = (uint16_t*)(img.planes[0] + (img.strides[0] * (img.height - i - 1)));
				int j = img.width;
				if ( !reverse )
				{
					while ( j -- )
					{
						*p ++ = *q ++;
					}
				}
				else
				{
					while ( j -- )
					{
						*q ++ = *p ++;
					}
				}
			}
			if ( img.planes[3] )
			{
				int j = img.width;
				uint8_t* a = img.planes[3] + (img.strides[3] * i);
				uint8_t* b = img.planes[3] + (img.strides[3] * (img.height - i - 1));
				if ( !reverse )
					while ( j -- )
						*a ++ = *b ++;
				else
					while ( j -- )
						*b ++ = *a ++;
			}
		}
		else if ( !strcmp( mirror, "diagonal" ) )
		{
			int uneven_w = ( img.width % 2 ) * 2;
			for ( i = 0; i < img.height; i ++ )
			{
				uint8_t* p = img.planes[0] + (img.strides[0] * i);
				uint8_t* q = img.planes[0] + (img.strides[0] * (img.height - i - 1));
				int j = ( ( img.width * ( img.height - i ) ) / img.height ) / 2;
				if ( !reverse )
				{
					while ( j -- )
					{
						*p ++ = *( q - 2 );
						*p ++ = *( q - 3 - uneven_w );
						*p ++ = *( q - 4 );
						*p ++ = *( q - 1 - uneven_w );
						q -= 4;
					}
				}
				else
				{
					while ( j -- )
					{
						*( q - 2 ) = *p ++;
						*( q - 3 - uneven_w ) = *p ++;
						*( q - 4 ) = *p ++;
						*( q - 1 - uneven_w ) = *p ++;
						q -= 4;
					}
				}
			}
			if ( img.planes[3] )
			{
				int i;
				for ( i = 0; i < img.height; i ++ )
				{
					int j = ( img.width * ( img.height - i ) ) / img.height;
					uint8_t* a = img.planes[3] + (img.strides[3] * i);
					uint8_t* b = img.planes[3] + (img.strides[3] * (img.height - i - 1));
					if ( !reverse )
						while ( j -- )
							*a ++ = *b --;
					else
						while ( j -- )
							*b -- = *a ++;
				}
			}
		}
		else if ( !strcmp( mirror, "xdiagonal" ) )
		{
			int uneven_w = ( img.width % 2 ) * 2;
			for ( i = 0; i < img.height; i ++ )
			{
				uint8_t* p = img.planes[0] + (img.strides[0] * (i + 1));
				uint8_t* q = img.planes[0] + (img.strides[0] * (img.height - i));
				int j = ( ( img.width * ( img.height - i ) ) / img.height ) / 2;
				if ( !reverse )
				{
					while ( j -- )
					{
						*q ++ = *( p - 2 );
						*q ++ = *( p - 3 - uneven_w );
						*q ++ = *( p - 4 );
						*q ++ = *( p - 1 - uneven_w );
						p -= 4;
					}
				}
				else
				{
					while ( j -- )
					{
						*( p - 2 ) = *q ++;
						*( p - 3 - uneven_w ) = *q ++;
						*( p - 4 ) = *q ++;
						*( p - 1 - uneven_w ) = *q ++;
						p -= 4;
					}
				}
			}
			if ( img.planes[3] )
			{
				int i;
				for ( i = 0; i < img.height; i ++ )
				{
					int j = ( ( img.width * ( img.height - i ) ) / img.height );
					uint8_t* a = img.planes[3] + (img.strides[3] * i) + img.width - 1;
					uint8_t* b = img.planes[3] + (img.strides[3] * (img.height - i - 1));
					if ( !reverse )
						while ( j -- )
							*b ++ = *a --;
					else
						while ( j -- )
							*a -- = *b ++;
				}
			}
		}
		else if ( !strcmp( mirror, "flip" ) )
		{
			uint8_t t[ 4 ];
			int uneven_w = ( img.width % 2 ) * 2;
			for ( i = 0; i < img.height; i ++ )
			{
				uint8_t* p = img.planes[0] + (img.strides[0] * i);
				uint8_t* q = p + *width * 2;
				while ( p < q )
				{
					t[ 0 ] = p[ 0 ];
					t[ 1 ] = p[ 1 + uneven_w ];
					t[ 2 ] = p[ 2 ];
					t[ 3 ] = p[ 3 + uneven_w ];
					*p ++ = *( q - 2 );
					*p ++ = *( q - 3 - uneven_w );
					*p ++ = *( q - 4 );
					*p ++ = *( q - 1 - uneven_w );
					*( -- q ) = t[ 3 ];
					*( -- q ) = t[ 0 ];
					*( -- q ) = t[ 1 ];
					*( -- q ) = t[ 2 ];
				}
			}
			if ( img.planes[3] )
			{
				uint8_t c;
				for ( i = 0; i < img.height; i ++ )
				{
					uint8_t* a = img.planes[3] + (img.strides[3] * i);
					uint8_t* b = a + img.width - 1;
					while ( a < b )
					{
						c = *a;
						*a ++ = *b;
						*b -- = c;
					}
				}
			}
		}
		else if ( !strcmp( mirror, "flop" ) )
		{
			uint16_t t;
			int hh = *height / 2;
			for ( i = 0; i < hh; i ++ )
			{
				uint16_t* p = (uint16_t*)(img.planes[0] + (img.strides[0] * i));
				uint16_t* q = (uint16_t*)(img.planes[0] + (img.strides[0] * (img.height - i - 1)));
				int j = img.width;
				while ( j -- )
				{
					t = *p;
					*p ++ = *q;
					*q ++ = t;
				}
			}
			if ( img.planes[3] )
			{
				uint8_t c;
				for ( i = 0; i < img.height; i ++ )
				{
					uint8_t* a = img.planes[3] + (img.strides[3] * i);
					uint8_t* b = img.planes[3] + (img.strides[3] * (img.height - i - 1));
					while ( a < b )
					{
						c = *a;
						*a ++ = *b;
						*b -- = c;
					}
				}
			}
		}
	}

	// Return the error
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	// Push the service on to the stack
	mlt_frame_push_service( frame, filter );

	// Push the filter method on to the stack
	mlt_frame_push_service( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_mirror_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Construct a new filter
	mlt_filter filter = mlt_filter_new( );

	// If we have a filter, initialise it
	if ( filter != NULL )
	{
		// Get the properties
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

		// Set the default mirror type
		mlt_properties_set_or_default( properties, "mirror", arg, "horizontal" );

		// Assign the process method
		filter->process = filter_process;
	}

	// Return the filter
	return filter;
}

