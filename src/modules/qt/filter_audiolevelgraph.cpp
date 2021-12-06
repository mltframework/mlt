/*
 * filter_audiolevel.cpp -- audio level visualization filter
 * Copyright (c) 2021 Meltytech, LLC
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

#include "common.h"
#include "graph.h"
#include <framework/mlt.h>
#include <framework/mlt_log.h>
#include <cstring> // memset
#include <QPainter>
#include <QImage>
#include <QVector>
#include <math.h>  //pow

// Private Types
typedef struct
{
	mlt_filter levels_filter;
	int preprocess_warned;
} private_data;

static int filter_get_audio( mlt_frame frame, void** buffer, mlt_audio_format* format, int* frequency, int* channels, int* samples )
{
	mlt_filter filter = (mlt_filter)mlt_frame_pop_audio( frame );
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	private_data* pdata = (private_data*)filter->child;

	// Create the audiolevel filter the first time.
	if( !pdata->levels_filter )
	{
		mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE(filter) );
		pdata->levels_filter = mlt_factory_filter( profile, "audiolevel", NULL );
		if( !pdata->levels_filter )
		{
			mlt_log_warning( MLT_FILTER_SERVICE(filter), "Unable to create audiolevel filter.\n" );
			return 1;
		}
	}

	// The service must stay locked while using the private data
	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	// Perform audio level processing on the frame
	mlt_filter_process( pdata->levels_filter, frame );
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

	return 0;
}

double get_level_from_frame( mlt_frame frame, int channel )
{
	char prop_str[30];
	snprintf ( prop_str, 30, "meta.media.audio_level.%d", channel );
	return mlt_properties_get_double( MLT_FRAME_PROPERTIES(frame), prop_str);
}

static void convert_levels( mlt_filter filter, mlt_frame frame, int channels, float* levels )
{
	private_data* pdata = (private_data*)filter->child;
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties levels_properties = MLT_FILTER_PROPERTIES( pdata->levels_filter );
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	int reverse = mlt_properties_get_int( filter_properties, "reverse" );
	int real_channels = mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "audio_channels" );
	if (real_channels == 0) real_channels = 1;

	int chan_index = 0;
	for( chan_index = 0; chan_index < channels; chan_index++ )
	{
		double level = 0.0;
		if ( channels == 1 )
		{
			// For 1 channel display, average all channel levels
			int real_chan_index = 0;
			for ( real_chan_index = 0; real_chan_index < real_channels; real_chan_index ++ )
			{
				level += get_level_from_frame( frame, real_chan_index );
			}
			level /= real_channels;
		} else {
			int real_chan_index = chan_index % real_channels;
			level = get_level_from_frame( frame, real_chan_index );
		}

		if( reverse )
		{
			levels[channels - chan_index - 1] = level;
		} else {
			levels[chan_index] = level;
		}
	}
}

static void draw_levels( mlt_filter filter, mlt_frame frame, QImage* qimg, int width, int height )
{
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
	mlt_rect rect = mlt_properties_anim_get_rect( filter_properties, "rect", position, length );
	if ( strchr( mlt_properties_get( filter_properties, "rect" ), '%' ) ) {
		rect.x *= qimg->width();
		rect.w *= qimg->width();
		rect.y *= qimg->height();
		rect.h *= qimg->height();
	}
	double scale = mlt_profile_scale_width(profile, width);
	rect.x *= scale;
	rect.w *= scale;
	scale = mlt_profile_scale_height(profile, height);
	rect.y *= scale;
	rect.h *= scale;
	char* graph_type = mlt_properties_get( filter_properties, "type" );
	int mirror = mlt_properties_get_int( filter_properties, "mirror" );
	int segment_gap = mlt_properties_get_int( filter_properties, "segment_gap" ) * scale;
	int segment_width = mlt_properties_get_int( filter_properties, "thickness" ) * scale;
	QVector<QColor> colors = get_graph_colors( filter_properties );

	QRectF r( rect.x, rect.y, rect.w, rect.h );
	QPainter p( qimg );

	if( mirror ) {
		// Draw two half rectangle instead of one full rectangle.
		r.setHeight( r.height() / 2.0 );
	}

	setup_graph_painter( p, r, filter_properties );
	setup_graph_pen( p, r, filter_properties, scale );

	int channels = mlt_properties_get_int( filter_properties, "channels" );
	if ( channels == 0 ) {
		// "0" means use number of channels in the frame
		channels = mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "audio_channels" );
	}
	if ( channels == 0 ) channels = 1;
	float* levels = (float*)mlt_pool_alloc( channels * sizeof(float) );

	convert_levels( filter, frame, channels, levels );

	if( graph_type && graph_type[0] == 'b' ) {
		paint_bar_graph( p, r, channels, levels );
	} else {
		paint_segment_graph( p, r, channels, levels, segment_gap, colors, segment_width );
	}

	if( mirror ) {
		// Second rectangle is mirrored.
		p.translate( 0, r.y() * 2 + r.height() * 2 );
		p.scale( 1, -1 );
		if( graph_type && graph_type[0] == 'b' ) {
			paint_bar_graph( p, r, channels, levels );
		} else {
			paint_segment_graph( p, r, channels, levels, segment_gap, colors, segment_width );
		}
	}

	mlt_pool_release( levels );

	p.end();
}
/** Get the image.
*/
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = (mlt_filter)mlt_frame_pop_service( frame );
	private_data* pdata = (private_data*)filter->child;
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	if( mlt_properties_get( frame_properties, "meta.media.audio_level.0" ) )
	{
		// Get the current image
		*format = mlt_image_rgba;
		error = mlt_frame_get_image( frame, image, format, width, height, 1 );

		// Draw the audio levels
		if( !error ) {
			QImage qimg( *width, *height, QImage::Format_ARGB32 );
			convert_mlt_to_qimage_rgba( *image, &qimg, *width, *height );
			draw_levels( filter, frame, &qimg, *width, *height );
			convert_qimage_to_mlt_rgba( &qimg, *image, *width, *height );
		}
	} else {
		if ( pdata->preprocess_warned++ == 2 )
		{
			// This filter depends on the consumer processing the audio before
			// the video.
			mlt_log_warning( MLT_FILTER_SERVICE(filter), "Audio not preprocessed.\n" );
		}
		mlt_frame_get_image( frame, image, format, width, height, writable );
	}

	return error;
}

