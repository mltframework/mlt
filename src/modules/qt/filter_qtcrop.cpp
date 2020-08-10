/*
 * filter_qtcrop.cpp -- cropping filter
 * Copyright (c) 2020 Meltytech, LLC
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
#include "math.h"
#include <mlt++/Mlt.h>
#include <framework/mlt_log.h>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QString>

static int get_image(mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable)
{
	int error = 0;
	mlt_filter filter = (mlt_filter)mlt_frame_pop_service(frame);
	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
	mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
	mlt_position position = mlt_filter_get_position(filter, frame);
	mlt_position length = mlt_filter_get_length2(filter, frame);
	mlt_rect rect = mlt_properties_anim_get_rect(properties, "rect", position, length);

	// Get the current image
	*format = mlt_image_rgb24a;
	mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "resize_alpha", 255);
	error = mlt_frame_get_image(frame, image, format, width, height, writable);

	if (!error && *format == mlt_image_rgb24a) {
		QImage bgImage(*width, *height, QImage::Format_ARGB32);
		convert_mlt_to_qimage_rgba(*image, &bgImage, *width, *height);

		QImage fgImage = bgImage.copy();
		QPainter painter(&bgImage);
		QPainterPath path;
		mlt_color color = mlt_properties_get_color(properties, "color");
		double radius = mlt_properties_anim_get_double(properties, "radius", position, length);

		painter.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing);
		bgImage.fill(QColor(color.r, color.g, color.b, color.a));
		if (mlt_properties_get_int(properties, "circle")) {
			QPointF center(double(*width) / 2.0, double(*height) / 2.0);
			double hypotenuse = sqrt(pow(*width, 2) + pow(*height, 2));
			radius *= 0.5 * hypotenuse;
			path.addEllipse(center, radius, radius);
		} else {
			const char* s = mlt_properties_get(properties, "rect");
			if (qstrlen(s) > 0 && strchr(s, '%')) {
				rect.x *= *width;
				rect.w *= *width;
				rect.y *= *height;
				rect.h *= *height;
			} else {
				double scale = mlt_profile_scale_width(profile, *width);
				double scale_height = mlt_profile_scale_height(profile, *height);
				rect.x *= scale;
				rect.y *= scale_height;
				rect.w *= scale;
				rect.h *= scale_height;
			}
			radius *= 0.5 * MIN(*width, *height);
			path.addRoundedRect(rect.x, rect.y, rect.w, rect.h, radius, radius);
		}
		painter.setClipPath(path);
		painter.drawImage(QPointF(0, 0), fgImage);
		painter.end();

		convert_qimage_to_mlt_rgba(&bgImage, *image, *width, *height);
	}
	return error;
}

static mlt_frame process(mlt_filter filter, mlt_frame frame)
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, get_image );

	return frame;
}

extern "C" {

mlt_filter filter_qtcrop_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
	Q_UNUSED(profile)
	Q_UNUSED(type)
	Q_UNUSED(id)
	mlt_filter filter = mlt_filter_new();

	if (!filter || !createQApplicationIfNeeded(MLT_FILTER_SERVICE(filter)))  {
		mlt_filter_close(filter);
		return NULL;
	}
	filter->process = process;

	mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
	mlt_properties_set_string(properties, "rect", arg? arg : "0%/0%:100%x100%" );
	mlt_properties_set_int(properties, "circle", 0);
	mlt_properties_set_string(properties, "color", "#00000000");
	mlt_properties_set_double(properties, "radius", 0.0);

	return filter;
}

}
