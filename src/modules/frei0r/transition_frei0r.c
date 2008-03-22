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
	
	if (*format!=mlt_image_yuv422 ){
		return -1;
	}
	
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );
	mlt_transition transition = mlt_frame_pop_service( a_frame );
	mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );
	mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );
	mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

	int invert = mlt_properties_get_int( properties, "invert" );

	if ( mlt_properties_get( a_props, "rescale.interp" ) == NULL || !strcmp( mlt_properties_get( a_props, "rescale.interp" ), "none" ) )
		mlt_properties_set( a_props, "rescale.interp", "nearest" );

	// set consumer_aspect_ratio for a and b frame
	if ( mlt_properties_get_double( a_props, "aspect_ratio" ) == 0.0 )
		mlt_properties_set_double( a_props, "aspect_ratio", mlt_properties_get_double( a_props, "consumer_aspect_ratio" ) );
	if ( mlt_properties_get_double( b_props, "aspect_ratio" ) == 0.0 )
		mlt_properties_set_double( b_props, "aspect_ratio", mlt_properties_get_double( a_props, "consumer_aspect_ratio" ) );
	mlt_properties_set_double( b_props, "consumer_aspect_ratio", mlt_properties_get_double( a_props, "consumer_aspect_ratio" ) );

	if ( mlt_properties_get( b_props, "rescale.interp" ) == NULL || !strcmp( mlt_properties_get( b_props, "rescale.interp" ), "none" ) )
                mlt_properties_set( b_props, "rescale.interp", "nearest" );
	
	uint8_t *images[]={NULL,NULL,NULL};

	mlt_frame_get_image( a_frame, &images[0], format, width, height, 1 );
	mlt_frame_get_image( b_frame, &images[1], format, width, height, 1 );
	
	mlt_position in = mlt_transition_get_in( transition );
	mlt_position out = mlt_transition_get_out( transition );

	// Get the position of the frame
	char *name = mlt_properties_get( MLT_TRANSITION_PROPERTIES( transition ), "_unique_id" );
	mlt_position position = mlt_properties_get_position( MLT_FRAME_PROPERTIES( a_frame ), name );

	float pos=( float )( position - in ) / ( float )( out - in + 1 );
	
	process_frei0r_item( transition_type , pos , properties, !invert ? a_frame : b_frame , images , format, width,height, writable );
	
	*width = mlt_properties_get_int( !invert ? a_props : b_props, "width" );
        *height = mlt_properties_get_int( !invert ? a_props : b_props, "height" );
	*image = mlt_properties_get_data( !invert ? a_props : b_props , "image", NULL );
	return 0;
}

mlt_frame transition_process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	char *name = mlt_properties_get( MLT_TRANSITION_PROPERTIES( transition ), "_unique_id" );
	mlt_properties_set_position( MLT_FRAME_PROPERTIES( a_frame ), name, mlt_frame_get_position( a_frame ) );
	mlt_frame_push_service( a_frame, transition );
	mlt_frame_push_frame( a_frame, b_frame );
	mlt_frame_push_get_image( a_frame, transition_get_image );
	return a_frame;
}

void transition_close( mlt_transition this ){
	destruct ( MLT_TRANSITION_PROPERTIES ( this ) );
}