/** Filter processing.
*/
static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	if( mlt_frame_is_test_card( frame ) ) {
		// The producer does not generate video. This filter will create an
		// image on the producer's behalf.
		mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
		mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE(filter) );
		mlt_properties_set_int( frame_properties, "progressive", 1 );
		mlt_properties_set_double( frame_properties, "aspect_ratio", mlt_profile_sar( profile ) );
		mlt_properties_set_int( frame_properties, "meta.media.width", profile->width );
		mlt_properties_set_int( frame_properties, "meta.media.height", profile->height );
		// Tell the framework that there really is an image.
		mlt_properties_set_int( frame_properties, "test_image", 0 );
		// Push a callback to create the image.
		mlt_frame_push_get_image( frame, create_image );
	}

	mlt_frame_push_audio( frame, filter );
	mlt_frame_push_audio( frame, (void*)filter_get_audio );
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

static void filter_close( mlt_filter filter )
{
	private_data* pdata = (private_data*)filter->child;

	if ( pdata )
	{
		mlt_filter_close( pdata->levels_filter );
		free( pdata );
	}
	filter->child = NULL;
	filter->close = NULL;
	filter->parent.close = NULL;
	mlt_service_close( &filter->parent );
}

/** Constructor for the filter.
*/

extern "C" {

mlt_filter filter_audiolevelgraph_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if ( filter && pdata && createQApplicationIfNeeded( MLT_FILTER_SERVICE(filter) ) )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set_int( properties, "_filter_private", 1 );
		mlt_properties_set( properties, "type", "bar" );
		mlt_properties_set( properties, "bgcolor", "0x00000000" );
		mlt_properties_set( properties, "color.1", "0xffffffff" );
		mlt_properties_set( properties, "rect", "0% 0% 100% 100%" );
		mlt_properties_set( properties, "thickness", "0" );
		mlt_properties_set( properties, "fill", "0" );
		mlt_properties_set( properties, "mirror", "0" );
		mlt_properties_set( properties, "reverse", "0" );
		mlt_properties_set( properties, "angle", "0" );
		mlt_properties_set( properties, "gorient", "v" );
		mlt_properties_set_int( properties, "channels", 2 );
		mlt_properties_set_int( properties, "segment_gap", 10 );

		pdata->levels_filter = 0;

		filter->close = filter_close;
		filter->process = filter_process;
		filter->child = pdata;
	}
	else
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Filter audio level graph failed\n" );

		if( filter )
		{
			mlt_filter_close( filter );
		}

		if( pdata )
		{
			free( pdata );
		}

		filter = NULL;
	}
	return filter;
}

}

