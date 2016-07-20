/*
 * filter_lightshow.cpp -- animate color to the audio
 * Copyright (C) 2015 Meltytech, LLC
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
#include <framework/mlt.h>
#include <stdlib.h> // calloc(), free()
#include <math.h>   // sin()
#include <QPainter>
#include <QImage>

// Private Constants
static const double PI = 3.14159265358979323846;

// Private Types
typedef struct
{
	mlt_filter fft;
	char* mag_prop_name;
	int rel_pos;
	int preprocess_warned;
} private_data;

static int filter_get_audio( mlt_frame frame, void** buffer, mlt_audio_format* format, int* frequency, int* channels, int* samples )
{
	mlt_filter filter = (mlt_filter)mlt_frame_pop_audio( frame );
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	private_data* pdata = (private_data*)filter->child;

	// Create the FFT filter the first time.
	if( !pdata->fft )
	{
		mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE(filter) );
		pdata->fft = mlt_factory_filter( profile, "fft", NULL );
		mlt_properties_set_int( MLT_FILTER_PROPERTIES( pdata->fft ), "window_size",
				mlt_properties_get_int( filter_properties, "window_size" ) );
		if( !pdata->fft )
		{
			mlt_log_warning( MLT_FILTER_SERVICE(filter), "Unable to create FFT.\n" );
			return 1;
		}
	}

	mlt_properties fft_properties = MLT_FILTER_PROPERTIES( pdata->fft );
	double low_freq = mlt_properties_get_int( filter_properties, "frequency_low" );
	double hi_freq = mlt_properties_get_int( filter_properties, "frequency_high" );
	double threshold = mlt_properties_get_int( filter_properties, "threshold" );
	double osc = mlt_properties_get_int( filter_properties, "osc" );
	float peak = 0;

	// The service must stay locked while using the private data
	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	// Perform FFT processing on the frame
	mlt_filter_process( pdata->fft, frame );
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	float* bins = (float*)mlt_properties_get_data( fft_properties, "bins", NULL );
	double window_level = mlt_properties_get_double( fft_properties, "window_level" );

	if( bins && window_level == 1.0 )
	{
		// Find the peak FFT magnitude in the configured range of frequencies
		int bin_count = mlt_properties_get_int( fft_properties, "bin_count" );
		double bin_width = mlt_properties_get_double( fft_properties, "bin_width" );
		int bin = 0;
		for( bin = 0; bin < bin_count; bin++ )
		{
			double F = bin_width * (double)bin;
			if( F >= low_freq && F <= hi_freq )
			{
				if( bins[bin] > peak )
				{
					peak = bins[bin];
				}
			}
		}
	}

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

	// Scale the magnitude to dB and apply oscillation
	double dB = peak > 0.0 ? 20 * log10( peak ) : -1000.0;
	double mag = 0.0;
	if( dB >= threshold )
	{
		// Scale to range 0.0-1.0
		mag = 1 - (dB / threshold);
		if( osc != 0 )
		{
			// Apply the oscillation
			double fps = mlt_profile_fps( mlt_service_profile( MLT_FILTER_SERVICE(filter) ) );
			double t = pdata->rel_pos / fps;
			mag = mag * sin( 2 * PI * osc * t );
		}
		pdata->rel_pos++;
	} else {
		pdata->rel_pos = 1;
		mag = 0;
	}

	// Save the magnitude as a property on the frame to be used in get_image()
	mlt_properties_set_double( MLT_FRAME_PROPERTIES(frame), pdata->mag_prop_name, mag );

	return 0;
}

static void setup_pen( QPainter& p, QRect& rect, mlt_properties filter_properties )
{
	QVector<QColor> colors;
	bool color_found = true;

	// Find user specified colors for the gradient
	while( color_found ) {
		QString prop_name = QString("color.") + QString::number(colors.size() + 1);
		if( mlt_properties_get(filter_properties, prop_name.toUtf8().constData() ) ) {
			mlt_color mcolor = mlt_properties_get_color( filter_properties, prop_name.toUtf8().constData() );
			colors.append( QColor( mcolor.r, mcolor.g, mcolor.b, mcolor.a ) );
		} else {
			color_found = false;
		}
	}

	if( !colors.size() ) {
		// No color specified. Just use white.
		p.setBrush( Qt::white );
	} else if( colors.size() == 1 ) {
		// Only use one color
		p.setBrush( colors[0] );
	} else {
		// Use Gradient
		qreal sx = 1.0;
		qreal sy = 1.0;
		qreal dx = rect.x();
		qreal dy = rect.y();
		qreal radius = rect.width() / 2;

		if ( rect.width() > rect.height() )
		{
			radius = rect.height() / 2;
			sx = (qreal)rect.width() / (qreal)rect.height();
		} else if ( rect.height() > rect.width() ) {
			radius = rect.width() / 2;
			sy = (qreal)rect.height() / (qreal)rect.width();
		}

		QPointF center( radius, radius );
		QRadialGradient gradient( center, radius );

		qreal step = 1.0 / ( colors.size() - 1 );
		for( int i = 0; i < colors.size(); i++ )
		{
			gradient.setColorAt( (qreal)i * step, colors[i] );
		}

		QBrush brush( gradient );
		QTransform transform( sx, 0.0, 0.0, 0.0, sy, 0.0, dx, dy, 1.0 );
		brush.setTransform( transform );
		p.setBrush( brush );
	}
	p.setPen( QColor(0,0,0,0) ); // Clear pen
}

static void draw_light( mlt_properties filter_properties, QImage* qimg, mlt_rect* rect, double mag )
{
	QPainter p( qimg );
	QRect r( rect->x, rect->y, rect->w, rect->h );
	p.setRenderHint( QPainter::Antialiasing );
	// Output transparency = input transparency
	p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
	setup_pen( p, r, filter_properties );
	p.setOpacity( mag );
	p.drawRect( r );
	p.end();
}

/** Get the image.
*/
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = (mlt_filter)mlt_frame_pop_service( frame );

	private_data* pdata = (private_data*)filter->child;
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	if( mlt_properties_get( frame_properties, pdata->mag_prop_name ) )
	{
		double mag = mlt_properties_get_double( frame_properties, pdata->mag_prop_name );
		mlt_position position = mlt_filter_get_position( filter, frame );
		mlt_position length = mlt_filter_get_length2( filter, frame );
		mlt_rect rect = mlt_properties_anim_get_rect( filter_properties, "rect", position, length );

		// Get the current image
		*format = mlt_image_rgb24a;
		error = mlt_frame_get_image( frame, image, format, width, height, 1 );

		if ( strchr( mlt_properties_get( filter_properties, "rect" ), '%' ) ) {
			rect.x *= *width;
			rect.w *= *width;
			rect.y *= *height;
			rect.h *= *height;
		}

		// Draw the light
		if( !error ) {
			QImage qimg( *width, *height, QImage::Format_ARGB32 );
			convert_mlt_to_qimage_rgba( *image, &qimg, *width, *height );
			draw_light( filter_properties, &qimg, &rect, mag );
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
		mlt_filter_close( pdata->fft );
		free( pdata->mag_prop_name );
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

mlt_filter filter_lightshow_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if ( filter && pdata && createQApplicationIfNeeded( MLT_FILTER_SERVICE(filter) ) )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set_int( properties, "_filter_private", 1 );
		mlt_properties_set_int( properties, "frequency_low", 20 );
		mlt_properties_set_int( properties, "frequency_high", 20000 );
		mlt_properties_set_double( properties, "threshold", -30.0 );
		mlt_properties_set_double( properties, "osc", 5.0 );
		mlt_properties_set( properties, "color.1", "0xffffffff" );
		mlt_properties_set( properties, "rect", "0% 0% 100% 100%" );
		mlt_properties_set_int( properties, "window_size", 2048 );

		// Create a unique ID for storing data on the frame
		pdata->mag_prop_name = (char*)calloc( 1, 20 );
		snprintf( pdata->mag_prop_name, 20, "fft_mag.%p", filter );
		pdata->mag_prop_name[20 - 1] = '\0';

		pdata->fft = 0;

		filter->close = filter_close;
		filter->process = filter_process;
		filter->child = pdata;
	}
	else
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Filter lightshow failed\n" );

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

