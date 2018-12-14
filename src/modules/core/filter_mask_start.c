/*
 * filter_mask_start.c -- clone a frame before invoking a filter
 * Copyright (C) 2018 Meltytech, LLC
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
#include <framework/mlt_log.h>

#include <string.h>

static int get_image(mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable)
{
	mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
	int error = mlt_frame_get_image(frame, image, format, width, height, writable);
	if (!error) {
		mlt_frame clone = mlt_frame_clone(frame, 1);
		clone->convert_audio = frame->convert_audio;
		clone->convert_image = frame->convert_image;
		mlt_properties_set_data(properties,
			"mask frame", clone, 0, (mlt_destructor) mlt_frame_close, NULL);
	}
	return error;
}

static mlt_frame process(mlt_filter filter, mlt_frame frame)
{
	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
	mlt_filter instance = mlt_properties_get_data(properties, "instance", NULL);
	const char* name = mlt_properties_get(properties, "filter");

	if (!name || !strcmp("", name))
		return frame;

	// Create the filter if needed.
	if (!instance
		|| !mlt_properties_get(MLT_FILTER_PROPERTIES(instance), "mlt_service") 
		|| strcmp(name, mlt_properties_get(MLT_FILTER_PROPERTIES(instance), "mlt_service"))) {
		mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
		instance = mlt_factory_filter(profile, name, NULL);
		mlt_properties_set_data(properties, "instance", instance, 0, (mlt_destructor) mlt_filter_close, NULL);
	}

	if (instance) {
		mlt_properties instance_props = MLT_FILTER_PROPERTIES(instance);

		mlt_properties_pass_list(instance_props, properties, "in out");
		mlt_properties_pass(instance_props, properties, "filter.");
		mlt_frame_push_get_image(frame, get_image);
		return mlt_filter_process(instance, frame);
	} else {
		mlt_properties_debug(properties, "failed to create filter", stderr);
		return frame;
	}
}

mlt_filter filter_mask_start_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
	mlt_filter filter = mlt_filter_new();
	if (filter)	{
		mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "filter", arg? arg : "frei0r.alphaspot");
		filter->process = process;
	}
	return filter;
}
