/*
 * filter_detect.cpp
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
#include <framework/mlt_animation.h>
}

#include <string.h>
#include <assert.h>
#include "common.h"

#define FILTER_NAME "vid.stab.detect"

typedef struct _stab_data
{
	bool initialized;
	VSMotionDetect md;
	mlt_animation animation;

	void *parent;
} StabData;

char* vectors_serializer(mlt_animation animation, int length)
{
	return mlt_animation_serialize(animation);
}

#include <sstream>
char* lm_serializer(LocalMotions *lms, int length)
{
	std::ostringstream oss;
	int size = vs_vector_size(lms);
	for (int i = 0; i < size; ++i)
	{
		LocalMotion* lm = (LocalMotion*) vs_vector_get(lms, i);
		oss << lm->v.x << ' ';
		oss << lm->v.y << ' ';
		oss << lm->f.x << ' ';
		oss << lm->f.y << ' ';
		oss << lm->f.size << ' ';
		oss << lm->contrast << ' ';
		oss << lm->match << ' ';
	}
	return strdup(oss.str().c_str());
}

void lm_destructor(void *lms)
{
	vs_vector_del(static_cast<VSVector*>(lms));
}

static void serialize_localmotions(StabData* data, LocalMotions &vectors, mlt_position pos)
{
	mlt_animation_item_s item;

	// Initialize animation item
	item.is_key = 1;
	item.frame = data->md.frameNum;
	item.keyframe_type = mlt_keyframe_discrete;
	item.property = mlt_property_init();

	mlt_property_set_data(item.property, &vectors, 1, lm_destructor, (mlt_serialiser) lm_serializer);
	mlt_animation_insert(data->animation, &item);
	mlt_property_close(item.property);
}

int init_detect(StabData *data, mlt_properties properties, mlt_image_format *format, int *width, int *height)
{
	VSPixelFormat pf = convertImageFormat(*format);
	VSFrameInfo fi;
	vsFrameInfoInit(&fi, *width, *height, pf);

	VSMotionDetectConfig conf = vsMotionDetectGetDefaultConfig(FILTER_NAME);
	conf.shakiness = mlt_properties_get_int(properties, "shakiness");
	conf.accuracy = mlt_properties_get_int(properties, "accuracy");
	conf.stepSize = mlt_properties_get_int(properties, "stepsize");
	conf.algo = mlt_properties_get_int(properties, "algo");
	conf.contrastThreshold = mlt_properties_get_double(properties, "mincontrast");
	conf.show = mlt_properties_get_int(properties, "show");
	conf.virtualTripod = mlt_properties_get_int(properties, "tripod");

	vsMotionDetectInit(&data->md, &conf, &fi);

	// change name of the filter, so the vid.stab.transform will be used in 2nd-pass.
	mlt_properties_set(properties, "mlt_service", "vid.stab.transform");
	return 0;
}

static int get_image(mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable)
{
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

	*format = mlt_image_yuv420p;
	StabData *data = static_cast<StabData*>(filter->child);

	int error = mlt_frame_get_image(frame, image, format, width, height, writable);
	if (!error)
	{
		// Service locks are for concurrency control
		mlt_service_lock(MLT_FILTER_SERVICE(filter));

		if (!data->initialized)
		{
			init_detect(data, properties, format, width, height);
			data->initialized = true;
		}

		VSMotionDetect* md = &data->md;
		LocalMotions localmotions;
		VSFrame vsFrame;
		vsFrameFillFromBuffer(&vsFrame, *image, &md->fi);

		// detect and save motions
		vsMotionDetection(md, &localmotions, &vsFrame);
		mlt_position pos = mlt_filter_get_position( filter, frame );
		serialize_localmotions(data, localmotions, pos);

		mlt_service_unlock(MLT_FILTER_SERVICE(filter));
	}

	return error;
}

static mlt_frame process_filter(mlt_filter filter, mlt_frame frame)
{
	mlt_frame_push_service(frame, filter);
	mlt_frame_push_get_image(frame, get_image);
	return frame;
}

static void close_filter(mlt_filter filter)
{
	StabData *data = static_cast<StabData*>(filter->child);
	if (data)
	{
		if (data->initialized)
		{
			vsMotionDetectionCleanup(&data->md);
		}
		delete data;
		filter->child = NULL;
	}
}

extern "C"
{

mlt_filter filter_detect_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
	mlt_filter filter = NULL;

	StabData *data = new StabData;
	memset(data, 0, sizeof(StabData));

	if ((filter = mlt_filter_new()))
	{
		filter->process = process_filter;
		filter->close = close_filter;
		filter->child = data;

		data->animation = mlt_animation_new();
		data->parent = filter;

		mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
		mlt_properties_set(properties, "mlt_service", "vid.stab.transform");

		//properties for stabilize
		mlt_properties_set(properties, "shakiness", "4");
		mlt_properties_set(properties, "accuracy", "4");
		mlt_properties_set(properties, "stepsize", "6");
		mlt_properties_set(properties, "algo", "1");
		mlt_properties_set(properties, "mincontrast", "0.3");
		mlt_properties_set(properties, "show", "0");
		mlt_properties_set(properties, "tripod", "0");

		// properties for transform
		mlt_properties_set(properties, "smoothing", "10");
		mlt_properties_set(properties, "maxshift", "-1");
		mlt_properties_set(properties, "maxangle", "-1");
		mlt_properties_set(properties, "crop", "0");
		mlt_properties_set(properties, "invert", "0");
		mlt_properties_set(properties, "relative", "1");
		mlt_properties_set(properties, "zoom", "0");
		mlt_properties_set(properties, "optzoom", "1");
		mlt_properties_set(properties, "sharpen", "0.8");

		mlt_properties_set(properties, "vid.stab.version", LIBVIDSTAB_VERSION);
		mlt_properties_set_data(properties, "vectors", data->animation, 1, (mlt_destructor) mlt_animation_close,
				(mlt_serialiser) vectors_serializer);

		return filter;
	}

	delete data;
	return NULL;
}

}
