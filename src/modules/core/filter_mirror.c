/*
 * filter_mirror.c -- mirror filter
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
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

	// Get the alpha
	uint8_t *alpha = mlt_frame_get_alpha_mask( frame );

	// If we have an image of the right colour space
	if ( error == 0 )
	{
		// We'll KISS here
		int hh = *height / 2;

		if ( !strcmp( mirror, "horizontal" ) )
		{
			uint8_t *p = NULL;
			uint8_t *q = NULL;
			uint8_t *a = NULL;
			uint8_t *b = NULL;
			int i;
			int uneven_w = ( *width % 2 ) * 2;
			for ( i = 0; i < *height; i ++ )
			{
				p = ( uint8_t * )*image + i * *width * 2;
				q = p + *width * 2;
				a = alpha + i * *width;
				b = a + *width - 1;
				if ( !reverse )
				{
					while ( p < q )
					{
						*p ++ = *( q - 2 );
						*p ++ = *( q - 3 - uneven_w );
						*p ++ = *( q - 4 );
						*p ++ = *( q - 1 - uneven_w );
						q -= 4;
						*a ++ = *b --;
						*a ++ = *b --;
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
						*b -- = *a ++;
						*b -- = *a ++;
					}
				}
			}
		}
		else if ( !strcmp( mirror, "vertical" ) )
		{
			uint16_t *end = ( uint16_t *)*image + *width * *height;
			uint16_t *p = NULL;
			uint16_t *q = NULL;
			uint8_t *a = NULL;
			uint8_t *b = NULL;
			int i;
			int j;
			for ( i = 0; i < hh; i ++ )
			{
				p = ( uint16_t * )*image + i * *width;
				q = end - ( i + 1 ) * *width;
				j = *width;
				a = alpha + i * *width;
				b = alpha + ( *height - i - 1 ) * *width;
				if ( !reverse )
				{
					while ( j -- )
					{
						*p ++ = *q ++;
						*a ++ = *b ++;
					}
				}
				else
				{
					while ( j -- )
					{
						*q ++ = *p ++;
						*b ++ = *a ++;
					}
				}
			}
		}
		else if ( !strcmp( mirror, "diagonal" ) )
		{
			uint8_t *end = ( uint8_t *)*image + *width * *height * 2;
			uint8_t *p = NULL;
			uint8_t *q = NULL;
			uint8_t *a = NULL;
			uint8_t *b = NULL;
			int i;
			int j;
			int uneven_w = ( *width % 2 ) * 2;
			for ( i = 0; i < *height; i ++ )
			{
				p = ( uint8_t * )*image + i * *width * 2;
				q = end - i * *width * 2;
				j = ( ( *width * ( *height - i ) ) / *height ) / 2;
				a = alpha + i * *width;
				b = alpha + ( *height - i - 1 ) * *width;
				if ( !reverse )
				{
					while ( j -- )
					{
						*p ++ = *( q - 2 );
						*p ++ = *( q - 3 - uneven_w );
						*p ++ = *( q - 4 );
						*p ++ = *( q - 1 - uneven_w );
						q -= 4;
						*a ++ = *b --;
						*a ++ = *b --;
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
						*b -- = *a ++;
						*b -- = *a ++;
					}
				}
			}
		}
		else if ( !strcmp( mirror, "xdiagonal" ) )
		{
			uint8_t *end = ( uint8_t *)*image + *width * *height * 2;
			uint8_t *p = NULL;
			uint8_t *q = NULL;
			int i;
			int j;
			uint8_t *a = NULL;
			uint8_t *b = NULL;
			int uneven_w = ( *width % 2 ) * 2;
			for ( i = 0; i < *height; i ++ )
			{
				p = ( uint8_t * )*image + ( i + 1 ) * *width * 2;
				q = end - ( i + 1 ) * *width * 2;
				j = ( ( *width * ( *height - i ) ) / *height ) / 2;
				a = alpha + ( i + 1 ) * *width - 1;
				b = alpha + ( *height - i - 1 ) * *width;
				if ( !reverse )
				{
					while ( j -- )
					{
						*q ++ = *( p - 2 );
						*q ++ = *( p - 3 - uneven_w );
						*q ++ = *( p - 4 );
						*q ++ = *( p - 1 - uneven_w );
						p -= 4;
						*b ++ = *a --;
						*b ++ = *a --;
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
						*a -- = *b ++;
						*a -- = *b ++;
					}
				}
			}
		}
		else if ( !strcmp( mirror, "flip" ) )
		{
			uint8_t t[ 4 ];
			uint8_t *p = NULL;
			uint8_t *q = NULL;
			int i;
			uint8_t *a = NULL;
			uint8_t *b = NULL;
			uint8_t c;
			int uneven_w = ( *width % 2 ) * 2;
			for ( i = 0; i < *height; i ++ )
			{
				p = ( uint8_t * )*image + i * *width * 2;
				q = p + *width * 2;
				a = alpha + i * *width;
				b = a + *width - 1;
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
					c = *a;
					*a ++ = *b;
					*b -- = c;
					c = *a;
					*a ++ = *b;
					*b -- = c;
				}
			}
		}
		else if ( !strcmp( mirror, "flop" ) )
		{
			uint16_t *end = ( uint16_t *)*image + *width * *height;
			uint16_t *p = NULL;
			uint16_t *q = NULL;
			uint16_t t;
			uint8_t *a = NULL;
			uint8_t *b = NULL;
			uint8_t c;
			int i;
			int j;
			for ( i = 0; i < hh; i ++ )
			{
				p = ( uint16_t * )*image + i * *width;
				q = end - ( i + 1 ) * *width;
				a = alpha + i * *width;
				b = alpha + ( *height - i - 1 ) * *width;
				j = *width;
				while ( j -- )
				{
					t = *p;
					*p ++ = *q;
					*q ++ = t;
					c = *a;
					*a ++ = *b;
					*b ++ = c;
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

