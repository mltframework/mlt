/*
 * filter_mask_apply.c -- composite atop a cloned frame made by mask_start
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
#include <framework/mlt_transition.h>
#include <framework/mlt_log.h>

#include <string.h>

static int dummy_get_image(mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable)
{
	mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
	*image = mlt_properties_get_data(properties, "image", NULL);
	*format = mlt_properties_get_int(properties, "format");
	*width = mlt_properties_get_int(properties, "width");
	*height = mlt_properties_get_int(properties, "height");
	return 0;
}

static int get_image(mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable)
{
	mlt_transition transition = mlt_frame_pop_service(frame);
	*format = mlt_frame_pop_service_int(frame);
	int error = mlt_frame_get_image(frame, image, format, width, height, writable);
	if (!error) {
		mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
		mlt_frame clone = mlt_properties_get_data(properties, "mask frame", NULL);
		if (clone) {
			mlt_frame_push_get_image(frame, dummy_get_image);
			mlt_service_lock(MLT_TRANSITION_SERVICE(transition));
			mlt_transition_process(transition, clone, frame);
			mlt_service_unlock(MLT_TRANSITION_SERVICE(transition));
			error = mlt_frame_get_image(clone, image, format, width, height, writable);
			if (!error) {
				int size = mlt_image_format_size(*format, *width, *height, NULL);
				mlt_frame_set_image(frame, *image, size, NULL);
			}
		}
	}
	return error;
}

static mlt_frame process(mlt_filter filter, mlt_frame frame)
{
	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
	mlt_transition transition = mlt_properties_get_data(properties, "instance", NULL);
	char *name = mlt_properties_get(MLT_FILTER_PROPERTIES(filter), "transition");

	if (!name || !strcmp("", name))
		return frame;

	// Create the transition if needed.
	if (!transition
		|| !mlt_properties_get(MLT_FILTER_PROPERTIES(transition), "mlt_service") 
		|| strcmp(name, mlt_properties_get(MLT_FILTER_PROPERTIES(transition), "mlt_service"))) {
		mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));

		transition = mlt_factory_transition(profile, name, NULL);
		mlt_properties_set_data(MLT_FILTER_PROPERTIES(filter), "instance", transition, 0, (mlt_destructor) mlt_transition_close, NULL);
	}

	if (transition) {
		mlt_properties transition_props = MLT_TRANSITION_PROPERTIES(transition);
		int type = mlt_properties_get_int(transition_props, "_transition_type");
		int hide = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "hide");

		mlt_properties_pass_list(transition_props, properties, "in out");
		mlt_properties_pass(transition_props, properties, "transition." );

		// Only if video transition on visible track.
		if ((type & 1) && !mlt_frame_is_test_card(frame) && !(hide & 1)) {
			mlt_frame_push_service_int(frame,
				mlt_image_format_id(mlt_properties_get(properties, "mlt_image_format")));
			mlt_frame_push_service(frame, transition);
			mlt_frame_push_get_image(frame, get_image);
		}
		if (type == 0)
			mlt_properties_debug(transition_props, "unknown transition type", stderr);
	} else {
		mlt_properties_debug(properties, "mask_failed to create transition", stderr );
	}

	return frame;
}

mlt_filter filter_mask_apply_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
	mlt_filter filter = mlt_filter_new( );
	if (filter) {
		mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "transition", arg? arg : "frei0r.composition");
		mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "mlt_image_format", "rgb24a");
		filter->process = process;
	}
	return filter;
}
