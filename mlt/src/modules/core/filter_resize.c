/*
 * filter_resize.c -- resizing filter
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

#include "filter_resize.h"

#include <framework/mlt_frame.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int owidth = *width;
	int oheight = *height;
	mlt_frame_get_image( this, image, format, &owidth, &oheight, 0 );
	if ( *width == 0 )
		*width = 720;
	if ( *height == 0 )
		*height = 576;
	if ( !strcmp( mlt_properties_get( mlt_frame_properties( this ), "resize.scale" ), "affine" ) )
		*image = mlt_frame_rescale_yuv422( this, *width, *height );
	else
		*image = mlt_frame_resize_yuv422( this, *width, *height );
	return 0;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_frame_push_get_image( frame, filter_get_image );
	mlt_properties_set( mlt_frame_properties( frame ), "resize.scale", mlt_properties_get( mlt_filter_properties( this ), "scale" ) );
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_resize_init( char *arg )
{
	mlt_filter this = calloc( sizeof( struct mlt_filter_s ), 1 );
	if ( mlt_filter_init( this, this ) == 0 )
	{
		this->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set( mlt_filter_properties( this ), "scale", arg );
		else
			mlt_properties_set( mlt_filter_properties( this ), "scale", "off" );
	}
	return this;
}

