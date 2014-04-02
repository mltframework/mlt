/*
 * transition_vqm.c -- video quality measurement
 * Copyright (c) 2012 Dan Dennedy <dan@dennedy.org>
 * Core psnr and ssim routines based on code from
 *   qsnr (C) 2010 E. Oriani, ema <AT> fastwebnet <DOT> it
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
 */

#include "common.h"
#include <framework/mlt.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <QImage>
#include <QColor>
#include <QPainter>
#include <QPalette>
#include <QFont>
#include <QString>

static double calc_psnr( const uint8_t *a, const uint8_t *b, int size, int bpp )
{
	double mse = 0.0;
	int n = size + 1;

	while ( --n )
	{
		int diff = *a - *b;
		mse += diff * diff;
		a += bpp;
		b += bpp;
	}

	return 10.0 * log10( 255.0 * 255.0 / ( mse == 0 ? 1e-10 : mse/size ) );
}

static double calc_ssim( const uint8_t *a, const uint8_t *b, int width, int height, int window_size, int bpp )
{
	int	windows_x = width / window_size;
	int windows_y = height / window_size;
	double	avg = 0.0;

	if ( !windows_x || !windows_y )
		return 0.0;

	// for each window
	for ( int y = 0; y < windows_y; ++y )
		for ( int x = 0; x < windows_x; ++x )
		{
			int	base_offset = x * window_size + y * window_size * width;
			double	ref_acc = 0.0,
					ref_acc_2 = 0.0,
					cmp_acc = 0.0,
					cmp_acc_2 = 0.0,
					ref_cmp_acc = 0.0;

			// accumulate the pixel values for this window
			for ( int j = 0; j < window_size; ++j )
				for ( int i = 0; i < window_size; ++i )
				{
					uint8_t c_a = a[bpp * (base_offset + j * width + i)];
					uint8_t c_b = b[bpp * (base_offset + j * width + i)];
					ref_acc += c_a;
					ref_acc_2 += c_a * c_a;
					cmp_acc += c_b;
					cmp_acc_2 += c_b * c_b;
					ref_cmp_acc += c_a * c_b;
				}

			// compute the SSIM for this window
			// http://en.wikipedia.org/wiki/SSIM
			// http://en.wikipedia.org/wiki/Variance
			// http://en.wikipedia.org/wiki/Covariance
			double n_samples = window_size * window_size,
					ref_avg = ref_acc / n_samples,
					ref_var = ref_acc_2 / n_samples - ref_avg * ref_avg,
					cmp_avg = cmp_acc / n_samples,
					cmp_var = cmp_acc_2 / n_samples - cmp_avg * cmp_avg,
					ref_cmp_cov = ref_cmp_acc / n_samples - ref_avg * cmp_avg,
					c1 = 6.5025, // (0.01*255.0)^2
					c2 = 58.5225, // (0.03*255)^2
					ssim_num = (2.0 * ref_avg * cmp_avg + c1) * (2.0 * ref_cmp_cov + c2),
					ssim_den = (ref_avg * ref_avg + cmp_avg * cmp_avg + c1) * (ref_var + cmp_var + c2);

			// accumulate the SSIM
			avg += ssim_num / ssim_den;
		}

	// return the average SSIM
	return avg / windows_x / windows_y;
}

