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
#include <framework/mlt_slices.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	mlt_image image;
	char* mirror;
	int reverse;
} slice_desc;

static int do_slice_proc(int id, int index, int jobs, void* data)
{
	(void) id; // unused
	slice_desc* desc = (slice_desc*) data;
	int slice_line_start, slice_height = mlt_slices_size_slice(jobs, index, desc->image->height, &slice_line_start);
	int slice_line_end = slice_line_start + slice_height;
	int slice_line_start_half, slice_height_half = mlt_slices_size_slice(jobs, index, desc->image->height / 2, &slice_line_start_half);
	int slice_line_end_half = slice_line_start_half + slice_height_half;
	int uneven_w = ( desc->image->width % 2 ) * 2;
	int i;

	if ( !strcmp( desc->mirror, "horizontal" ) )
	{
		for ( i = slice_line_start; i < slice_line_end; i++ )
		{
			uint8_t* p = desc->image->planes[0] + desc->image->strides[0] * i;
			uint8_t* q = p + desc->image->width * 2;
			if ( !desc->reverse )
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
		if ( desc->image->planes[3] )
		{
			for ( i = slice_line_start; i < slice_line_end; i++ )
			{
				uint8_t* a = desc->image->planes[3] + desc->image->strides[3] * i;
				uint8_t* b = a + desc->image->width - 1;
				if ( !desc->reverse )
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
	else if ( !strcmp( desc->mirror, "vertical" ) )
	{
		for ( i = slice_line_start_half; i < slice_line_end_half; i ++ )
		{
			uint16_t* p = (uint16_t*)(desc->image->planes[0] + (desc->image->strides[0] * i));
			uint16_t* q = (uint16_t*)(desc->image->planes[0] + (desc->image->strides[0] * (desc->image->height - i - 1)));
			int j = desc->image->width;
			if ( !desc->reverse )
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
		if ( desc->image->planes[3] )
		{
			for ( i = slice_line_start_half; i < slice_line_end_half; i ++ )
			{
				int j = desc->image->width;
				uint8_t* a = desc->image->planes[3] + (desc->image->strides[3] * i);
				uint8_t* b = desc->image->planes[3] + (desc->image->strides[3] * (desc->image->height - i - 1));
				if ( !desc->reverse )
					while ( j -- )
						*a ++ = *b ++;
				else
					while ( j -- )
						*b ++ = *a ++;
			}
		}
	}
	else if ( !strcmp( desc->mirror, "diagonal" ) )
	{
		for ( i = slice_line_start; i < slice_line_end; i ++ )
		{
			uint8_t* p = desc->image->planes[0] + (desc->image->strides[0] * i);
			uint8_t* q = desc->image->planes[0] + (desc->image->strides[0] * (desc->image->height - i - 1));
			int j = ( ( desc->image->width * ( desc->image->height - i ) ) / desc->image->height ) / 2;
			if ( !desc->reverse )
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
		if ( desc->image->planes[3] )
		{
			int i;
			for ( i = slice_line_start; i < slice_line_end; i ++ )
			{
				int j = ( desc->image->width * ( desc->image->height - i ) ) / desc->image->height;
				uint8_t* a = desc->image->planes[3] + (desc->image->strides[3] * i);
				uint8_t* b = desc->image->planes[3] + (desc->image->strides[3] * (desc->image->height - i - 1));
				if ( !desc->reverse )
					while ( j -- )
						*a ++ = *b --;
				else
					while ( j -- )
						*b -- = *a ++;
			}
		}
	}
	else if ( !strcmp( desc->mirror, "xdiagonal" ) )
	{
		for ( i = slice_line_start; i < slice_line_end; i ++ )
		{
			uint8_t* p = desc->image->planes[0] + (desc->image->strides[0] * (i + 1));
			uint8_t* q = desc->image->planes[0] + (desc->image->strides[0] * (desc->image->height - i));
			int j = ( ( desc->image->width * ( desc->image->height - i ) ) / desc->image->height ) / 2;
			if ( !desc->reverse )
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
		if ( desc->image->planes[3] )
		{
			int i;
			for ( i = slice_line_start; i < slice_line_end; i ++ )
			{
				int j = ( ( desc->image->width * ( desc->image->height - i ) ) / desc->image->height );
				uint8_t* a = desc->image->planes[3] + (desc->image->strides[3] * i) + desc->image->width - 1;
				uint8_t* b = desc->image->planes[3] + (desc->image->strides[3] * (desc->image->height - i - 1));
				if ( !desc->reverse )
					while ( j -- )
						*b ++ = *a --;
				else
					while ( j -- )
						*a -- = *b ++;
			}
		}
	}
	else if ( !strcmp( desc->mirror, "flip" ) )
	{
		uint8_t t[ 4 ];
		for ( i = slice_line_start; i < slice_line_end; i ++ )
		{
			uint8_t* p = desc->image->planes[0] + (desc->image->strides[0] * i);
			uint8_t* q = p + desc->image->width * 2;
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
		if ( desc->image->planes[3] )
		{
			uint8_t c;
			for ( i = slice_line_start; i < slice_line_end; i ++ )
			{
				uint8_t* a = desc->image->planes[3] + (desc->image->strides[3] * i);
				uint8_t* b = a + desc->image->width - 1;
				while ( a < b )
				{
					c = *a;
					*a ++ = *b;
					*b -- = c;
				}
			}
		}
	}
	else if ( !strcmp( desc->mirror, "flop" ) )
	{
		uint16_t t;
		for ( i = slice_line_start_half; i < slice_line_end_half; i ++ )
		{
			uint16_t* p = (uint16_t*)(desc->image->planes[0] + (desc->image->strides[0] * i));
			uint16_t* q = (uint16_t*)(desc->image->planes[0] + (desc->image->strides[0] * (desc->image->height - i - 1)));
			int j = desc->image->width;
			while ( j -- )
			{
				t = *p;
				*p ++ = *q;
				*q ++ = t;
			}
		}
		if ( desc->image->planes[3] )
		{
			uint8_t c;
			for ( i = slice_line_start_half; i < slice_line_end_half; i ++ )
			{
				uint8_t* a = desc->image->planes[3] + (desc->image->strides[3] * i);
				uint8_t* b = desc->image->planes[3] + (desc->image->strides[3] * (desc->image->height - i - 1));
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

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Pop the mirror filter from the stack
	mlt_filter filter = mlt_frame_pop_service( frame );

	// Get the image
	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// If we have an image of the right colour space
	if ( error == 0 && *format == mlt_image_yuv422 )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		slice_desc desc;
		struct mlt_image_s img;
		mlt_image_set_values( &img, *image, *format, *width, *height );
		if ( mlt_frame_get_alpha( frame ) )
		{
			img.planes[3] = mlt_frame_get_alpha( frame );
			img.strides[3] = img.width;
		}
		desc.image = &img;
		desc.mirror = mlt_properties_get( properties, "mirror" );
		desc.reverse = mlt_properties_get_int( properties, "reverse" );
		mlt_slices_run_normal(0, do_slice_proc, &desc);
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

