/*
 * filter_vidstab.cpp
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

#include <sstream>
#include <string.h>
#include <assert.h>
#include "common.h"


typedef struct _stab_data
{
	VSMotionDetect md;
	mlt_animation animation;
} StabData;

typedef struct
{
	VSTransformData td;
	VSTransformations trans;
} TransformData;


char* vectors_serializer(mlt_animation animation, int length)
{
	return mlt_animation_serialize(animation);
}

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

void destroy_transforms(TransformData *data)
{
	if (data)
	{
		vsTransformDataCleanup(&data->td);
		vsTransformationsCleanup(&data->trans);
		delete data;
	}
}

TransformData* initialize_transforms(int *width, int *height, mlt_image_format *format,
		mlt_properties properties, const char* interps)
{
	TransformData *data = new TransformData;
	memset(data, 0, sizeof(TransformData));

	VSPixelFormat pf = convertImageFormat(*format);
	VSFrameInfo fi_src, fi_dst;
	vsFrameInfoInit(&fi_src, *width, *height, pf);
	vsFrameInfoInit(&fi_dst, *width, *height, pf);

	const char* filterName = mlt_properties_get(properties, "mlt_service");

	VSTransformConfig conf = vsTransformGetDefaultConfig(filterName);
	conf.smoothing = mlt_properties_get_int(properties, "smoothing");
	conf.maxShift = mlt_properties_get_int(properties, "maxshift");
	conf.maxAngle = mlt_properties_get_double(properties, "maxangle");
	conf.crop = (VSBorderType) mlt_properties_get_int(properties, "crop");
	conf.zoom = mlt_properties_get_int(properties, "zoom");
	conf.optZoom = mlt_properties_get_int(properties, "optzoom");
	conf.zoomSpeed = mlt_properties_get_double(properties, "zoomspeed");
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
		mlt_animation_close(animation);
		destroy_transforms(data);
		return NULL;
	}

	VSManyLocalMotions mlms;
	if (vectors_deserialize(animation, &mlms))
	{
		mlt_animation_close(animation);
		destroy_transforms(data);
		return NULL;
	}

	mlt_animation_close(animation);

	vsLocalmotions2Transforms(&data->td, &mlms, &data->trans);
	vsPreprocessTransforms(&data->td, &data->trans);
	return data;
}

int get_image_and_transform(mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable)
{
	int error = 0;
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

	*format = mlt_image_yuv420p;

	error = mlt_frame_get_image(frame, image, format, width, height, 1);
	if (!error)
	{
		// Service locks are for concurrency control
		mlt_service_lock(MLT_FILTER_SERVICE(filter));

		TransformData *data = static_cast<TransformData*>(mlt_properties_get_data(properties, "_transform_data", NULL));

		// Handle signal from app to re-init data
		if (mlt_properties_get_int(properties, "refresh"))
		{
			mlt_properties_set(properties, "refresh", NULL);
			destroy_transforms(data);
			data = NULL;
		}

		if (!data)
		{
			const char *interps = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "rescale.interp");
			data = initialize_transforms(width, height, format, properties, interps);
			if(!data) {
				mlt_service_unlock(MLT_FILTER_SERVICE(filter));
				return 1; // return error code
			}
			mlt_properties_set_data(properties, "_transform_data", data, 0, (mlt_destructor) destroy_transforms, NULL);
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

static StabData* init_detect(mlt_properties properties, mlt_image_format *format, int *width, int *height)
{
	StabData *data = new StabData;
	memset(data, 0, sizeof(StabData));
	data->animation = mlt_animation_new();

	VSPixelFormat pf = convertImageFormat(*format);
	VSFrameInfo fi;
	vsFrameInfoInit(&fi, *width, *height, pf);

	const char* filterName = mlt_properties_get(properties, "mlt_service");

	VSMotionDetectConfig conf = vsMotionDetectGetDefaultConfig(filterName);
	conf.shakiness = mlt_properties_get_int(properties, "shakiness");
	conf.accuracy = mlt_properties_get_int(properties, "accuracy");
	conf.stepSize = mlt_properties_get_int(properties, "stepsize");
	conf.algo = mlt_properties_get_int(properties, "algo");
	conf.contrastThreshold = mlt_properties_get_double(properties, "mincontrast");
	conf.show = mlt_properties_get_int(properties, "show");
	conf.virtualTripod = mlt_properties_get_int(properties, "tripod");
	vsMotionDetectInit(&data->md, &conf, &fi);

	// add vectors to properties
	mlt_properties_set_data(properties, "vectors", data->animation, 1, (mlt_destructor) mlt_animation_close,
					(mlt_serialiser) vectors_serializer);
	return data;
}

void destroy_detect(StabData *data)
{
	if (data)
	{
		vsMotionDetectionCleanup(&data->md);
		delete data;
	}
}

int get_image_and_detect(mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable)
{
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

	*format = mlt_image_yuv420p;

	writable = writable || mlt_properties_get_int(properties, "show") ? 1 : 0;

	int error = mlt_frame_get_image(frame, image, format, width, height, writable);
	if (!error)
	{
		// Service locks are for concurrency control
		mlt_service_lock(MLT_FILTER_SERVICE(filter));

		StabData *data = static_cast<StabData*>(mlt_properties_get_data(properties, "_stab_data", NULL));
		if (!data)
		{
			data = init_detect(properties, format, width, height);
			mlt_properties_set_data(properties, "_stab_data", data, 0, (mlt_destructor) destroy_detect, NULL);
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
	mlt_filter filter = mlt_filter_new();

	if( filter )
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
	}

	return filter;
}

}
