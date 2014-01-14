/*
 * filter_vidstab.cpp
 * Copyright (C) 2013 Marco Gittler <g.marco@freenet.de>
 * Copyright (C) 2013 Jakub Ksiezniak <j.ksiezniak@gmail.com>
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

typedef struct
{
	VSMotionDetect md;
	VSManyLocalMotions mlms;
} vs_analyze;

typedef struct
{
	VSTransformData td;
	VSTransformations trans;
} vs_apply;

typedef struct
{
	vs_analyze* analyze_data;
	vs_apply* apply_data;
} vs_data;

/** Free all data used by a VSManyLocalMotions instance.
 */

static void free_manylocalmotions( VSManyLocalMotions* mlms )
{
	for( int i = 0; i < vs_vector_size( mlms ); i++ )
	{
		LocalMotions* lms = (LocalMotions*)vs_vector_get( mlms, i );
		vs_vector_del( lms );
	}
	vs_vector_del( mlms );
}

/** Serialize a VSManyLocalMotions instance and store it in the properties.
 *
 * Each LocalMotions instance is converted to a string and stored in an animation.
 * Then, the entire animation is serialized and stored in the properties.
 * \param properties the filter properties
 * \param mlms an initialized VSManyLocalMotions instance
 */

static void publish_manylocalmotions( mlt_properties properties, VSManyLocalMotions* mlms )
{
	mlt_animation animation = mlt_animation_new();
	mlt_animation_item_s item;

	// Initialize animation item.
	item.is_key = 1;
	item.keyframe_type = mlt_keyframe_discrete;
	item.property = mlt_property_init();

	// Convert each LocalMotions instance to a string and add it to the animation.
	for( int i = 0; i < vs_vector_size( mlms ); i++ )
	{
		LocalMotions* lms = (LocalMotions*)vs_vector_get( mlms, i );
		item.frame = i;
		int size = vs_vector_size( lms );

		std::ostringstream oss;
		for ( int j = 0; j < size; ++j )
		{
			LocalMotion* lm = (LocalMotion*)vs_vector_get( lms, j );
			oss << lm->v.x << ' ';
			oss << lm->v.y << ' ';
			oss << lm->f.x << ' ';
			oss << lm->f.y << ' ';
			oss << lm->f.size << ' ';
			oss << lm->contrast << ' ';
			oss << lm->match << ' ';
		}
		mlt_property_set_string( item.property, oss.str().c_str() );
		mlt_animation_insert( animation, &item );
	}

	// Serialize and store the animation.
	char* motion_str = mlt_animation_serialize( animation );
	mlt_properties_set( properties, "results", motion_str );

	mlt_property_close( item.property );
	mlt_animation_close( animation );
	free( motion_str );
}

/** Get the motions data from the properties and convert it to a VSManyLocalMotions.
 *
 * Each LocalMotions instance is converted to a string and stored in an animation.
 * Then, the entire animation is serialized and stored in the properties.
 * \param properties the filter properties
 * \param mlms an initialized (but empty) VSManyLocalMotions instance
 */

static void read_manylocalmotions( mlt_properties properties, VSManyLocalMotions* mlms )
{
	mlt_animation_item_s item;
	item.property = mlt_property_init();
	mlt_animation animation = mlt_animation_new();
	// Get the results property which represents the VSManyLocalMotions
	char* motion_str = mlt_properties_get( properties, "results" );

	mlt_animation_parse( animation, motion_str, 0, 0, NULL );

	int length = mlt_animation_get_length( animation );

	for ( int i = 0; i <= length; ++i )
	{
		LocalMotions lms;
		vs_vector_init( &lms, 0 );

		// Get the animation item that represents the LocalMotions
		mlt_animation_get_item( animation, &item, i );

		// Convert the property to a real LocalMotions
		std::istringstream iss( mlt_property_get_string( item.property ) );
		while ( iss.good() )
		{
			LocalMotion lm;
			iss >> lm.v.x >> lm.v.y >> lm.f.x >> lm.f.y >> lm.f.size >> lm.contrast >> lm.match;
			if ( !iss.fail() )
			{
				vs_vector_append_dup( &lms, &lm, sizeof(lm) );
			}
		}

		// Add the LocalMotions to the ManyLocalMotions
		vs_vector_set_dup( mlms, i, &lms, sizeof(LocalMotions) );
	}

	mlt_property_close( item.property );
	mlt_animation_close( animation );
}

