/*
 * filter_mirror.c -- mirror filter
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

#include "filter_mirror.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Pop the mirror filter from the stack
	mlt_filter this = mlt_frame_pop_service( frame );

	// Get the mirror type
	mlt_properties properties = mlt_filter_properties( this );

	// Get the properties
	char *mirror = mlt_properties_get( properties, "mirror" );

	// Determine if reverse is required
	int reverse = mlt_properties_get_int( properties, "reverse" );

	// Get the image
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// If we have an image of the right colour space
	if ( error == 0 && *format == mlt_image_yuv422 )
	{
		// We'll KISS here
		int hh = *height / 2;

		if ( !strcmp( mirror, "horizontal" ) )
		{
			uint8_t *p = NULL;
			uint8_t *q = NULL;
			int i;
			for ( i = 0; i < *height; i ++ )
			{
				p = ( uint8_t * )*image + i * *width * 2;
				q = p + *width * 2;
				if ( !reverse )
				{
					while ( p < q )
					{
						*p ++ = *( q - 2 );
						*p ++ = *( q - 3 );
						*p ++ = *( q - 4 );
						*p ++ = *( q - 1 );
						q -= 4;
					}
				}
				else
				{
					while ( p < q )
					{
						*( q - 2 ) = *p ++;
						*( q - 3 ) = *p ++;
						*( q - 4 ) = *p ++;
						*( q - 1 ) = *p ++;
						q -= 4;
					}
				}
			}
		}
		else if ( !strcmp( mirror, "vertical" ) )
		{
			uint16_t *end = ( uint16_t *)*image + *width * *height;
			uint16_t *p = NULL;
			uint16_t *q = NULL;
			int i;
			int j;
			for ( i = 0; i < hh; i ++ )
			{
				p = ( uint16_t * )*image + i * *width;
				q = end - ( i + 1 ) * *width;
				j = *width;
				if ( !reverse )
				{
					while ( j -- )
						*p ++ = *q ++;
				}
				else
				{
					while ( j -- )
						*q ++ = *p ++;
				}
			}
		}
		else if ( !strcmp( mirror, "diagonal" ) )
		{
			uint8_t *end = ( uint8_t *)*image + *width * *height * 2;
			uint8_t *p = NULL;
			uint8_t *q = NULL;
			int i;
			int j;
			for ( i = 0; i < *height; i ++ )
			{
				p = ( uint8_t * )*image + i * *width * 2;
				q = end - i * *width * 2;
				j = ( ( *width * ( *height - i ) ) / *height ) / 2;
				if ( !reverse )
				{
					while ( j -- )
					{
						*p ++ = *( q - 2 );
						*p ++ = *( q - 3 );
						*p ++ = *( q - 4 );
						*p ++ = *( q - 1 );
						q -= 4;
					}
				}
				else
				{
					while ( j -- )
					{
						*( q - 2 ) = *p ++;
						*( q - 3 ) = *p ++;
						*( q - 4 ) = *p ++;
						*( q - 1 ) = *p ++;
						q -= 4;
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
			for ( i = 0; i < *height; i ++ )
			{
				p = ( uint8_t * )*image + ( i + 1 ) * *width * 2;
				q = end - ( i + 1 ) * *width * 2;
				j = ( ( *width * ( *height - i ) ) / *height ) / 2;
				if ( !reverse )
				{
					while ( j -- )
					{
						*q ++ = *( p - 2 );
						*q ++ = *( p - 3 );
						*q ++ = *( p - 4 );
						*q ++ = *( p - 1 );
						p -= 4;
					}
				}
				else
				{
					while ( j -- )
					{
						*( p - 2 ) = *q ++;
						*( p - 3 ) = *q ++;
						*( p - 4 ) = *q ++;
						*( p - 1 ) = *q ++;
						p -= 4;
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
			for ( i = 0; i < *height; i ++ )
			{
				p = ( uint8_t * )*image + i * *width * 2;
				q = p + *width * 2;
				while ( p < q )
				{
					t[ 0 ] = p[ 0 ];
					t[ 1 ] = p[ 1 ];
					t[ 2 ] = p[ 2 ];
					t[ 3 ] = p[ 3 ];
					*p ++ = *( q - 2 );
					*p ++ = *( q - 3 );
					*p ++ = *( q - 4 );
					*p ++ = *( q - 1 );
					*( -- q ) = t[ 3 ];
					*( -- q ) = t[ 0 ];
					*( -- q ) = t[ 1 ];
					*( -- q ) = t[ 2 ];
				}
			}
		}
		else if ( !strcmp( mirror, "flop" ) )
		{
			uint16_t *end = ( uint16_t *)*image + *width * *height;
			uint16_t *p = NULL;
			uint16_t *q = NULL;
			uint16_t t;
			int i;
			int j;
			for ( i = 0; i < hh; i ++ )
			{
				p = ( uint16_t * )*image + i * *width;
				q = end - ( i + 1 ) * *width;
				j = *width;
				while ( j -- )
				{
					t = *p;
					*p ++ = *q;
					*q ++ = t;
				}
			}
		}
	}

	// Return the error
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	// Push the service on to the stack
	mlt_frame_push_service( frame, this );

	// Push the filter method on to the stack
	mlt_frame_push_service( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_mirror_init( void *arg )
{
	// Construct a new filter
	mlt_filter this = mlt_filter_new( );

	// If we have a filter, initialise it
	if ( this != NULL )
	{
		// Get the properties
		mlt_properties properties = mlt_filter_properties( this );

		// Set the default mirror type
		mlt_properties_set_or_default( properties, "mirror", arg, "horizontal" );

		// Assign the process method
		this->process = filter_process;
	}

	// Return the filter
	return this;
}

