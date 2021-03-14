/*
 * filter_tracker.cpp -- Motion tracker
 * Copyright (C) 2016 Jean-Baptiste Mardelle
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt.h>
#include <opencv2/tracking.hpp>
#include <opencv2/core/version.hpp>


typedef struct
{
	cv::Ptr<cv::Tracker> tracker;
	cv::Rect2d boundingBox;
	char * algo;
	mlt_rect startRect;
	bool initialized;
	bool playback;
	bool analyze;
	int last_position;
	int analyse_width;
	int analyse_height;
	mlt_position producer_in;
	mlt_position producer_length;
} private_data;


static void property_changed( mlt_service owner, mlt_filter filter, mlt_event_data event_data )
{
	private_data* pdata = (private_data*)filter->child;
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	const char *name = mlt_event_data_to_string(event_data);

	if ( name && !strcmp( name, "results" ) )
	{
		mlt_properties_anim_get_int( filter_properties, "results", 0, -1 );
		mlt_animation anim = mlt_properties_get_animation( filter_properties, "results" );
		if ( anim && mlt_animation_key_count( anim ) > 0 )
		{
			// We received valid analysis data
			pdata->initialized = true;
			pdata->playback = true;
			return;
		}
		else
                {
			// Analysis data was discarded
			pdata->initialized = false;
			pdata->producer_length = 0;
			pdata->playback = false;
			mlt_properties_set( filter_properties, "_results", NULL );
		}
	}
	if ( !pdata->initialized )
	{
		return;
	}
	if ( !strcmp( name, "rect" ) )
	{
		// The initial rect was changed, we need to reset the tracker with this new rect
		mlt_rect rect = mlt_properties_get_rect( filter_properties, "rect" );
		if ( rect.x != pdata->startRect.x || rect.y != pdata->startRect.y || rect.w != pdata->startRect.w || rect.h != pdata->startRect.h )
		{
			pdata->playback = false;
			pdata->initialized = false;
		}
	}
	else if ( !strcmp( name, "algo" ) )
	{
		char *algo = mlt_properties_get( filter_properties, "algo" );
		if ( pdata->algo && *pdata->algo != '\0' && strcmp( algo, pdata->algo ) )
		{
			pdata->playback = false;
			pdata->initialized = false;
		}
	}
	else if ( !strcmp( name, "_reset" ) )
	{
		mlt_properties_set( filter_properties, "results", NULL );
	}
}

static void apply( mlt_filter filter, private_data* data, int width, int height, int position, int length )
{
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_rect rect = mlt_properties_anim_get_rect( properties, "results", position, length );
	mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
	// Calculate the region now
	double scale_width = mlt_profile_scale_width( profile, width );
	double scale_height = mlt_profile_scale_height( profile, height );
	rect.x *= scale_width;
	rect.w *= scale_width;
	rect.y *= scale_height;
	rect.h *= scale_height;
	data->boundingBox.x = rect.x;
	data->boundingBox.y= rect.y;
	data->boundingBox.width = rect.w;
	data->boundingBox.height = rect.h;
}


static void analyze( mlt_filter filter, cv::Mat cvFrame, private_data* data, int width, int height, int position, int length )
{
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );

	if ( data->analyse_width == -1 )
	{
		// Store analyze width/height
		data->analyse_width = width;
		data->analyse_height = height;
	}
	else if ( data->analyse_width != width || data->analyse_height != height )
	{
		// Frame size changed, reset all stored data
		data->initialized = false;
		data->analyse_width = width;
		data->analyse_height = height;
	}
	// Create tracker and initialize it
	if (!data->initialized)
	{
		// Build tracker
		data->algo = mlt_properties_get( filter_properties, "algo" );
#if CV_VERSION_MAJOR > 3 || (CV_VERSION_MAJOR == 3 && CV_VERSION_MINOR >= 3)
		if ( !data->algo || *data->algo == '\0' || !strcmp(data->algo, "KCF" ) )
		{
			data->tracker = cv::TrackerKCF::create();
		}
#if CV_VERSION_MAJOR > 3 || (CV_VERSION_MAJOR == 3 && CV_VERSION_MINOR >= 4 && CV_VERSION_REVISION >= 2)
		else if ( !strcmp(data->algo, "CSRT" ) )
		{
			data->tracker = cv::TrackerCSRT::create();
		}
		else if ( !strcmp(data->algo, "MOSSE" ) )
		{
			data->tracker = cv::TrackerMOSSE::create();
		}
#endif
		else if ( !strcmp(data->algo, "MIL" ) )
		{
			data->tracker = cv::TrackerMIL::create();
		}
		else if ( !strcmp(data->algo, "TLD" ) )
		{
			data->tracker = cv::TrackerTLD::create();
		}
		else
		{
			data->tracker = cv::TrackerBoosting::create();
		}
#else
		if ( data->algo == NULL || !strcmp(data->algo, "" ) )
		{
			data->tracker = cv::Tracker::create( "KCF" );
		}
		else
		{
			data->tracker = cv::Tracker::create( data->algo );
		}
#endif

		// Discard previous results
		mlt_properties_set( filter_properties, "_results", "" );
		if( data->tracker == NULL )
		{
			fprintf( stderr, "Tracker initialized FAILED\n" );
		}
		else
		{
			data->startRect = mlt_properties_get_rect( filter_properties, "rect" );
			mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
			double scale_width = mlt_profile_scale_width( profile, width );
			double scale_height = mlt_profile_scale_height( profile, height );
			data->startRect.x *= scale_width;
			data->startRect.w *= scale_width;
			data->startRect.y *= scale_height;
			data->startRect.h *= scale_height;

			// Ensure startRect is within frame boundaries
			if ( data->startRect.x > width )
			{
				data->startRect.x = width - 10;
			}
			else
			{
				data->startRect.x = MAX( 0., data->startRect.x );
			}
			if ( data->startRect.y > height )
			{
				data->startRect.y = height - 10;
			}
			else
			{
				data->startRect.y = MAX( 0., data->startRect.y );
			}
			if ( data->startRect.x + data->startRect.w > width )
			{
				data->startRect.w = width - data->startRect.x;
			}
			if ( data->startRect.y + data->startRect.h > height )
			{
				data->startRect.h = height - data->startRect.y;
			}

			data->boundingBox.x = data->startRect.x;
			data->boundingBox.y= data->startRect.y;
			data->boundingBox.width = data->startRect.w;
			data->boundingBox.height = data->startRect.h;
			if ( data->boundingBox.width <1 ) {
				data->boundingBox.width = 50;
			}
			if ( data->boundingBox.height <1 ) {
				data->boundingBox.height = 50;
			}
			if ( data->tracker->init( cvFrame, data->boundingBox ) ) {
				data->initialized = true;
				data->analyze = true;
				data->last_position = position - 1;
			}
			// init anim property
			mlt_properties_anim_get_int( filter_properties, "_results", 0, length );
			mlt_animation anim = mlt_properties_get_animation( filter_properties, "_results" );
			if ( anim == NULL ) {
				fprintf( stderr, "animation initialized FAILED\n" );
			}
		}
	}
	else
	{
		data->tracker->update( cvFrame, data->boundingBox );
	}
	if( data->analyze && position != data->last_position + 1 )
	{
		// We are in real time, do not try to store data
		data->analyze = false;
	}
	if ( !data->analyze )
	{
		return;
	}
	// Store results in temp variable
	mlt_rect rect;
	rect.x = data->boundingBox.x;
	rect.y = data->boundingBox.y;
	rect.w = data->boundingBox.width;
	rect.h = data->boundingBox.height;
	rect.o = 0;

	int steps = mlt_properties_get_int(filter_properties, "steps");
	if ( steps > 1 && position > 0 && position < length - 1 )
	{
		if ( position % steps == 0 )
			mlt_properties_anim_set_rect( filter_properties, "_results", rect, position, length, mlt_keyframe_smooth );
	}
	else
	{
		mlt_properties_anim_set_rect( filter_properties, "_results", rect, position, length, mlt_keyframe_smooth );
	}
	if ( position + 1 == length )
	{
		//Analysis finished, store results
		mlt_animation anim = mlt_properties_get_animation( filter_properties, "_results");
		mlt_properties_set( filter_properties, "results", strdup( mlt_animation_serialize( anim ) ) );
		// Discard temporary data
		mlt_properties_set( filter_properties, "_results", (char*) NULL );

		data->playback = true;
	}
	data->last_position = position;
}


/** Get the image.
*/
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );
	int shape_width = mlt_properties_get_int( filter_properties, "shape_width" );
	int blur = mlt_properties_get_int( filter_properties, "blur" );
	cv::Mat cvFrame;

	private_data* data = (private_data*) filter->child;
	if ( shape_width == 0 && blur == 0 && !data->playback ) {
		error = mlt_frame_get_image( frame, image, format, width, height, 1 );
	}
	else
	{
		*format = mlt_image_rgb24;
		error = mlt_frame_get_image( frame, image, format, width, height, 1 );
		cvFrame = cv::Mat( *height, *width, CV_8UC3, *image );
	}
	if ( data->producer_length == 0 )
	{
		mlt_producer producer = mlt_frame_get_original_producer( frame );
		if ( producer )
		{
			data->producer_in = mlt_producer_get_in( producer );
			data->producer_length = mlt_producer_get_playtime( producer );
		}
	}
	if ( !data->initialized )
	{
		mlt_properties_anim_get_int( filter_properties, "results", 0, data->producer_length );
		mlt_animation anim = mlt_properties_get_animation( filter_properties, "results" );
		if ( anim && mlt_animation_key_count(anim) > 0 )
		{
			data->initialized = true;
			data->playback = true;
		}
	}
	if( data->playback )
	{
		// Clip already analysed, don't re-process
		apply( filter, data, *width, *height, position, data->producer_in + data->producer_length );
	}
	else
	{
		analyze( filter, cvFrame, data, *width, *height, position, data->producer_in + data->producer_length );
	}
	// ensure bounding box is within the frame boundaries or OpenCV will crash
	if ( data->boundingBox.x > *width )
	{
		data->boundingBox.x = *width - 10;
	}
	else
	{
		data->boundingBox.x = MAX(0., data->boundingBox.x);
	}
	if ( data->boundingBox.y > *height )
	{
		data->boundingBox.y = *height - 10;
	}
	else
	{
		data->boundingBox.y = MAX(0., data->boundingBox.y);
	}
	if ( data->boundingBox.x + data->boundingBox.width > *width )
	{
		data->boundingBox.width = *width - data->boundingBox.x;
	}
	if ( data->boundingBox.y + data->boundingBox.height > *height )
	{
		data->boundingBox.height = *height - data->boundingBox.y;
	}

	if ( blur > 0 )
	{
		switch ( mlt_properties_get_int( filter_properties, "blur_type" ) )
		{
			case 1:
				// Gaussian Blur
				cv::GaussianBlur( cvFrame( data->boundingBox ), cvFrame( data->boundingBox ), cv::Size( 0, 0 ), blur );
                                break;
			case 0:
			default:
				// Median Blur
				++blur;
				if ( blur % 2 == 0 )
				{
					// median blur param must be odd and, minimum 3
					++blur;
				}
				cv::medianBlur( cvFrame( data->boundingBox ), cvFrame( data->boundingBox ), blur );
				break;
		}
	}

	// Paint overlay shape
	if ( shape_width != 0 )
        {
		// Get the OpenCV image
		mlt_color shape_color = mlt_properties_get_color( filter_properties, "shape_color" );
		switch ( mlt_properties_get_int( filter_properties, "shape" ) )
                {
		case 2:
			// Arrow
			cv::arrowedLine( cvFrame, cv::Point( data->boundingBox.x + data->boundingBox.width/2, data->boundingBox.y - data->boundingBox.height/2 ), cv::Point( data->boundingBox.x + data->boundingBox.width/2, data->boundingBox.y ), cv::Scalar( shape_color.r, shape_color.g, shape_color.b ), MAX( shape_width, 1 ), 4, 0, .2 );
			break;
		case 1:
			// Ellipse
			{
				cv::RotatedRect bounding = cv::RotatedRect( cv::Point2f( data->boundingBox.x + data->boundingBox.width/2, data->boundingBox.y + data->boundingBox.height/2 ), cv::Size2f( data->boundingBox.width, data->boundingBox.height ), 0);
				cv::ellipse( cvFrame, bounding, cv::Scalar( shape_color.r, shape_color.g, shape_color.b ), shape_width, 1 );
			}
			break;
		case 0:
		default:
			// Rectangle
			cv::rectangle( cvFrame, data->boundingBox, cv::Scalar( shape_color.r, shape_color.g, shape_color.b ), shape_width, 1 );
			break;
		}
	}

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
	return error;
}