static int get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );
	mlt_properties properties = MLT_FRAME_PROPERTIES( a_frame );
	mlt_transition transition = MLT_TRANSITION( mlt_frame_pop_service( a_frame ) );
	uint8_t *b_image;
	int window_size = mlt_properties_get_int( MLT_TRANSITION_PROPERTIES( transition ), "window_size" );
	double psnr[3], ssim[3];

	*format = mlt_image_yuv422;
	mlt_frame_get_image( b_frame, &b_image, format, width, height, writable );
	mlt_frame_get_image( a_frame, image, format, width, height, writable );

	psnr[0] = calc_psnr( *image, b_image, *width * *height, 2 );
	psnr[1] = calc_psnr( *image + 1, b_image + 1, *width * *height / 2, 4 );
	psnr[2] = calc_psnr( *image + 3, b_image + 3, *width * *height / 2, 4 );
	ssim[0] = calc_ssim( *image, b_image, *width, *height, window_size, 2 );
	ssim[1] = calc_ssim( *image + 1, b_image + 1, *width / 2, *height, window_size, 4 );
	ssim[2] = calc_ssim( *image + 3, b_image + 3, *width / 2, *height, window_size, 4 );
	mlt_properties_set_double( properties, "meta.vqm.psnr.y", psnr[0] );
	mlt_properties_set_double( properties, "meta.vqm.psnr.cb", psnr[1] );
	mlt_properties_set_double( properties, "meta.vqm.psnr.cr", psnr[2] );
	mlt_properties_set_double( properties, "meta.vqm.ssim.y", ssim[0] );
	mlt_properties_set_double( properties, "meta.vqm.ssim.cb", ssim[1] );
	mlt_properties_set_double( properties, "meta.vqm.ssim.cr", ssim[2] );
	printf( "%05d %05.2f %05.2f %05.2f %5.3f %5.3f %5.3f\n",
			mlt_frame_get_position( a_frame ), psnr[0], psnr[1], psnr[2],
			ssim[0], ssim[1], ssim[2] );

	// copy the B frame to the bottom of the A frame for comparison
	window_size = mlt_image_format_size( *format, *width, *height, NULL ) / 2;
	memcpy( *image + window_size, b_image + window_size, window_size );

	if ( !mlt_properties_get_int( MLT_TRANSITION_PROPERTIES( transition ), "render" ) )
		return 0;

	// get RGBA image for Qt drawing
	*format = mlt_image_rgb24a;
	mlt_frame_get_image( a_frame, image, format, width, height, 1 );

	// convert mlt image to qimage
	QImage img( *width, *height, QImage::Format_ARGB32 );
	int y = *height + 1;
	uint8_t *src = *image;
	while ( --y )
	{
		QRgb *dst = (QRgb*) img.scanLine( *height - y );
		int x = *width + 1;
		while ( --x )
		{
			*dst++ = qRgba( src[0], src[1], src[2], 255 );
			src += 4;
		}
	}

	// setup Qt drawing
	QPainter painter;
	painter.begin( &img );
	painter.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing );

	// draw some stuff with Qt
	QPalette palette;
	QFont font;
	QString s;
	font.setBold( true );
	font.setPointSize( 30 * *height / 1080 );
	painter.setPen( QColor("black") );
	painter.drawLine( 0, *height/2 + 1, *width, *height/2 );
	painter.setPen( QColor("white") );
	painter.drawLine( 0, *height/2 - 1, *width, *height/2 );
	painter.setFont( font );
	s.sprintf( "Frame: %05d\nPSNR:   %05.2f (Y) %05.2f (Cb) %05.2f (Cr)\nSSIM:    %5.3f (Y) %5.3f (Cb) %5.3f (Cr)",
			  mlt_frame_get_position( a_frame ), psnr[0], psnr[1], psnr[2],
			  ssim[0], ssim[1], ssim[2] );
	painter.setPen( QColor("black") );
	painter.drawText( 52, *height * 8 / 10 + 2, *width, *height, 0, s );
	painter.setPen( QColor("white") );
	painter.drawText( 50, *height * 8 / 10, *width, *height, 0, s );

	// finish Qt drawing
	painter.end();
	window_size = mlt_image_format_size( *format, *width, *height, NULL );
	uint8_t *dst = (uint8_t *) mlt_pool_alloc( window_size );
	mlt_properties_set_data( MLT_FRAME_PROPERTIES(a_frame), "image", dst, window_size, mlt_pool_release, NULL );
	*image = dst;

	// convert qimage to mlt
	y = *height + 1;
	while ( --y )
	{
		QRgb *src = (QRgb*) img.scanLine( *height - y );
		int x = *width + 1;
		while ( --x )
		{
			*dst++ = qRed( *src );
			*dst++ = qGreen( *src );
			*dst++ = qBlue( *src );
			*dst++ = qAlpha( *src );
			src++;
		}
	}

	return 0;
}

static mlt_frame process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	mlt_frame_push_service( a_frame, transition );
	mlt_frame_push_frame( a_frame, b_frame );
	mlt_frame_push_get_image( a_frame, get_image );

	return a_frame;
}

extern "C" {

mlt_transition transition_vqm_init( mlt_profile profile, mlt_service_type type, const char *id, void *arg )
{
	mlt_transition transition = mlt_transition_new();

	if ( transition )
	{
		mlt_properties properties = MLT_TRANSITION_PROPERTIES( transition );

		if ( !createQApplicationIfNeeded( MLT_TRANSITION_SERVICE(transition) ) )
		{
			mlt_transition_close( transition );
			return NULL;
		}
		transition->process = process;
		mlt_properties_set_int( properties, "_transition_type", 1 ); // video only
		mlt_properties_set_int( properties, "window_size", 8 );
		printf( "frame psnr[Y] psnr[Cb] psnr[Cr] ssim[Y] ssim[Cb] ssim[Cr]\n" );
	}

	return transition;
}

} // extern "C"
