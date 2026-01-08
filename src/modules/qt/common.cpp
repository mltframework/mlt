/*
 * Copyright (C) 2014-2026 Meltytech, LLC
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
#include <QImageReader>
#include <QLocale>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_ANDROID)
#include <X11/Xlib.h>
#include <cstdlib>
#endif

bool createQApplicationIfNeeded(mlt_service service)
{
    if (!qApp) {
#if defined(Q_OS_WIN) && defined(NODEPLOY)
        QCoreApplication::addLibraryPath(QString(mlt_environment("MLT_APPDIR"))
                                         + QStringLiteral("/bin"));
        QCoreApplication::addLibraryPath(QString(mlt_environment("MLT_APPDIR"))
                                         + QStringLiteral("/plugins"));
#endif
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_ANDROID)
        if (getenv("DISPLAY") == 0 && getenv("WAYLAND_DISPLAY") == 0) {
            const char *qt_qpa = getenv("QT_QPA_PLATFORM");
            if (!qt_qpa || strcmp(qt_qpa, "offscreen")) {
                mlt_log_error(
                    service,
                    "The MLT Qt module requires a X11 or Wayland environment.\n"
                    "Please either run melt from a session with a display server or use a "
                    "fake X server like xvfb:\n"
                    "xvfb-run -a melt (...)\n");
                return false;
            }
        }
#endif
        if (!mlt_properties_get(mlt_global_properties(), "qt_argv"))
            mlt_properties_set(mlt_global_properties(), "qt_argv", "MLT");
        static int argc = 1;
        static char *argv[] = {mlt_properties_get(mlt_global_properties(), "qt_argv")};
        new QApplication(argc, argv);
        const char *localename = mlt_properties_get_lcnumeric(MLT_SERVICE_PROPERTIES(service));
        QLocale::setDefault(QLocale(localename));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QImageReader::setAllocationLimit(1024);
#endif
    }
    return true;
}

mlt_image_format choose_image_format(mlt_image_format format)
{
    if (format == mlt_image_rgba64) {
        return mlt_image_rgba64;
    }
    return mlt_image_rgba;
}

void convert_qimage_to_mlt(QImage *qImg, uint8_t *mImg, int width, int height)
{
    // Destination pointer must be the same pointer that was provided to
    // convert_mlt_to_qimage()
    Q_ASSERT(mImg == qImg->constBits());
}

void convert_mlt_to_qimage(
    uint8_t *mImg, QImage *qImg, int width, int height, mlt_image_format format)
{
    if (format == mlt_image_rgba64) {
        *qImg = QImage(mImg, width, height, QImage::Format_RGBA64);
        return;
    }

    Q_ASSERT(format == mlt_image_rgba);

    *qImg = QImage(mImg, width, height, QImage::Format_RGBA8888);
}

int create_image(mlt_frame frame,
                 uint8_t **image,
                 mlt_image_format *image_format,
                 int *width,
                 int *height,
                 int writable)
{
    int error = 0;
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);

    *image_format = mlt_image_rgba;

    // Use the width and height suggested by the rescale filter.
    if (mlt_properties_get_int(frame_properties, "rescale_width") > 0)
        *width = mlt_properties_get_int(frame_properties, "rescale_width");
    if (mlt_properties_get_int(frame_properties, "rescale_height") > 0)
        *height = mlt_properties_get_int(frame_properties, "rescale_height");
    // If no size is requested, use native size.
    if (*width <= 0)
        *width = mlt_properties_get_int(frame_properties, "meta.media.width");
    if (*height <= 0)
        *height = mlt_properties_get_int(frame_properties, "meta.media.height");

    int size = mlt_image_format_size(*image_format, *width, *height, NULL);
    *image = static_cast<uint8_t *>(mlt_pool_alloc(size));
    memset(*image, 0, size); // Transparent
    mlt_frame_set_image(frame, *image, size, mlt_pool_release);

    return error;
}
