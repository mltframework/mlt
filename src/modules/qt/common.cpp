/*
 * Copyright (C) 2014 Dan Dennedy <dan@dennedy.org>
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
#include <QApplication>
#include <QLocale>
#include <QImage>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include <X11/Xlib.h>
#include <cstdlib>
#endif

bool createQApplicationIfNeeded(mlt_service service)
{
	if (!qApp) {
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
		XInitThreads();
		if (getenv("DISPLAY") == 0) {
			mlt_log_error(service,
				"The MLT Qt module requires a X11 environment.\n"
				"Please either run melt from an X session or use a fake X server like xvfb:\n"
				"xvfb-run -a melt (...)\n" );
			return false;
		}
#endif
		if (!mlt_properties_get(mlt_global_properties(), "qt_argv"))
			mlt_properties_set(mlt_global_properties(), "qt_argv", "MLT");
		static int argc = 1;
		static char* argv[] = { mlt_properties_get(mlt_global_properties(), "qt_argv") };
		new QApplication(argc, argv);
		const char *localename = mlt_properties_get_lcnumeric(MLT_SERVICE_PROPERTIES(service));
		QLocale::setDefault(QLocale(localename));
	}
	return true;
}

void convert_qimage_to_mlt_rgba( QImage* qImg, uint8_t* mImg, int width, int height )
{
#if QT_VERSION >= 0x050200
	// QImage::Format_RGBA8888 was added in Qt5.2
	// Nothing to do in this case  because the image was modified directly.
	// Destination pointer must be the same pointer that was provided to
	// convert_mlt_to_qimage_rgba()
	Q_ASSERT(mImg == qImg->constBits());
#else
	int y = height + 1;
	while (--y)
	{
		QRgb* src = (QRgb*)qImg->scanLine(height - y);
		int x = width + 1;
		while (--x)
		{
			*mImg++ = qRed(*src);
			*mImg++ = qGreen(*src);
			*mImg++ = qBlue(*src);
			*mImg++ = qAlpha(*src);
			src++;
		}
	}
#endif
}

void convert_mlt_to_qimage_rgba( uint8_t* mImg, QImage* qImg, int width, int height )
{
#if QT_VERSION >= 0x050200
	// QImage::Format_RGBA8888 was added in Qt5.2
	// Initialize the QImage with the MLT image because the data formats match.
	*qImg = QImage( mImg, width, height, QImage::Format_RGBA8888 );
#else
	*qImg = QImage( width, height, QImage::Format_ARGB32 );
	int y = height + 1;
	while (--y)
	{
		QRgb *dst = (QRgb*)qImg->scanLine(height - y);
		int x = width + 1;
		while (--x)
		{
			*dst++ = qRgba(mImg[0], mImg[1], mImg[2], mImg[3]);
			mImg += 4;
		}
	}
#endif
}

int create_image( mlt_frame frame, uint8_t **image, mlt_image_format *image_format, int *width, int *height, int writable )
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