static vs_apply* init_apply_data( mlt_filter filter, mlt_frame frame, int width, int height, mlt_image_format format )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	vs_apply* apply_data = (vs_apply*)calloc( 1, sizeof(vs_apply) );
	memset( apply_data, 0, sizeof( vs_apply ) );
	VSPixelFormat pf = convertImageFormat( format );

	const char* filterName = mlt_properties_get( properties, "mlt_service" );
	VSTransformConfig conf = vsTransformGetDefaultConfig( filterName );
	conf.smoothing = mlt_properties_get_int( properties, "smoothing" );
	conf.maxShift = mlt_properties_get_int( properties, "maxshift" );
	conf.maxAngle = mlt_properties_get_double( properties, "maxangle" );
	conf.crop = (VSBorderType)mlt_properties_get_int( properties, "crop" );
	conf.zoom = mlt_properties_get_int( properties, "zoom" );
	conf.optZoom = mlt_properties_get_int( properties, "optzoom" );
	conf.zoomSpeed = mlt_properties_get_double( properties, "zoomspeed" );
	conf.relative = mlt_properties_get_int( properties, "relative" );
	conf.invert = mlt_properties_get_int( properties, "invert" );
	if ( mlt_properties_get_int( properties, "tripod" ) != 0 )
	{
		// Virtual tripod mode: relative=False, smoothing=0
		conf.relative = 0;
		conf.smoothing = 0;
	}

	// by default a bilinear interpolation is selected
	const char *interps = mlt_properties_get( MLT_FRAME_PROPERTIES( frame ), "rescale.interp" );
	conf.interpolType = VS_BiLinear;
	if (strcmp(interps, "nearest") == 0 || strcmp(interps, "neighbor") == 0)
		conf.interpolType = VS_Zero;
	else if (strcmp(interps, "tiles") == 0 || strcmp(interps, "fast_bilinear") == 0)
		conf.interpolType = VS_Linear;

	// load motions
	VSManyLocalMotions mlms;
	vs_vector_init( &mlms, mlt_filter_get_length2( filter, frame ) );
	read_manylocalmotions( properties, &mlms );

	// Convert motions to VSTransformations
	VSTransformData* td = &apply_data->td;
	VSTransformations* trans = &apply_data->trans;
	VSFrameInfo fi_src, fi_dst;
	vsFrameInfoInit( &fi_src, width, height, pf );
	vsFrameInfoInit( &fi_dst, width, height, pf );
	vsTransformDataInit( td, &conf, &fi_src, &fi_dst );
	vsTransformationsInit( trans );
	vsLocalmotions2Transforms( td, &mlms, trans );
	vsPreprocessTransforms( td, trans );

	free_manylocalmotions( &mlms );

	return apply_data;
}

static void destory_apply_data( vs_apply* apply_data )
{
	if ( apply_data )
	{
		vsTransformDataCleanup( &apply_data->td );
		vsTransformationsCleanup( &apply_data->trans );
		free( apply_data );
	}
}

static vs_analyze* init_analyze_data( mlt_filter filter, mlt_frame frame, mlt_image_format format, int width, int height )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	vs_analyze* analyze_data = (vs_analyze*)calloc( 1, sizeof(vs_analyze) );
	memset( analyze_data, 0, sizeof(vs_analyze) );

	// Initialize the VSManyLocalMotions vector where motion data will be
	// stored for each frame.
	vs_vector_init( &analyze_data->mlms, mlt_filter_get_length2( filter, frame ) );

	// Initialize a VSFrameInfo to be used below
	VSPixelFormat pf = convertImageFormat( format );
	VSFrameInfo fi;
	vsFrameInfoInit( &fi, width, height, pf );

	// Initialize a VSMotionDetect
	const char* filterName = mlt_properties_get( properties, "mlt_service" );
	VSMotionDetectConfig conf = vsMotionDetectGetDefaultConfig( filterName );
	conf.shakiness = mlt_properties_get_int( properties, "shakiness" );
	conf.accuracy = mlt_properties_get_int( properties, "accuracy" );
	conf.stepSize = mlt_properties_get_int( properties, "stepsize" );
	conf.algo = mlt_properties_get_int( properties, "algo" );
	conf.contrastThreshold = mlt_properties_get_double( properties, "mincontrast" );
	conf.show = mlt_properties_get_int( properties, "show" );
	conf.virtualTripod = mlt_properties_get_int( properties, "tripod" );
	vsMotionDetectInit( &analyze_data->md, &conf, &fi );

	return analyze_data;
}

void destory_analyze_data( vs_analyze* analyze_data )
{
	if ( analyze_data )
	{
		vsMotionDetectionCleanup( &analyze_data->md );
		free_manylocalmotions( &analyze_data->mlms );
		free( analyze_data );
	}
}

