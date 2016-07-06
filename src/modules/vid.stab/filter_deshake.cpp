/*
 * filter_deshake.cpp
 * Copyright (C) 2013 Marco Gittler <g.marco@freenet.de>
 * Copyright (C) 2013 Jakub Ksiezniak <j.ksiezniak@gmail.com>
 * Copyright (C) 2014 Brian Matherly <pez4brian@yahoo.com>
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

extern "C"
{
#include <vid.stab/libvidstab.h>
#include "common.h"
}

#include <framework/mlt.h>
#include <string.h>
#include <assert.h>

typedef struct _deshake_data
{
	bool initialized;
	VSMotionDetect md;
	VSTransformData td;
	VSSlidingAvgTrans avg;
	VSMotionDetectConfig mconf;
	VSTransformConfig tconf;
	mlt_position lastFrame;
} DeshakeData;

static void get_config( VSTransformConfig* tconf, VSMotionDetectConfig* mconf, mlt_filter filter, mlt_frame frame )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	const char* filterName = mlt_properties_get( properties, "mlt_service" );

	memset( mconf, 0, sizeof(VSMotionDetectConfig) );
	*mconf = vsMotionDetectGetDefaultConfig( filterName );
	mconf->shakiness = mlt_properties_get_int( properties, "shakiness" );
	mconf->accuracy = mlt_properties_get_int(properties, "accuracy");
	mconf->stepSize = mlt_properties_get_int(properties, "stepsize");
	mconf->contrastThreshold = mlt_properties_get_double(properties, "mincontrast");

	memset( tconf, 0, sizeof(VSTransformConfig) );
	*tconf = vsTransformGetDefaultConfig( filterName );
	tconf->smoothing = mlt_properties_get_int(properties, "smoothing");
	tconf->maxShift = mlt_properties_get_int(properties, "maxshift");
	tconf->maxAngle = mlt_properties_get_double(properties, "maxangle");
	tconf->crop = (VSBorderType) mlt_properties_get_int(properties, "crop");
	tconf->zoom = mlt_properties_get_int(properties, "zoom");
	tconf->optZoom = mlt_properties_get_int(properties, "optzoom");
	tconf->zoomSpeed = mlt_properties_get_double(properties, "zoomspeed");
	tconf->relative = 1;

	// by default a bicubic interpolation is selected
	const char *interps = mlt_properties_get( MLT_FRAME_PROPERTIES( frame ), "rescale.interp" );
	tconf->interpolType = VS_BiCubic;
	if ( strcmp( interps, "nearest" ) == 0 || strcmp( interps, "neighbor" ) == 0 )
		tconf->interpolType = VS_Zero;
	else if ( strcmp( interps, "tiles" ) == 0 || strcmp( interps, "fast_bilinear" ) == 0 )
		tconf->interpolType = VS_Linear;
	else if ( strcmp( interps, "bilinear" ) == 0 )
		tconf->interpolType = VS_BiLinear;
}

static int check_config( mlt_filter filter, mlt_frame frame )
{
	DeshakeData *data = static_cast<DeshakeData*>( filter->child );
	VSTransformConfig new_tconf;
	VSMotionDetectConfig new_mconf;

	get_config( &new_tconf, &new_mconf, filter, frame );

	if( compare_transform_config( &data->tconf, &new_tconf ) ||
		compare_motion_config( &data->mconf, &new_mconf ) )
	{
		return 1;
	}

	return 0;
}

static void init_deshake( DeshakeData *data, mlt_filter filter, mlt_frame frame,
		VSPixelFormat vs_format, int *width, int *height )
{
	VSFrameInfo fiIn, fiOut;

	vsFrameInfoInit( &fiIn, *width, *height, vs_format );
	vsFrameInfoInit( &fiOut, *width, *height, vs_format );
	get_config( &data->tconf, &data->mconf, filter, frame );
	vsMotionDetectInit( &data->md, &data->mconf, &fiIn );
	vsTransformDataInit(&data->td, &data->tconf, &fiIn, &fiOut);

	data->avg.initialized = 0;
}

static void clear_deshake(DeshakeData *data)
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
	uint8_t* vs_image = NULL;
	VSPixelFormat vs_format = PF_NONE;

	// VS only works on progressive frames
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "consumer_deinterlace", 1 );

	*format = validate_format( *format );
	DeshakeData *data = static_cast<DeshakeData*>(filter->child);

	int error = mlt_frame_get_image(frame, image, format, width, height, 1);

	// Convert the received image to a format vid.stab can handle
	if ( !error )
	{
		vs_format = mltimage_to_vsimage( *format, *width, *height, *image, &vs_image );
	}

	if ( vs_image )
	{
		// Service locks are for concurrency control
		mlt_service_lock(MLT_FILTER_SERVICE(filter));

		// clear deshake data, when seeking or dropping frames
		mlt_position pos = mlt_filter_get_position( filter, frame );
		if( pos != data->lastFrame + 1 ||
			check_config( filter, frame) == 1 )
		{
			clear_deshake( data );
			data->initialized = false;
		}
		data->lastFrame = pos;

		if ( !data->initialized )
		{
			init_deshake( data, filter, frame, vs_format, width, height );
			data->initialized = true;
		}

		VSMotionDetect* md = &data->md;
		VSTransformData* td = &data->td;
		LocalMotions localmotions;
		VSTransform motion;
		VSFrame vsFrame;

		vsFrameFillFromBuffer(&vsFrame, vs_image, &md->fi);
		vsMotionDetection(md, &localmotions, &vsFrame);

		const char* filterName = mlt_properties_get( MLT_FILTER_PROPERTIES( filter ), "mlt_service" );
		motion = vsSimpleMotionsToTransform(md->fi, filterName, &localmotions);
		vs_vector_del(&localmotions);

		vsTransformPrepare(td, &vsFrame, &vsFrame);

		VSTransform t = vsLowPassTransforms(td, &data->avg, &motion);
//	    mlt_log_warning(filter, "Trans: det: %f %f %f \n\t\t act: %f %f %f %f",
//	                 motion.x, motion.y, motion.alpha,
//	                 t.x, t.y, t.alpha, t.zoom);
		vsDoTransform(td, t);
		vsTransformFinish(td);

		vsimage_to_mltimage( vs_image, *image, *format, *width, *height );

		mlt_service_unlock(MLT_FILTER_SERVICE(filter));

		free_vsimage( vs_image, vs_format );
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

mlt_filter filter_deshake_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = NULL;

	DeshakeData *data = new DeshakeData;
	memset(data, 0, sizeof(DeshakeData));

	if ((filter = mlt_filter_new()))
	{
		filter->process = process_filter;
		filter->close = close_filter;
		filter->child = data;

		mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
		//properties for stabilize
		mlt_properties_set(properties, "shakiness", "4");
		mlt_properties_set(properties, "accuracy", "4");
		mlt_properties_set(properties, "stepsize", "6");
		mlt_properties_set(properties, "mincontrast", "0.3");

		//properties for transform
		mlt_properties_set(properties, "smoothing", "15");
		mlt_properties_set(properties, "maxshift", "-1");
		mlt_properties_set(properties, "maxangle", "-1");
		mlt_properties_set(properties, "crop", "0");
		mlt_properties_set(properties, "zoom", "0");
		mlt_properties_set(properties, "optzoom", "1");
		mlt_properties_set(properties, "zoomspeed", "0.25");

		init_vslog();

		return filter;
	}

	delete data;
	return NULL;
}

}
