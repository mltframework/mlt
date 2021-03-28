/*
 * filter_choppy.c -- simple frame repeating filter
 * Copyright (C) 2020 Meltytech, LLC
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
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <stdlib.h>
#include <string.h>

static int get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error;
	mlt_filter filter = mlt_frame_pop_service(frame);
	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	int amount = mlt_properties_anim_get_int(properties, "amount", position, length) + 1;

	if (amount > 1) {
		mlt_service_lock(MLT_FILTER_SERVICE(filter));
		mlt_frame cloned_frame = mlt_properties_get_data( properties, "cloned_frame", NULL);
		mlt_position cloned_pos = mlt_frame_get_position(cloned_frame);

		position = mlt_frame_get_position(frame);
		if (!cloned_frame || MLT_POSITION_MOD(position, amount) == 0 || abs(position - cloned_pos) > amount) {
			error = mlt_frame_get_image(frame, image, format, width, height, writable);
			cloned_frame = mlt_frame_clone(frame,  0);
			mlt_properties_set_data(properties, "cloned_frame", cloned_frame, 0, (mlt_destructor) mlt_frame_close, NULL);
			mlt_service_unlock(MLT_FILTER_SERVICE(filter));
		} else {
			mlt_service_unlock(MLT_FILTER_SERVICE(filter));
			error = mlt_frame_get_image(frame, image, format, width, height, writable);
			if (!error && cloned_frame && cloned_frame->image.data) {
				mlt_image_copy_deep( &cloned_frame->image, &frame->image );
			}
		}
	} else {
		error = mlt_frame_get_image(frame, image, format, width, height, writable);
	}
	return error;
}

/** Filter processing.
*/

static mlt_frame process(mlt_filter filter, mlt_frame frame)
{
	mlt_frame_push_service(frame, filter);
	mlt_frame_push_get_image(frame, get_image);
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_choppy_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
	mlt_filter filter = mlt_filter_new();
	if (filter) {
		filter->process = process;
		mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "amount", arg? arg : "0");
	}
	return filter;
}
