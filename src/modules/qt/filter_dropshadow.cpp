/*
 * filter_dropshadow.cpp -- Drop Shadow Effect
 * Copyright (c) 2024-2025 Meltytech, LLC
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
#include <MltFilter.h>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>

static int get_image(mlt_frame frame,
                     uint8_t **image,
                     mlt_image_format *image_format,
                     int *width,
                     int *height,
                     int writable)
{
    auto error = 0;
    auto filter = Mlt::Filter(mlt_filter(mlt_frame_pop_service(frame)));

    *image_format = choose_image_format(*image_format);
    error = mlt_frame_get_image(frame, image, image_format, width, height, writable);

    if (!error) {
        QImage qimg;
        convert_mlt_to_qimage(*image, &qimg, *width, *height, *image_format);

        auto shadow = new QGraphicsDropShadowEffect;
        auto f = Mlt::Frame(frame);
        mlt_position pos = filter.get_position(f);
        mlt_position len = filter.get_length2(f);
        auto color = filter.anim_get_color("color", pos, len);
        shadow->setColor(QColor(color.r, color.g, color.b, color.a));
        shadow->setBlurRadius(filter.anim_get_double("radius", pos, len));
        shadow->setXOffset(filter.anim_get_double("x", pos, len));
        shadow->setYOffset(filter.anim_get_double("y", pos, len));

        QGraphicsScene scene;
        QGraphicsPixmapItem item;
        scene.setItemIndexMethod(QGraphicsScene::NoIndex);
        item.setPixmap(QPixmap::fromImage(qimg));
        item.setGraphicsEffect(shadow);
        scene.addItem(&item);

        QPainter painter(&qimg);
        scene.render(&painter);
        painter.end();
        convert_qimage_to_mlt(&qimg, *image, *width, *height);
    }

    return error;
}

static mlt_frame process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, get_image);

    return frame;
}

extern "C" {

mlt_filter filter_dropshadow_init(mlt_profile profile,
                                  mlt_service_type type,
                                  const char *id,
                                  char *arg)
{
    mlt_filter filter = mlt_filter_new();

    if (!filter)
        return nullptr;

    if (!createQApplicationIfNeeded(MLT_FILTER_SERVICE(filter))) {
        mlt_filter_close(filter);
        return nullptr;
    }

    filter->process = process;

    auto properties = Mlt::Properties(filter);
    properties.set("color", "#b4636363");
    properties.set("radius", 1.0);
    properties.set("x", 8.0);
    properties.set("y", 8.0);

    return filter;
}

} // extern "C"
