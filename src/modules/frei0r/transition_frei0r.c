/*
 * transition_frei0r.c -- frei0r transition
 * Copyright (c) 2008 Marco Gittler <g.marco@freenet.de>
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

#include <framework/mlt.h>
#include "frei0r_helper.h"
#include <string.h>

static int is_opaque( uint8_t *image, int width, int height )
{
	int pixels = width * height + 1;
	while ( --pixels ) {
		if ( image[3] != 0xff ) return 0;
		image += 4;
	}
	return 1;
}

static int transition_get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable ){
	
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );
	mlt_transition transition = mlt_frame_pop_service( a_frame );
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );
	int invert = mlt_properties_get_int( properties, "invert" );
	uint8_t *images[] = {NULL, NULL, NULL};
	int error = 0;

	// Get the B-frame.
	*format = mlt_image_rgb24a;
	error = mlt_frame_get_image( b_frame, &images[1], format, width, height, 0 );
	if ( error ) return error;

	// An optimization for cairoblend in normal (over) mode and opaque B frame.
	if ( !strcmp( "frei0r.cairoblend", mlt_properties_get( properties, "mlt_service" ) )
	     && ( !mlt_properties_get( properties, "0" ) || mlt_properties_get_double( properties, "0" ) == 1.0 )
	     && ( !mlt_properties_get( properties, "1" ) || !strcmp( "normal", mlt_properties_get( properties, "1" ) ) )
	    // Check if the alpha channel is entirely opaque.
	    && is_opaque( images[1], *width, *height ) )
	{
		if (invert)
			error = mlt_frame_get_image( a_frame, image, format, width, height, 0 );
		else
			*image = images[1];
	}
	else
	{
		error = mlt_frame_get_image( a_frame, &images[0], format, width, height, 0 );
		if ( error ) return error;

		double position = mlt_transition_get_position( transition, a_frame );
		mlt_profile profile = mlt_service_profile( MLT_TRANSITION_SERVICE( transition ) );
		double time = position / mlt_profile_fps( profile );
		process_frei0r_item( MLT_TRANSITION_SERVICE(transition), position, time, properties, !invert ? a_frame : b_frame, images, width, height );

		*width = mlt_properties_get_int( !invert ? a_props : b_props, "width" );
		*height = mlt_properties_get_int( !invert ? a_props : b_props, "height" );
		*image = mlt_properties_get_data( !invert ? a_props : b_props , "image", NULL );
	}
	return error;
}

mlt_frame transition_process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	mlt_frame_push_service( a_frame, transition );
	mlt_frame_push_frame( a_frame, b_frame );
	mlt_frame_push_get_image( a_frame, transition_get_image );
	return a_frame;
}

void transition_close( mlt_transition this ){
	destruct ( MLT_TRANSITION_PROPERTIES ( this ) );
}
