/*
 * filter_greyscale.c -- greyscale filter
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

#include "filter_greyscale.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>

/** Greyscale class.
*/

typedef struct 
{
	struct mlt_filter_s parent;
}
filter_greyscale;

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_frame_get_image( this, image, format, width, height, 1 );
	uint8_t *p = *image;
	uint8_t *q = *image + *width * *height * 2;
	while ( p ++ != q )
		*p ++ = 128;
	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_greyscale_init( void *arg )
{
	filter_greyscale *this = calloc( sizeof( filter_greyscale ), 1 );
	if ( this != NULL )
	{
		mlt_filter filter = &this->parent;
		mlt_filter_init( filter, this );
		filter->process = filter_process;
	}
	return ( mlt_filter )this;
}