/** Filter processing.
*/
static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

static void filter_close( mlt_filter filter )
{
	private_data* data = (private_data*) filter->child;
	free ( data );
	filter->child = NULL;
	filter->close = NULL;
	filter->parent.close = NULL;
	mlt_service_close( &filter->parent );
}

/** Constructor for the filter.
*/

extern "C" {

mlt_filter filter_tracker_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	private_data* data = (private_data*) calloc( 1, sizeof(private_data) );

	if ( filter && data)
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set_int( properties, "shape_width", 1 );
		mlt_properties_set_int( properties, "steps", 5 );
		mlt_properties_set( properties, "algo", "KCF" );
		data->initialized = false;
		data->playback = false;
		data->boundingBox.x = 0;
		data->boundingBox.y= 0;
		data->boundingBox.width = 0;
		data->boundingBox.height = 0;
		data->analyze = false;
		data->last_position = -1;
		data->analyse_width = -1;
		data->analyse_height = -1;
		data->producer_in = 0;
		data->producer_length = 0;
		filter->child = data;

		// Create a unique ID for storing data on the frame
		filter->close = filter_close;
		filter->process = filter_process;

		mlt_events_listen( properties, filter, "property-changed", (mlt_listener)property_changed );
	}
	else
	{
		mlt_log_error( MLT_FILTER_SERVICE( filter ), "Filter tracker failed\n" );

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

