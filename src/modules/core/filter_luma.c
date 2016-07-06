/*
 * filter_luma.c -- luma filter
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
#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>
#include <framework/mlt_transition.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Do it :-).
*/

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	mlt_transition luma = mlt_properties_get_data( properties, "luma", NULL );
	mlt_frame b_frame = mlt_properties_get_data( properties, "frame", NULL );
	mlt_properties b_frame_props = b_frame ? MLT_FRAME_PROPERTIES( b_frame ) : NULL;
	int out = mlt_properties_get_int( properties, "period" );
	int cycle = mlt_properties_get_int( properties, "cycle" );
	int duration = mlt_properties_get_int( properties, "duration" );
	mlt_position position = mlt_filter_get_position( filter, frame );

	out = out? out + 1 : 25;
	if ( cycle )
		out = cycle;
	if ( duration < 1 || duration > out )
		duration = out;
	*format = mlt_image_yuv422;

	if ( b_frame == NULL || mlt_properties_get_int( b_frame_props, "width" ) != *width || mlt_properties_get_int( b_frame_props, "height" ) != *height )
	{
		b_frame = mlt_frame_init( MLT_FILTER_SERVICE( filter ) );
		mlt_properties_set_data( properties, "frame", b_frame, 0, ( mlt_destructor )mlt_frame_close, NULL );
	}

	if ( luma == NULL )
	{
		char *resource = mlt_properties_get( properties, "resource" );
		mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
		luma = mlt_factory_transition( profile, "luma", resource );
		if ( luma != NULL )
		{
			mlt_properties luma_properties = MLT_TRANSITION_PROPERTIES( luma );
			mlt_properties_set_int( luma_properties, "in", 0 );
			mlt_properties_set_int( luma_properties, "out", duration - 1 );
			mlt_properties_set_int( luma_properties, "reverse", 1 );
			mlt_properties_set_data( properties, "luma", luma, 0, ( mlt_destructor )mlt_transition_close, NULL );
		}
	}

	mlt_position modulo_pos = MLT_POSITION_MOD(position, out);
	mlt_log_debug( MLT_FILTER_SERVICE(filter), "pos " MLT_POSITION_FMT " mod period " MLT_POSITION_FMT "\n", position, modulo_pos );
	if ( luma != NULL &&
	     ( mlt_properties_get( properties, "blur" ) != NULL ||
		   ( position >= duration && modulo_pos < duration - 1 ) ) )
	{
		mlt_properties luma_properties = MLT_TRANSITION_PROPERTIES( luma );
		mlt_properties_pass( luma_properties, properties, "luma." );
		int in = position / out * out + mlt_frame_get_position( frame ) - position;
		mlt_properties_set_int( luma_properties, "in", in );
		mlt_properties_set_int( luma_properties, "out", in + duration - 1 );
		mlt_transition_process( luma, frame, b_frame );
	}

	error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// We only need a copy of the last frame in the cycle, but we could miss it
	// with realtime frame-dropping, so we copy the last several frames of the cycle.
	if ( error == 0 && modulo_pos > out - duration )
	{
		mlt_properties a_props = MLT_FRAME_PROPERTIES( frame );
		int size = 0;
		uint8_t *src = mlt_properties_get_data( a_props, "image", &size );
		uint8_t *dst = mlt_pool_alloc( size );

		if ( dst != NULL )
		{
			mlt_log_debug( MLT_FILTER_SERVICE(filter), "copying frame " MLT_POSITION_FMT "\n", modulo_pos );
			mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );
			memcpy( dst, src, size );
			mlt_frame_set_image( b_frame, dst, size, mlt_pool_release );
			mlt_properties_set_int( b_props, "width", *width );
			mlt_properties_set_int( b_props, "height", *height );
			mlt_properties_set_int( b_props, "format", *format );
		}
	}

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	// Push the filter on to the stack
	mlt_frame_push_service( frame, filter );

	// Push the get_image on to the stack
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_luma_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		filter->process = filter_process;
		if ( arg != NULL )
			mlt_properties_set( properties, "resource", arg );
	}
	return filter;
}

