/*
 * filter_watermark.c -- watermark filter
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

#include "filter_watermark.h"

#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_transition.h>

#include <stdio.h>
#include <stdlib.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_properties frame_properties = mlt_frame_properties( this );
	mlt_filter filter = mlt_properties_get_data( frame_properties, "watermark", NULL );
	mlt_properties properties = mlt_filter_properties( filter );
	mlt_producer producer = mlt_properties_get_data( properties, "producer", NULL );
	mlt_transition composite = mlt_properties_get_data( properties, "composite", NULL );

	if ( composite == NULL )
	{
		char *geometry = mlt_properties_get( properties, "geometry" );
		composite = mlt_factory_transition( "composite", geometry == NULL ? "85%,5%:10%x10%" : geometry );
		if ( composite != NULL )
		{
			mlt_properties composite_properties = mlt_transition_properties( composite );
			char *distort = mlt_properties_get( properties, "distort" );
			if ( distort != NULL )
				mlt_properties_set( composite_properties, "distort", distort );
			mlt_properties_set_data( properties, "composite", composite, 0, ( mlt_destructor )mlt_transition_close, NULL );
		}
	}

	if ( producer == NULL )
	{
		char *resource = mlt_properties_get( properties, "resource" );
		char *factory = mlt_properties_get( properties, "factory" );
		producer = mlt_factory_producer( factory, resource );
		if ( producer != NULL )
		{
			mlt_properties producer_properties = mlt_producer_properties( producer );
			mlt_properties_set( producer_properties, "eof", "loop" );
			mlt_properties_set_data( properties, "producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );
		}
	}

	if ( composite != NULL && producer != NULL )
	{
		mlt_service service = mlt_producer_service( producer );
		mlt_frame b_frame = NULL;

		if ( mlt_service_get_frame( service, &b_frame, 0 ) == 0 )
			mlt_transition_process( composite, this, b_frame );

		error = mlt_frame_get_image( this, image, format, width, height, 1 );

		mlt_frame_close( b_frame );
	}
	else
	{
		error = mlt_frame_get_image( this, image, format, width, height, 1 );
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	mlt_properties properties = mlt_frame_properties( frame );
	mlt_properties_set_data( properties, "watermark", this, 0, NULL, NULL );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_watermark_init( void *arg )
{
	mlt_filter this = mlt_filter_new( );
	if ( this != NULL )
	{
		mlt_properties properties = mlt_filter_properties( this );
		this->process = filter_process;
		mlt_properties_set( properties, "factory", "fezzik" );
		if ( arg != NULL )
			mlt_properties_set( properties, "resource", arg );
	}
	return this;
}

