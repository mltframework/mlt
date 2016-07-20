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
	mlt_filter fft;
	char* fft_prop_name;
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
	int reverse = mlt_properties_get_int( filter_properties, "reverse" );

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
				double y0 = bins[bin_index - 1];
				double y1 = bins[bin_index];
				double spect_center = band_freq_low + (band_freq_hi - band_freq_low) / 2;
				double prev_freq = bin_freq - bin_width;
				double t = bin_width / ( spect_center - prev_freq );
				mag = y0 + ( y1 - y0 ) * t;
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
		double spect_val = 0;
		if( dB >= threshold )
		{
			spect_val = 1.0 - (dB / threshold);
		}

		if( reverse )
		{
			spectrum[spect_bands - spect_index - 1] = spect_val;
		} else {
			spectrum[spect_index] = spect_val;
		}

		// Calculate the next spectrum point frequency range.
		band_freq_low = band_freq_hi;
		band_freq_hi = band_freq_hi * band_freq_factor;
	}
}

static void draw_spectrum( mlt_filter filter, mlt_frame frame, QImage* qimg )
{
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position position = mlt_filter_get_position( filter, frame );
	mlt_position length = mlt_filter_get_length2( filter, frame );
	mlt_rect rect = mlt_properties_anim_get_rect( filter_properties, "rect", position, length );
	if ( strchr( mlt_properties_get( filter_properties, "rect" ), '%' ) ) {
		rect.x *= qimg->width();
		rect.w *= qimg->width();
		rect.y *= qimg->height();
		rect.h *= qimg->height();
	}
	char* graph_type = mlt_properties_get( filter_properties, "type" );
	int mirror = mlt_properties_get_int( filter_properties, "mirror" );
	int fill = mlt_properties_get_int( filter_properties, "fill" );
	double tension = mlt_properties_get_double( filter_properties, "tension" );

	QRectF r( rect.x, rect.y, rect.w, rect.h );
	QPainter p( qimg );

	if( mirror ) {
		// Draw two half rectangle instead of one full rectangle.
		r.setHeight( r.height() / 2.0 );
	}

	setup_graph_painter( p, r, filter_properties );
	setup_graph_pen( p, r, filter_properties );

	int bands = mlt_properties_get_int( filter_properties, "bands" );
	if ( bands == 0 ) {
		// "0" means match rectangle width
		bands = r.width();
	}
	float* spectrum = (float*)mlt_pool_alloc( bands * sizeof(float) );

	convert_fft_to_spectrum( filter, frame, bands, spectrum );

	if( graph_type && graph_type[0] == 'b' ) {
		paint_bar_graph( p, r, bands, spectrum );
	} else {
		paint_line_graph( p, r, bands, spectrum, tension, fill );
	}

	if( mirror ) {
		// Second rectangle is mirrored.
		p.translate( 0, r.y() * 2 + r.height() * 2 );
		p.scale( 1, -1 );

		if( graph_type && graph_type[0] == 'b' ) {
			paint_bar_graph( p, r, bands, spectrum );
		} else {
			paint_line_graph( p, r, bands, spectrum, tension, fill );
		}
	}

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
			convert_mlt_to_qimage_rgba( *image, &qimg, *width, *height );
			draw_spectrum( filter, frame, &qimg );
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

	if ( filter && pdata && createQApplicationIfNeeded( MLT_FILTER_SERVICE(filter) ) )
	{
		mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
		mlt_properties_set_int( properties, "_filter_private", 1 );
		mlt_properties_set_int( properties, "frequency_low", 20 );
		mlt_properties_set_int( properties, "frequency_high", 20000 );
		mlt_properties_set( properties, "type", "line" );
		mlt_properties_set( properties, "bgcolor", "0x00000000" );
		mlt_properties_set( properties, "color.1", "0xffffffff" );
		mlt_properties_set( properties, "rect", "0% 0% 100% 100%" );
		mlt_properties_set( properties, "thickness", "0" );
		mlt_properties_set( properties, "fill", "0" );
		mlt_properties_set( properties, "mirror", "0" );
		mlt_properties_set( properties, "reverse", "0" );
		mlt_properties_set( properties, "tension", "0.4" );
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

