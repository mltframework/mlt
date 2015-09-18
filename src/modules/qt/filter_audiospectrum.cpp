/*
 * filter_audiospectrum.cpp -- audio spectrum visualization filter
 * Copyright (c) 2015 Meltytech, LLC
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "common.h"
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
	mlt_filter fft;
	char* fft_prop_name;
	int preprocess_warned;
} private_data;

static inline double linear_interpolate( double y1, double y2, double t )
{
	return y1 + ( y2 - y1 ) * t;
}

static inline double catmull_rom_interpolate( double y0, double y1, double y2, double y3, double t )
{
	double t2 = t * t;
	double a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
	double a1 = y0 - 2.5 * y1 + 2 * y2 - 0.5 * y3;
	double a2 = -0.5 * y0 + 0.5 * y2;
	double a3 = y1;
	return a0 * t * t2 + a1 * t2 + a2 * t + a3;
}

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

	// The service must stay locked while using the private data
	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	// Perform FFT processing on the frame
	mlt_filter_process( pdata->fft, frame );
	mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );

	float* bins = (float*)mlt_properties_get_data( fft_properties, "bins", NULL );

	if( bins )
	{
		double window_level = mlt_properties_get_double( fft_properties, "window_level" );
		int bin_count = mlt_properties_get_int( fft_properties, "bin_count" );
		size_t bins_size = bin_count * sizeof(float);
		float* save_bins = (float*)mlt_pool_alloc( bins_size );

		if( window_level == 1.0 )
		{
			memcpy( save_bins, bins, bins_size );
		} else {
			memset( save_bins, 0, bins_size );
		}

		// Save the bin data as a property on the frame to be used in get_image()
		mlt_properties_set_data( MLT_FRAME_PROPERTIES(frame), pdata->fft_prop_name, save_bins, bins_size, mlt_pool_release, NULL );
	}

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

	return 0;
}

static void convert_fft_to_spectrum( mlt_filter filter, mlt_frame frame, int spect_bands, float* spectrum )
{
	private_data* pdata = (private_data*)filter->child;
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties fft_properties = MLT_FILTER_PROPERTIES( pdata->fft );
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
	double low_freq = mlt_properties_get_int( filter_properties, "frequency_low" );
	double hi_freq = mlt_properties_get_int( filter_properties, "frequency_high" );
	int bin_count = mlt_properties_get_int( fft_properties, "bin_count" );
	double bin_width = mlt_properties_get_double( fft_properties, "bin_width" );
	float* bins = (float*)mlt_properties_get_data( frame_properties, pdata->fft_prop_name, NULL );
	double threshold = mlt_properties_get_int( filter_properties, "threshold" );

	// Map the linear fft bin frequencies to a log scale spectrum.
	double band_freq_factor = pow( hi_freq / low_freq, 1.0 / (double)spect_bands );
	double band_freq_low = low_freq;
	double band_freq_hi = band_freq_low * band_freq_factor;
	int bin_index = 0;
	int spect_index = 0;
	double bin_freq = 0;

	// Skip bins that occur before the low frequency of the spectrum
	while( bin_freq < band_freq_low )
	{
		bin_freq += bin_width;
		bin_index ++;
	}

	for( spect_index = 0; spect_index < spect_bands && bin_index < bin_count; spect_index++ )
	{
		float mag = 0.0;

		if( bin_freq > band_freq_hi )
		{
			// There is no bin for this point. Interpolate between the two closest.
			if( bin_index == 0 )
			{
				mag = bins[bin_index];
			}
			else
			{
				double spect_center = band_freq_low + (band_freq_hi - band_freq_low) / 2;
				mag = linear_interpolate( bins[bin_index - 1], bins[bin_index], spect_center - bin_freq + bin_width );
			}
		}
		else
		{
			// Find the bin frequency with the greatest magnitude in the range for
			// this spectrum point.
			while( bin_freq < band_freq_hi && bin_index < bin_count )
			{
				if( mag < bins[bin_index] )
				{
					mag = bins[bin_index];
				}

				bin_freq += bin_width;
				bin_index ++;
			}
		}

		// Scale the magnitude to the range 0.0-1.0 based on dB
		double dB = mag > 0.0 ? 20 * log10( mag ) : -1000.0;
		if( dB >= threshold )
		{
			spectrum[spect_index] = 1.0 - (dB / threshold);
		} else {
			spectrum[spect_index] = 0;
		}

		// Calculate the next spectrum point frequency range.
		band_freq_low = band_freq_hi;
		band_freq_hi = band_freq_hi * band_freq_factor;
	}
}

static void paint_spectrum( QPainter& p, QRectF& rect, int spect_bands, float* spectrum, int fill )
{
	int width = rect.width();
	int height = rect.height();

	if( spect_bands <= width ) {
//		// For each x position on the waveform, find the sample value that
//		// applies to that position and draw a point at that location.
		QPoint point(0, height - spectrum[0] * height);
		QPoint lastPoint = point;
		int p0 = -1;
		int p1 = 0;
		int p2 = 1;
		int p3 = 2;
		double pixelsPerPoint = (double)(spect_bands - 1) / (double)width;
		for ( int x = 1; x < width; x++ )
		{
			double pn = (double)x * pixelsPerPoint;
			if( (int)pn > p1 )
			{
				p0++;
				p1++;
				p2++;
				p3++;
			}
			double t = pn - (double)p1;
			double y = 0;

			if( p0 < 0 || p3 >= spect_bands )
			{

				y = linear_interpolate( spectrum[p1], spectrum[p2], t );
			}
			else
			{
				y = catmull_rom_interpolate( spectrum[p0], spectrum[p1], spectrum[p2], spectrum[p3], t );
			}

			point.setX( x + rect.x() );
			point.setY( height - y * (double)height );
			p.drawLine( lastPoint, point );
			lastPoint = point;
		}
	} else { // width < spect_bands
		// For each x position on the waveform, find the min and max sample
		// values that apply to that position. Draw a vertical line from the
		// min value to the max value.
		QPoint high;
		QPoint low;
		qreal max = spectrum[0];
		qreal min = max;
		int lastX = 0;
		for ( int index = 0; index <= spect_bands; index++ )
		{
			int x = ( index * width ) / spect_bands;
			if ( x != lastX ) {
				// The min and max have been determined for the previous x
				// So draw the line
				high.setX( lastX + rect.x() );
				high.setY( height - max * height );
				low.setX( lastX + rect.x() );
				low.setY( height - min * height );

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
			if ( spectrum[index] > max ) max = spectrum[index];
			if ( spectrum[index] < min ) min = spectrum[index];
		}
	}
}

static void setup_pen( QPainter& p, QRectF& rect, mlt_properties filter_properties )
{
	int thickness = mlt_properties_get_int( filter_properties, "thickness" );
	QString gorient = mlt_properties_get( filter_properties, "gorient" );
	QVector<QColor> colors;
	bool color_found = true;

	QPen pen;
	pen.setWidth( qAbs(thickness) );

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
		pen.setBrush(Qt::white);
	} else if ( colors.size() == 1 ) {
		// Only use one color
		pen.setBrush(colors[0]);
	} else {
		QLinearGradient gradient;
		if( gorient.startsWith("h", Qt::CaseInsensitive) ) {
			gradient.setStart ( rect.x(), rect.y() );
			gradient.setFinalStop ( rect.x() + rect.width(), rect.y() );
		} else { // Vertical
			gradient.setStart ( rect.x(), rect.y() );
			gradient.setFinalStop ( rect.x(), rect.y() + rect.height() );
		}

		qreal step = 1.0 / ( colors.size() - 1 );
		for( int i = 0; i < colors.size(); i++ )
		{
			gradient.setColorAt( (qreal)i * step, colors[i] );
		}
		pen.setBrush(gradient);
	}

	p.setPen(pen);
}

static void draw_spectrum( mlt_filter filter, mlt_frame frame, QImage* qimg )
{
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	mlt_color bg_color = mlt_properties_get_color( filter_properties, "bgcolor" );
	double angle = mlt_properties_get_double( filter_properties, "angle" );
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

	p.setRenderHint(QPainter::Antialiasing);

	// Fill background
	if ( bg_color.r || bg_color.g || bg_color.g || bg_color.a ) {
		QColor qbgcolor( bg_color.r, bg_color.g, bg_color.b, bg_color.a );
		p.fillRect( 0, 0, qimg->width(), qimg->height(), qbgcolor );
	}

	// Apply rotation
	if ( angle ) {
		p.translate( r.x() + r.width() / 2, r.y() + r.height() / 2 );
		p.rotate( angle );
		p.translate( -(r.x() + r.width() / 2), -(r.y() + r.height() / 2) );
	}

	setup_pen( p, r, filter_properties );

	int bands = mlt_properties_get_int( filter_properties, "bands" );
	if ( bands == 0 ) {
		// "0" means match rectangle width
		bands = r.width();
	}
	float* spectrum = (float*)mlt_pool_alloc( bands * sizeof(float) );
	convert_fft_to_spectrum( filter, frame, bands, spectrum );
	paint_spectrum( p, r, bands, spectrum, fill );
	mlt_pool_release( spectrum );

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

	if( mlt_properties_get_data( frame_properties, pdata->fft_prop_name, NULL ) )
	{
		// Get the current image
		*format = mlt_image_rgb24a;
		error = mlt_frame_get_image( frame, image, format, width, height, 1 );

		// Draw the spectrum
		if( !error ) {
			QImage qimg( *width, *height, QImage::Format_ARGB32 );
			copy_mlt_to_qimage_rgba( *image, &qimg );
			draw_spectrum( filter, frame, &qimg );
			copy_qimage_to_mlt_rgba( &qimg, *image );
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

static int create_image( mlt_frame frame, uint8_t **image, mlt_image_format *image_format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	*image_format = mlt_image_rgb24a;

	// Use the width and height suggested by the rescale filter.
	if( mlt_properties_get_int( frame_properties, "rescale_width" ) > 0 )
		*width = mlt_properties_get_int( frame_properties, "rescale_width" );
	if( mlt_properties_get_int( frame_properties, "rescale_height" ) > 0 )
		*height = mlt_properties_get_int( frame_properties, "rescale_height" );
	// If no size is requested, use native size.
	if( *width <=0 )
		*width = mlt_properties_get_int( frame_properties, "meta.media.width" );
	if( *height <=0 )
		*height = mlt_properties_get_int( frame_properties, "meta.media.height" );

	int size = mlt_image_format_size( *image_format, *width, *height, NULL );
	*image = static_cast<uint8_t*>( mlt_pool_alloc( size ) );
	memset( *image, 0, size ); // Transparent
	mlt_frame_set_image( frame, *image, size, mlt_pool_release );

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
		free( pdata->fft_prop_name );
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

mlt_filter filter_audiospectrum_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	private_data* pdata = (private_data*)calloc( 1, sizeof(private_data) );

	if ( filter && pdata )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set_int( properties, "_filter_private", 1 );
		mlt_properties_set_int( properties, "frequency_low", 20 );
		mlt_properties_set_int( properties, "frequency_high", 20000 );
		mlt_properties_set( properties, "color.1", "0xffffffff" );
		mlt_properties_set( properties, "rect", "0% 0% 100% 100%" );
		mlt_properties_set( properties, "thickness", "0" );
		mlt_properties_set( properties, "fill", "0" );
		mlt_properties_set( properties, "angle", "0" );
		mlt_properties_set( properties, "gorient", "v" );
		mlt_properties_set_int( properties, "bands", 31 );
		mlt_properties_set_double( properties, "threshold", -60.0 );
		mlt_properties_set_int( properties, "window_size", 8192 );

		// Create a unique ID for storing data on the frame
		pdata->fft_prop_name = (char*)calloc( 1, 20 );
		snprintf( pdata->fft_prop_name, 20, "fft.%p", filter );
		pdata->fft_prop_name[20 - 1] = '\0';

		pdata->fft = 0;

		filter->close = filter_close;
		filter->process = filter_process;
		filter->child = pdata;
	}
	else
	{
		mlt_log_error( MLT_FILTER_SERVICE(filter), "Filter audio spectrum failed\n" );

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

