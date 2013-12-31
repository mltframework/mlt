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
}

#include <framework/mlt.h>
#include <string.h>
#include <assert.h>
#include "common.h"

#define FILTER_NAME "vid.stab.deshake"

typedef struct _deshake_data
{
	bool initialized;
	VSMotionDetect md;
	VSTransformData td;
	VSSlidingAvgTrans avg;

	void *parent;
} DeshakeData;

int init_deshake(DeshakeData *data, mlt_properties properties,
		mlt_image_format *format, int *width, int *height, char* interps)
{
	VSPixelFormat pf = convertImageFormat(*format);
	VSFrameInfo fiIn, fiOut;
	vsFrameInfoInit(&fiIn, *width, *height, pf);
	vsFrameInfoInit(&fiOut, *width, *height, pf);

	VSMotionDetectConfig conf = vsMotionDetectGetDefaultConfig(FILTER_NAME);
	conf.shakiness = mlt_properties_get_int(properties, "shakiness");
	conf.accuracy = mlt_properties_get_int(properties, "accuracy");
	conf.stepSize = mlt_properties_get_int(properties, "stepsize");
	conf.algo = mlt_properties_get_int(properties, "algo");
	conf.contrastThreshold = mlt_properties_get_double(properties, "mincontrast");
	conf.show = 0;

	vsMotionDetectInit(&data->md, &conf, &fiIn);

	VSTransformConfig tdconf = vsTransformGetDefaultConfig(FILTER_NAME);
	tdconf.smoothing = mlt_properties_get_int(properties, "smoothing");
	tdconf.maxShift = mlt_properties_get_int(properties, "maxshift");
	tdconf.maxAngle = mlt_properties_get_double(properties, "maxangle");
	tdconf.crop = (VSBorderType) mlt_properties_get_int(properties, "crop");
	tdconf.zoom = mlt_properties_get_int(properties, "zoom");
	tdconf.optZoom = mlt_properties_get_int(properties, "optzoom");
	tdconf.relative = 1;
	tdconf.invert = 0;

	// by default a bilinear interpolation is selected
	tdconf.interpolType = VS_BiLinear;
	if (strcmp(interps, "nearest") == 0 || strcmp(interps, "neighbor") == 0)
		tdconf.interpolType = VS_Zero;
	else if (strcmp(interps, "tiles") == 0 || strcmp(interps, "fast_bilinear") == 0)
		tdconf.interpolType = VS_Linear;

	vsTransformDataInit(&data->td, &tdconf, &fiIn, &fiOut);

	data->avg.initialized = 0;
	return 0;
}

void clear_deshake(DeshakeData *data)
{
	if (data->initialized)
	{
		vsMotionDetectionCleanup(&data->md);
		vsTransformDataCleanup(&data->td);
	}
}

static int get_image(mlt_frame frame, uint8_t **image, mlt_image_format *format,
		int *width, int *height, int writable)
{
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

	*format = mlt_image_yuv420p;
	DeshakeData *data = static_cast<DeshakeData*>(filter->child);

	int error = mlt_frame_get_image(frame, image, format, width, height, 1);
	if (!error)
	{
		// Service locks are for concurrency control
		mlt_service_lock(MLT_FILTER_SERVICE(filter));

		// Handle signal from app to re-init data
		if (mlt_properties_get_int(properties, "refresh"))
		{
			mlt_properties_set(properties, "refresh", NULL);
			clear_deshake(data);
			data->initialized = false;
		}

		if (!data->initialized)
		{
			char *interps = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "rescale.interp");
			init_deshake(data, properties, format, width, height,
					interps);
			data->initialized = true;
		}

		VSMotionDetect* md = &data->md;
		VSTransformData* td = &data->td;
		LocalMotions localmotions;
		VSTransform motion;
		VSFrame vsFrame;

		vsFrameFillFromBuffer(&vsFrame, *image, &md->fi);
		vsMotionDetection(md, &localmotions, &vsFrame);

		motion = vsSimpleMotionsToTransform(td, &localmotions);
		vs_vector_del(&localmotions);

		vsTransformPrepare(td, &vsFrame, &vsFrame);

		VSTransform t = vsLowPassTransforms(td, &data->avg, &motion);
//	    mlt_log_warning(filter, "Trans: det: %f %f %f \n\t\t act: %f %f %f %f",
//	                 motion.x, motion.y, motion.alpha,
//	                 t.x, t.y, t.alpha, t.zoom);
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
	DeshakeData *data = static_cast<DeshakeData*>(filter->child);
	if (data)
	{
		clear_deshake(data);
		delete data;
		filter->child = NULL;
	}
}

extern "C"
{

mlt_filter filter_deshake_init(mlt_profile profile, mlt_service_type type,
		const char *id, char *arg)
{
	mlt_filter filter = NULL;

	DeshakeData *data = new DeshakeData;
	memset(data, 0, sizeof(DeshakeData));

	if ((filter = mlt_filter_new()))
	{
		filter->process = process_filter;
		filter->close = close_filter;
		filter->child = data;
		data->parent = filter;

		mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
		//properties for stabilize
		mlt_properties_set(properties, "shakiness", "4");
		mlt_properties_set(properties, "accuracy", "4");
		mlt_properties_set(properties, "stepsize", "6");
		mlt_properties_set(properties, "algo", "1");
		mlt_properties_set(properties, "mincontrast", "0.3");

		//properties for transform
		mlt_properties_set(properties, "smoothing", "10");
		mlt_properties_set(properties, "maxshift", "-1");
		mlt_properties_set(properties, "maxangle", "-1");
		mlt_properties_set(properties, "crop", "0");
		mlt_properties_set(properties, "zoom", "0");
		mlt_properties_set(properties, "optzoom", "1");
		mlt_properties_set(properties, "sharpen", "0.8");

		return filter;
	}

	delete data;
	return NULL;
}

}
