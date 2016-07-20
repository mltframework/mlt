/*
 * filter_audiowaveform.cpp -- audio waveform visualization filter
 * Copyright (c) 2015-2016 Meltytech, LLC
 * Author: Brian Matherly <code@brianmatherly.com>
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

static const qreal MAX_AMPLITUDE = 32768.0;
static bool preprocess_warned = false;

static void paint_waveform( QPainter& p, QRectF& rect, int16_t* audio, int samples, int channels, int fill )
{
	int width = rect.width();
	const int16_t* q = audio;

	qreal half_height = rect.height() / 2.0;
	qreal center_y = rect.y() + half_height;

	if( samples < width ) {
		// For each x position on the waveform, find the sample value that
		// applies to that position and draw a point at that location.
		QPoint point(0, *q * half_height / MAX_AMPLITUDE + center_y);
		QPoint lastPoint = point;
		int lastSample = 0;
		for ( int x = 0; x < width; x++ )
		{
			int sample = ( x * samples ) / width;
			if ( sample != lastSample ) {
				lastSample = sample;
				q += channels;
			}

			lastPoint.setX( x + rect.x() );
			lastPoint.setY( point.y() );
			point.setX( x + rect.x() );
			point.setY( *q * half_height / MAX_AMPLITUDE + center_y );

			if ( fill ) {
				// Draw the line all the way to 0 to "fill" it in.
				if ( ( point.y() > center_y && lastPoint.y() > center_y ) ||
					 ( point.y() < center_y && lastPoint.y() < center_y ) ) {
					lastPoint.setY( center_y );
				}
			}

			if ( point.y() == lastPoint.y() ) {
				p.drawPoint( point );
			} else {
				p.drawLine( lastPoint, point );
			}
		}
	} else {
		// For each x position on the waveform, find the min and max sample
		// values that apply to that position. Draw a vertical line from the
		// min value to the max value.
		QPoint high;
		QPoint low;
		qreal max = *q;
		qreal min = *q;
		int lastX = 0;
		for ( int s = 0; s <= samples; s++ )
		{
			int x = ( s * width ) / samples;
			if ( x != lastX ) {
				// The min and max have been determined for the previous x
				// So draw the line

				if ( fill ) {
					// Draw the line all the way to 0 to "fill" it in.
					if ( max > 0 && min > 0 ) {
						min = 0;
					} else if ( min < 0 && max < 0 ) {
						max = 0;
					}
				}

				high.setX( lastX + rect.x() );
				high.setY( max * half_height / MAX_AMPLITUDE + center_y );
				low.setX( lastX + rect.x() );
				low.setY( min * half_height / MAX_AMPLITUDE + center_y );

				if ( high.y() == low.y() ) {
					p.drawPoint( high );
				} else {
					p.drawLine( low, high );
				}
				lastX = x;
				// Swap max and min so that the next line picks up where
				// this one left off.
				int tmp = max;
				max = min;
				min = tmp;
			}
			if ( *q > max ) max = *q;
			if ( *q < min ) min = *q;
			q += channels;
		}
	}
}

static void draw_waveforms( mlt_filter filter, mlt_frame frame, QImage* qimg, int16_t* audio, int channels, int samples )
{
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	int show_channel = mlt_properties_get_int( filter_properties, "show_channel" );
	int fill = mlt_properties_get_int( filter_properties, "fill" );
	mlt_rect rect = mlt_properties_anim_get_rect( filter_properties, "rect", position, length );
	if ( strchr( mlt_properties_get( filter_properties, "rect" ), '%' ) ) {
		rect.x *= qimg->width();
		rect.w *= qimg->width();
		rect.y *= qimg->height();
		rect.h *= qimg->height();
	}

	QRectF r( rect.x, rect.y, rect.w, rect.h );

	QPainter p( qimg );

	setup_graph_painter( p, r, filter_properties );

	if ( show_channel == 0 ) // Show all channels
	{
		QRectF c_rect = r;
		qreal c_height = r.height() / channels;
		for ( int c = 0; c < channels; c++ )
		{
			// Divide the rectangle into smaller rectangles for each channel.
			c_rect.setY( r.y() + c_height * c );
			c_rect.setHeight( c_height );
			setup_graph_pen( p, c_rect, filter_properties );
			paint_waveform( p, c_rect, audio + c, samples, channels, fill );
		}
	} else if ( show_channel > 0 ) { // Show one specific channel
		if ( show_channel > channels ) {
			// Sanity
			show_channel = 1;
		}
		setup_graph_pen( p, r, filter_properties );
		paint_waveform( p, r, audio + show_channel - 1, samples, channels, fill );
	}

	p.end();
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *image_format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	mlt_filter filter = (mlt_filter)mlt_frame_pop_service( frame );
	int samples = 0;
	int channels = 0;
	int frequency = 0;
	mlt_audio_format audio_format = mlt_audio_s16;
	int16_t* audio = (int16_t*)mlt_properties_get_data( frame_properties, "audio", NULL );

	if ( !audio && !preprocess_warned ) {
		// This filter depends on the consumer processing the audio before the
		// video. If the audio is not preprocessed, this filter will process it.
		// If this filter processes the audio, it could cause confusion for the
		// consumer if it needs different audio properties.
		mlt_log_warning( MLT_FILTER_SERVICE(filter), "Audio not preprocessed. Potential audio distortion.\n" );
		preprocess_warned = true;
	}

	*image_format = mlt_image_rgb24a;

	// Get the current image
	error = mlt_frame_get_image( frame, image, image_format, width, height, writable );

	// Get the audio
	if( !error ) {
		frequency = mlt_properties_get_int( frame_properties, "audio_frequency" );
		if (!frequency) {
			frequency = 48000;
		}
		channels = mlt_properties_get_int( frame_properties, "audio_channels" );
		if (!channels) {
			channels = 2;
		}
		samples = mlt_properties_get_int( frame_properties, "audio_samples" );
		if (!samples) {
			mlt_producer producer = mlt_frame_get_original_producer( frame );
			double fps = mlt_producer_get_fps( mlt_producer_cut_parent( producer ) );
			samples = mlt_sample_calculator( fps, frequency, mlt_frame_get_position( frame ) );
		}

		error = mlt_frame_get_audio( frame, (void**)&audio, &audio_format, &frequency, &channels, &samples );
	}

	// Draw the waveforms
	if( !error ) {
		QImage qimg( *width, *height, QImage::Format_ARGB32 );
		convert_mlt_to_qimage_rgba( *image, &qimg, *width, *height );
		draw_waveforms( filter, frame, &qimg, audio, channels, samples );
		convert_qimage_to_mlt_rgba( &qimg, *image, *width, *height );
	}

	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	if( mlt_frame_is_test_card( frame ) ) {
		// The producer does not generate video. This filter will create an
		// image on the producer's behalf.
		mlt_profile profile = mlt_service_profile(
			MLT_PRODUCER_SERVICE( mlt_frame_get_original_producer( frame ) ) );
		mlt_properties_set_int( frame_properties, "progressive", 1 );
		mlt_properties_set_double( frame_properties, "aspect_ratio", mlt_profile_sar( profile ) );
		mlt_properties_set_int( frame_properties, "meta.media.width", profile->width );
		mlt_properties_set_int( frame_properties, "meta.media.height", profile->height );
		// Tell the framework that there really is an image.
		mlt_properties_set_int( frame_properties, "test_image", 0 );
		// Push a callback to create the image.
		mlt_frame_push_get_image( frame, create_image );
	}
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/

extern "C" {

mlt_filter filter_audiowaveform_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();

	if ( !filter ) return NULL;

	if ( !createQApplicationIfNeeded( MLT_FILTER_SERVICE(filter) ) )  {
		mlt_filter_close( filter );
		return NULL;
	}

	filter->process = filter_process;
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties_set( filter_properties, "bgcolor", "0x00000000" );
	mlt_properties_set( filter_properties, "color.1", "0xffffffff" );
	mlt_properties_set( filter_properties, "thickness", "0" );
	mlt_properties_set( filter_properties, "show_channel", "0" );
	mlt_properties_set( filter_properties, "angle", "0" );
	mlt_properties_set( filter_properties, "rect", "0 0 100% 100%" );
	mlt_properties_set( filter_properties, "fill", "0" );
	mlt_properties_set( filter_properties, "gorient", "v" );

	return filter;
}

}
