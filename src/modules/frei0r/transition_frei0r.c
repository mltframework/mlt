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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt.h>
#include "frei0r_helper.h"
#include <string.h>
 
static int transition_get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable ){
	
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );
	mlt_transition transition = mlt_frame_pop_service( a_frame );
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	int invert = mlt_properties_get_int( properties, "invert" );

	uint8_t *images[]={NULL,NULL,NULL};

	*format = mlt_image_rgb24a;
	mlt_frame_get_image( a_frame, &images[0], format, width, height, 0 );
	mlt_frame_get_image( b_frame, &images[1], format, width, height, 0 );
	
	double position = mlt_transition_get_position( transition, a_frame );
	mlt_profile profile = mlt_service_profile( MLT_TRANSITION_SERVICE( transition ) );
	double time = position / mlt_profile_fps( profile );
	process_frei0r_item( MLT_TRANSITION_SERVICE(transition), position, time, properties, !invert ? a_frame : b_frame, images, width, height );
	
	*width = mlt_properties_get_int( !invert ? a_props : b_props, "width" );
        *height = mlt_properties_get_int( !invert ? a_props : b_props, "height" );
	*image = mlt_properties_get_data( !invert ? a_props : b_props , "image", NULL );
	return 0;
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
