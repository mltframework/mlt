/*
 * filter_transform.cpp
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
#include <sstream>
#include "common.h"

#define FILTER_NAME "vid.stab.transform"

typedef struct
{
	bool initialized;
	VSTransformData td;
	VSTransformations trans;

	void *parent;
} TransformData;

int lm_deserialize(LocalMotions *lms, mlt_property property)
{
	std::istringstream iss(mlt_property_get_string(property));
	vs_vector_init(lms, 0);

	while (iss.good())
	{
		LocalMotion lm;
		iss >> lm.v.x >> lm.v.y >> lm.f.x >> lm.f.y >> lm.f.size >> lm.contrast >> lm.match;
		if (iss.fail())
		{
			break;
		}
		vs_vector_append_dup(lms, &lm, sizeof(lm));
	}
	return 0;
}

int vectors_deserialize(mlt_animation anim, VSManyLocalMotions *mlms)
{
	int error = 0;
	mlt_animation_item_s item;
	item.property = mlt_property_init();

	vs_vector_init(mlms, 1024); // initial number of frames, but it will be increased

	int length = mlt_animation_get_length(anim);
	for (int i = 0; i < length; ++i)
	{
		LocalMotions lms;

		// read lms
		mlt_animation_get_item(anim, &item, i + 1);
		if ((error = lm_deserialize(&lms, item.property)))
		{
			break;
		}

		vs_vector_set_dup(mlms, i, &lms, sizeof(LocalMotions));
	}

	mlt_property_close(item.property);
	return error;
}

inline int initialize_transforms(TransformData *data, int *width, int *height, mlt_image_format *format,
		mlt_properties properties, char* interps)
{
	VSPixelFormat pf = convertImageFormat(*format);
	VSFrameInfo fi_src, fi_dst;
	vsFrameInfoInit(&fi_src, *width, *height, pf);
	vsFrameInfoInit(&fi_dst, *width, *height, pf);

	VSTransformConfig conf = vsTransformGetDefaultConfig(FILTER_NAME);
	conf.smoothing = mlt_properties_get_int(properties, "smoothing");
	conf.maxShift = mlt_properties_get_int(properties, "maxshift");
	conf.maxAngle = mlt_properties_get_double(properties, "maxangle");
	conf.crop = (VSBorderType) mlt_properties_get_int(properties, "crop");
	conf.zoom = mlt_properties_get_int(properties, "zoom");
	conf.optZoom = mlt_properties_get_int(properties, "optzoom");
	conf.relative = mlt_properties_get_int(properties, "relative");
	conf.invert = mlt_properties_get_int(properties, "invert");
	if (mlt_properties_get_int(properties, "tripod") != 0)
	{
		// Virtual tripod mode: relative=False, smoothing=0
		conf.relative = 0;
		conf.smoothing = 0;
	}

	// by default a bilinear interpolation is selected
	conf.interpolType = VS_BiLinear;
	if (strcmp(interps, "nearest") == 0 || strcmp(interps, "neighbor") == 0)
		conf.interpolType = VS_Zero;
	else if (strcmp(interps, "tiles") == 0 || strcmp(interps, "fast_bilinear") == 0)
		conf.interpolType = VS_Linear;

	vsTransformDataInit(&data->td, &conf, &fi_src, &fi_dst);

	vsTransformationsInit(&data->trans);

	// load transformations
	mlt_animation animation = mlt_animation_new();
	char* strAnim = mlt_properties_get(properties, "vectors");
	if (mlt_animation_parse(animation, strAnim, 0, 0, NULL))
	{
		mlt_log_warning(NULL, "parse failed\n");
		return 1;
	}

	VSManyLocalMotions mlms;
	if (vectors_deserialize(animation, &mlms))
	{
		return 1;
	}

	mlt_animation_close(animation);

	vsLocalmotions2Transforms(&data->td, &mlms, &data->trans);
	vsPreprocessTransforms(&data->td, &data->trans);
	return 0;
}

void clear_transforms(TransformData *data)
{
	if (data->initialized)
	{
		vsTransformDataCleanup(&data->td);
		vsTransformationsCleanup(&data->trans);
	}
}

static int get_image(mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable)
{
	int error = 0;
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

	*format = mlt_image_yuv420p;
	TransformData *data = static_cast<TransformData*>(filter->child);

	error = mlt_frame_get_image(frame, image, format, width, height, 1);
	if (!error)
	{
		// Service locks are for concurrency control
		mlt_service_lock(MLT_FILTER_SERVICE(filter));

		// Handle signal from app to re-init data
		if (mlt_properties_get_int(properties, "refresh"))
		{
			mlt_properties_set(properties, "refresh", NULL);
			clear_transforms(data);
			data->initialized = false;
		}

		if (!data->initialized)
		{
			char *interps = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "rescale.interp");
			initialize_transforms(data, width, height, format, properties, interps);
			data->initialized = true;
		}

		VSTransformData* td = &data->td;
		VSFrame vsFrame;
		vsFrameFillFromBuffer(&vsFrame, *image, vsTransformGetSrcFrameInfo(td));

		// transform frame
		data->trans.current = mlt_filter_get_position(filter, frame);
		vsTransformPrepare(td, &vsFrame, &vsFrame);
		VSTransform t = vsGetNextTransform(td, &data->trans);
		vsDoTransform(td, t);
		vsTransformFinish(td);

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
	TransformData *data = static_cast<TransformData*>(filter->child);
	if (data)
	{
		clear_transforms(data);
		delete data;
		filter->child = NULL;
	}
}

extern "C"
{

mlt_filter filter_transform_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
	mlt_filter filter = NULL;

	TransformData *data = new TransformData;
	memset(data, 0, sizeof(TransformData));

	if ((filter = mlt_filter_new()))
	{
		filter->process = process_filter;
		filter->close = close_filter;
		filter->child = data;

		data->parent = filter;

		mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

		// properties for transform
		mlt_properties_set(properties, "smoothing", "10");
		mlt_properties_set(properties, "maxshift", "-1");
		mlt_properties_set(properties, "maxangle", "-1");
		mlt_properties_set(properties, "crop", "0");
		mlt_properties_set(properties, "invert", "0");
		mlt_properties_set(properties, "relative", "1");
		mlt_properties_set(properties, "zoom", "0");
		mlt_properties_set(properties, "optzoom", "1");

		return filter;
	}

	delete data;
	return NULL;
}

}
