/*
 * filter_deshake.cpp
 * Copyright (C) 2013
 * Marco Gittler <g.marco@freenet.de>
 * Jakub Ksiezniak <j.ksiezniak@gmail.com>
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

extern "C"
{
#include <vid.stab/libvidstab.h>
#include <framework/mlt.h>
}

#include <string.h>
#include <assert.h>
#include "common.h"

static mlt_frame process_filter(mlt_filter filter, mlt_frame frame)
{
	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
	mlt_get_image vidstab_get_image = (mlt_get_image) mlt_properties_get_data( properties, "_vidstab_get_image", NULL );

#if 1
	mlt_position pos = mlt_filter_get_position(filter, frame);
	mlt_position length = mlt_filter_get_length2(filter, frame) - 1;
	if(pos >= length)
	{
		mlt_properties_set_data(properties, "_vidstab_get_image", NULL, 0, NULL, NULL);
	}
#endif

	if(vidstab_get_image == NULL)
	{
		if(mlt_properties_get(properties, "vectors") == NULL)
		{
			// vectors are NULL, so use a detect filter
			vidstab_get_image = get_image_and_detect;
		} else {
			// found vectors, so use a transform filter
			vidstab_get_image = get_image_and_transform;
		}

		mlt_properties_set_data( properties, "_vidstab_get_image", (void*)vidstab_get_image, 0, NULL, NULL );
	}

	mlt_frame_push_service(frame, filter);
	mlt_frame_push_get_image(frame, vidstab_get_image);
	return frame;
}

extern "C"
{

mlt_filter filter_vidstab_init(mlt_profile profile, mlt_service_type type,
		const char *id, char *arg)
{
	mlt_filter filter = NULL;

	if ((filter = mlt_filter_new()))
	{
		filter->process = process_filter;

		mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

		//properties for stabilize
		mlt_properties_set(properties, "shakiness", "4");
		mlt_properties_set(properties, "accuracy", "4");
		mlt_properties_set(properties, "stepsize", "6");
		mlt_properties_set(properties, "algo", "1");
		mlt_properties_set(properties, "mincontrast", "0.3");
		mlt_properties_set(properties, "show", "0");
		mlt_properties_set(properties, "tripod", "0");

		// properties for transform
		mlt_properties_set(properties, "smoothing", "15");
		mlt_properties_set(properties, "maxshift", "-1");
		mlt_properties_set(properties, "maxangle", "-1");
		mlt_properties_set(properties, "crop", "0");
		mlt_properties_set(properties, "invert", "0");
		mlt_properties_set(properties, "relative", "1");
		mlt_properties_set(properties, "zoom", "0");
		mlt_properties_set(properties, "optzoom", "1");
		mlt_properties_set(properties, "zoomspeed", "0.25");

		mlt_properties_set(properties, "vid.stab.version", LIBVIDSTAB_VERSION);

		return filter;
	}

	return NULL;
}

}