static int get_image_and_apply( mlt_filter filter, mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	vs_data* data = (vs_data*)filter->child;

	*format = mlt_image_yuv420p;

	error = mlt_frame_get_image( frame, image, format, width, height, 1 );
	if ( !error )
	{
		mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

		// Handle signal from app to re-init data
		if ( mlt_properties_get_int(properties, "refresh") )
		{
			mlt_properties_set(properties, "refresh", NULL);
			destory_apply_data( data->apply_data );
			data->apply_data = NULL;
		}

		// Init transform data if necessary (first time)
		if ( !data->apply_data )
		{
			data->apply_data = init_apply_data( filter, frame, *width, *height, *format );
		}

		// Apply transformations to this image
		VSTransformData* td = &data->apply_data->td;
		VSTransformations* trans = &data->apply_data->trans;
		VSFrame vsFrame;
		vsFrameFillFromBuffer( &vsFrame, *image, vsTransformGetSrcFrameInfo( td ) );
		trans->current = mlt_filter_get_position( filter, frame );
		vsTransformPrepare( td, &vsFrame, &vsFrame );
		VSTransform t = vsGetNextTransform( td, trans );
		vsDoTransform( td, t );
		vsTransformFinish( td );

		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
	}

	return error;
}


static int get_image_and_analyze( mlt_filter filter, mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	vs_data* data = (vs_data*)filter->child;
	mlt_position pos = mlt_filter_get_position( filter, frame );

	*format = mlt_image_yuv420p;

	writable = writable || mlt_properties_get_int( properties, "show" ) ? 1 : 0;

	int error = mlt_frame_get_image( frame, image, format, width, height, writable );
	if ( !error )
	{
		// Service locks are for concurrency control
		mlt_service_lock( MLT_FILTER_SERVICE(filter) );

		if ( !data->analyze_data )
		{
			data->analyze_data = init_analyze_data( filter, frame, *format, *width, *height );
		}

		// Initialize the VSFrame to be analyzed.
		VSMotionDetect* md = &data->analyze_data->md;
		LocalMotions localmotions;
		VSFrame vsFrame;
		vsFrameFillFromBuffer( &vsFrame, *image, &md->fi );

		// Detect and save motions.
		vsMotionDetection( md, &localmotions, &vsFrame );
		vs_vector_set_dup( &data->analyze_data->mlms, pos, &localmotions, sizeof(LocalMotions) );

		// Publish the motions if this is the last frame.
		if ( pos + 1 == mlt_filter_get_length2( filter, frame ) )
		{
			publish_manylocalmotions( properties, &data->analyze_data->mlms );
		}

		mlt_service_unlock( MLT_FILTER_SERVICE(filter) );
	}
	return error;
}

static int get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = (mlt_filter)mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );

	if( mlt_properties_get( properties, "results" ) )
	{
		return get_image_and_apply( filter, frame, image, format, width, height, writable );
	}
	else
	{
		return get_image_and_analyze( filter, frame, image, format, width, height, writable );
	}
}

static mlt_frame process_filter( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, get_image );
	return frame;
}

static void filter_close( mlt_filter filter )
{
	vs_data* data = (vs_data*)filter->child;
	if ( data )
	{
		if ( data->analyze_data ) destory_analyze_data( data->analyze_data );
		if ( data->apply_data ) destory_apply_data( data->apply_data );
		free( data );
	}
	filter->close = NULL;
	filter->child = NULL;
	filter->parent.close = NULL;
	mlt_service_close( &filter->parent );
}

extern "C"
{

mlt_filter filter_vidstab_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	vs_data* data = (vs_data*)calloc( 1, sizeof(vs_data) );

	if ( filter && data )
	{
		data->analyze_data = NULL;
		data->apply_data = NULL;

		filter->close = filter_close;
		filter->child = data;
		filter->process = process_filter;

		mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

		//properties for analyze
		mlt_properties_set(properties, "shakiness", "4");
		mlt_properties_set(properties, "accuracy", "4");
		mlt_properties_set(properties, "stepsize", "6");
		mlt_properties_set(properties, "algo", "1");
		mlt_properties_set(properties, "mincontrast", "0.3");
		mlt_properties_set(properties, "show", "0");
		mlt_properties_set(properties, "tripod", "0");

		// properties for apply
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
	else
	{
		if( filter )
		{
			mlt_filter_close( filter );
		}

		if( data )
		{
			free( data );
		}

		filter = NULL;
	}
	return filter;
}

}
