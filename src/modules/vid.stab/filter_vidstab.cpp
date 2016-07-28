/*
 * filter_vidstab.cpp
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
#include <framework/mlt.h>
#include <framework/mlt_animation.h>
#include "common.h"
}

#include <sstream>
#include <string.h>
#include <assert.h>

typedef struct
{
	VSMotionDetect md;
	FILE* results;
	mlt_position last_position;
} vs_analyze;

typedef struct
{
	VSTransformData td;
	VSTransformConfig conf;
	VSTransformations trans;
} vs_apply;

typedef struct
{
	vs_analyze* analyze_data;
	vs_apply* apply_data;
} vs_data;

static void get_transform_config( VSTransformConfig* conf, mlt_filter filter, mlt_frame frame )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	const char* filterName = mlt_properties_get( properties, "mlt_service" );

	*conf = vsTransformGetDefaultConfig( filterName );
	conf->smoothing = mlt_properties_get_int( properties, "smoothing" );
	conf->maxShift = mlt_properties_get_int( properties, "maxshift" );
	conf->maxAngle = mlt_properties_get_double( properties, "maxangle" );
	conf->crop = (VSBorderType)mlt_properties_get_int( properties, "crop" );
	conf->zoom = mlt_properties_get_int( properties, "zoom" );
	conf->optZoom = mlt_properties_get_int( properties, "optzoom" );
	conf->zoomSpeed = mlt_properties_get_double( properties, "zoomspeed" );
	conf->relative = mlt_properties_get_int( properties, "relative" );
	conf->invert = mlt_properties_get_int( properties, "invert" );
	if ( mlt_properties_get_int( properties, "tripod" ) != 0 )
	{
		// Virtual tripod mode: relative=False, smoothing=0
		conf->relative = 0;
		conf->smoothing = 0;
	}

	// by default a bicubic interpolation is selected
	const char *interps = mlt_properties_get( MLT_FRAME_PROPERTIES( frame ), "rescale.interp" );
	conf->interpolType = VS_BiCubic;
	if ( strcmp( interps, "nearest" ) == 0 || strcmp( interps, "neighbor" ) == 0 )
		conf->interpolType = VS_Zero;
	else if ( strcmp( interps, "tiles" ) == 0 || strcmp( interps, "fast_bilinear" ) == 0 )
		conf->interpolType = VS_Linear;
	else if ( strcmp( interps, "bilinear" ) == 0 )
		conf->interpolType = VS_BiLinear;
}

static int check_apply_config( mlt_filter filter, mlt_frame frame )
{
	vs_apply* apply_data = ((vs_data*)filter->child)->apply_data;

	if( apply_data )
	{
		VSTransformConfig new_conf;
		memset( &new_conf, 0, sizeof(VSTransformConfig) );
		get_transform_config( &new_conf, filter, frame );
		return compare_transform_config( &apply_data->conf, &new_conf );
	}

	return 0;
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

static void init_apply_data( mlt_filter filter, mlt_frame frame, VSPixelFormat vs_format, int width, int height )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	vs_data* data = (vs_data*)filter->child;
	vs_apply* apply_data = (vs_apply*)calloc( 1, sizeof(vs_apply) );
	char* results = mlt_properties_get( properties, "results" );
	char* filename = mlt_properties_get( properties, "filename" );

	// The XML producer can convert "filename" from relative to absolute, but
	// it does not do the same for "results". Therefore, if both exist and
	// "filename" ends with "results", then use "filename" instead.
	if ( results && filename
		 && strlen(filename) >= strlen(results)
		 && !strcmp( &filename[strlen(filename) - strlen(results)], results ) )
	{
		// Convert file name string encoding.
		mlt_properties_from_utf8( properties, "filename", "_filename" );
		filename = mlt_properties_get( properties, "_filename" );
	}
	else
	{
		// Convert file name string encoding.
		mlt_properties_from_utf8( properties, "results", "_results" );
		filename = mlt_properties_get( properties, "_results" );
	}
	mlt_log_info( MLT_FILTER_SERVICE(filter), "Load results from %s\n", filename );

	// Initialize the VSTransformConfig
	memset( apply_data, 0, sizeof( vs_apply ) );
	get_transform_config( &apply_data->conf, filter, frame );

	// Initialize VSTransformData
	VSFrameInfo fi_src, fi_dst;
	vsFrameInfoInit( &fi_src, width, height, vs_format );
	vsFrameInfoInit( &fi_dst, width, height, vs_format );
	vsTransformDataInit( &apply_data->td, &apply_data->conf, &fi_src, &fi_dst );

	// Initialize VSTransformations
	vsTransformationsInit( &apply_data->trans );

	// Load the motions from the analyze step and convert them to VSTransformations
	FILE* f = fopen( filename, "r" );
	VSManyLocalMotions mlms;

	if( vsReadLocalMotionsFile( f, &mlms ) == VS_OK )
	{
		int i = 0;
		mlt_log_info( MLT_FILTER_SERVICE(filter), "Successfully loaded %d motions\n", vs_vector_size( &mlms ) );
		vsLocalmotions2Transforms( &apply_data->td, &mlms, &apply_data->trans );
		vsPreprocessTransforms( &apply_data->td, &apply_data->trans );

		// Free the MultipleLocalMotions
		for( i = 0; i < vs_vector_size( &mlms ); i++ )
		{
			LocalMotions* lms = (LocalMotions*)vs_vector_get( &mlms, i );
			if( lms )
			{
				vs_vector_del( lms );
			}
		}
		vs_vector_del( &mlms );

		data->apply_data = apply_data;
	}
	else
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Can not read results file: %s\n", filename );
		destory_apply_data( apply_data );
		data->apply_data = NULL;
	}

	if( f )
	{
		fclose( f );
	}
}

void destory_analyze_data( vs_analyze* analyze_data )
{
	if ( analyze_data )
	{
		vsMotionDetectionCleanup( &analyze_data->md );
		if( analyze_data->results )
		{
			fclose( analyze_data->results );
			analyze_data->results = NULL;
		}
		free( analyze_data );
	}
}

static void init_analyze_data( mlt_filter filter, mlt_frame frame, VSPixelFormat vs_format, int width, int height )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	vs_data* data = (vs_data*)filter->child;
	vs_analyze* analyze_data = (vs_analyze*)calloc( 1, sizeof(vs_analyze) );
	memset( analyze_data, 0, sizeof(vs_analyze) );

	// Initialize a VSMotionDetectConfig
	const char* filterName = mlt_properties_get( properties, "mlt_service" );
	VSMotionDetectConfig conf = vsMotionDetectGetDefaultConfig( filterName );
	conf.shakiness = mlt_properties_get_int( properties, "shakiness" );
	conf.accuracy = mlt_properties_get_int( properties, "accuracy" );
	conf.stepSize = mlt_properties_get_int( properties, "stepsize" );
	conf.contrastThreshold = mlt_properties_get_double( properties, "mincontrast" );
	conf.show = mlt_properties_get_int( properties, "show" );
	conf.virtualTripod = mlt_properties_get_int( properties, "tripod" );

	// Initialize a VSFrameInfo
	VSFrameInfo fi;
	vsFrameInfoInit( &fi, width, height, vs_format );

	// Initialize the saved VSMotionDetect
	vsMotionDetectInit( &analyze_data->md, &conf, &fi );

	// Initialize the file to save results to
	char* filename = mlt_properties_get( properties, "filename" );
	analyze_data->results = fopen( filename, "w" );
	if ( vsPrepareFile( &analyze_data->md, analyze_data->results ) != VS_OK )
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Can not write to results file: %s\n", filename );
		destory_analyze_data( analyze_data );
		data->analyze_data = NULL;
	}
	else
	{
		data->analyze_data = analyze_data;
	}
}

static int apply_results( mlt_filter filter, mlt_frame frame, uint8_t* vs_image, VSPixelFormat vs_format, int width, int height )
{
	int error = 0;
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	vs_data* data = (vs_data*)filter->child;

	if ( check_apply_config( filter, frame ) ||
		mlt_properties_get_int( properties, "reload" ) )
	{
		mlt_properties_set_int( properties, "reload", 0 );
		destory_apply_data( data->apply_data );
		data->apply_data = NULL;
	}

	// Init transform data if necessary (first time)
	if ( !data->apply_data )
	{
		init_apply_data( filter, frame, vs_format, width, height );
	}

	if( data->apply_data )
	{
		// Apply transformations to this image
		VSTransformData* td = &data->apply_data->td;
		VSTransformations* trans = &data->apply_data->trans;
		VSFrame vsFrame;
		vsFrameFillFromBuffer( &vsFrame, vs_image, vsTransformGetSrcFrameInfo( td ) );
		trans->current = mlt_filter_get_position( filter, frame );
		vsTransformPrepare( td, &vsFrame, &vsFrame );
		VSTransform t = vsGetNextTransform( td, trans );
		vsDoTransform( td, t );
		vsTransformFinish( td );
	}

	return error;
}

static void analyze_image( mlt_filter filter, mlt_frame frame, uint8_t* vs_image, VSPixelFormat vs_format, int width, int height )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	vs_data* data = (vs_data*)filter->child;
	mlt_position pos = mlt_filter_get_position( filter, frame );

	// If any frames are skipped, analysis data will be incomplete.
	if( data->analyze_data && pos != data->analyze_data->last_position + 1 )
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Bad frame sequence\n" );
		destory_analyze_data( data->analyze_data );
		data->analyze_data = NULL;
	}

	if ( !data->analyze_data && pos == 0 )
	{
		// Analysis must start on the first frame
		init_analyze_data( filter, frame, vs_format, width, height );
	}

	if( data->analyze_data )
	{
		// Initialize the VSFrame to be analyzed.
		VSMotionDetect* md = &data->analyze_data->md;
		LocalMotions localmotions;
		VSFrame vsFrame;
		vsFrameFillFromBuffer( &vsFrame, vs_image, &md->fi );

		// Detect and save motions.
		if( vsMotionDetection( md, &localmotions, &vsFrame ) == VS_OK )
		{
			vsWriteToFile( md, data->analyze_data->results, &localmotions);
			vs_vector_del( &localmotions );
		}
		else
		{
			mlt_log_error( MLT_FILTER_SERVICE(filter), "Motion detection failed\n" );
			destory_analyze_data( data->analyze_data );
			data->analyze_data = NULL;
		}

		// Publish the motions if this is the last frame.
		if ( pos + 1 == mlt_filter_get_length2( filter, frame ) )
		{
			mlt_log_info( MLT_FILTER_SERVICE(filter), "Analysis complete\n" );
			destory_analyze_data( data->analyze_data );
			data->analyze_data = NULL;
			mlt_properties_set( properties, "results", mlt_properties_get( properties, "filename" ) );
		}
		else if ( data->analyze_data )
		{
			data->analyze_data->last_position = pos;
		}
	}
}

static int get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = (mlt_filter)mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	uint8_t* vs_image = NULL;
	VSPixelFormat vs_format = PF_NONE;

	// VS only works on progressive frames
	mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "consumer_deinterlace", 1 );

	*format = validate_format( *format );

	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// Convert the received image to a format vid.stab can handle
	if ( !error )
	{
		vs_format = mltimage_to_vsimage( *format, *width, *height, *image, &vs_image );
	}

	if( vs_image )
	{
		mlt_service_lock( MLT_FILTER_SERVICE(filter) );

		char* results = mlt_properties_get( properties, "results" );
		if( results && strcmp( results, "" ) )
		{
			apply_results( filter, frame, vs_image, vs_format, *width, *height );
			vsimage_to_mltimage( vs_image, *image, *format, *width, *height );
		}
		else
		{
			analyze_image( filter, frame, vs_image, vs_format, *width, *height );
			if( mlt_properties_get_int( properties, "show" ) == 1 )
			{
				vsimage_to_mltimage( vs_image, *image, *format, *width, *height );
			}
		}

		mlt_service_unlock( MLT_FILTER_SERVICE(filter) );

		free_vsimage( vs_image, vs_format );
	}

	return error;
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
		mlt_properties_set( properties, "filename", "vidstab.trf" );
		mlt_properties_set( properties, "shakiness", "4" );
		mlt_properties_set( properties, "accuracy", "4" );
		mlt_properties_set( properties, "stepsize", "6" );
		mlt_properties_set( properties, "algo", "1" );
		mlt_properties_set( properties, "mincontrast", "0.3" );
		mlt_properties_set( properties, "show", "0" );
		mlt_properties_set( properties, "tripod", "0" );

		// properties for apply
		mlt_properties_set( properties, "smoothing", "15" );
		mlt_properties_set( properties, "maxshift", "-1" );
		mlt_properties_set( properties, "maxangle", "-1" );
		mlt_properties_set( properties, "crop", "0" );
		mlt_properties_set( properties, "invert", "0" );
		mlt_properties_set( properties, "relative", "1" );
		mlt_properties_set( properties, "zoom", "0" );
		mlt_properties_set( properties, "optzoom", "1" );
		mlt_properties_set( properties, "zoomspeed", "0.25" );
		mlt_properties_set( properties, "reload", "0" );

		mlt_properties_set( properties, "vid.stab.version", LIBVIDSTAB_VERSION );

		init_vslog();
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
