/*
 * filter_outline.cpp
 * Copyright (C) 2025 Meltytech, LLC
 * Based on Kino DV Titler originally written by Alejandro Aguilar Sierra <asierra@servidor.unam.mx>
 * https://sourceforge.net/p/kino/code/HEAD/tree/trunk/kino/src/dvtitler/dvtitler.cc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mlt++/MltFilter.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct slice_desc
{
    mlt_color color;
    int thickness;
    mlt_image_s image;
    uint8_t *original;

    float alphaF(const float currentAlphaF, const int x, const int y)
    {
        return std::max(currentAlphaF, float(original[y * image.strides[0] + 4 * x + 3] / 255.f));
    };
};

static int sliced_proc(int id, int index, int jobs, void *data)
{
    (void) id;
    auto *desc = reinterpret_cast<slice_desc *>(data);
    auto start = 0;
    auto height = mlt_slices_size_slice(jobs, index, desc->image.height, &start);
    auto end = start + height;
    auto stride = desc->image.strides[0];
    auto maxX = desc->image.width - 1;
    auto maxY = desc->image.height - 1;

    for (auto y = start; y < end; y++) {
        for (auto x = 0; x < desc->image.width; x++) {
            auto oa = 0.f;
            for (auto o = 0; o < desc->thickness; o++) {
                oa = desc->alphaF(oa, MAX(x - o, 0), y);
                oa = desc->alphaF(oa, MIN(x + o, maxX), y);
                oa = desc->alphaF(oa, x, MAX(y - o, 0));
                oa = desc->alphaF(oa, x, MIN(y + o, maxY));
            }
            auto p = &desc->image.planes[0][y * stride + 4 * x];
            auto a = p[3] / 255.f;
            p[0] = a * p[0] + (1.f - a) * (oa * desc->color.r);
            p[1] = a * p[1] + (1.f - a) * (oa * desc->color.g);
            p[2] = a * p[2] + (1.f - a) * (oa * desc->color.b);
            p[3] = a * p[3] + (1.f - a) * (oa * desc->color.a);
        }
    }
    return 0;
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    Mlt::Filter filter(reinterpret_cast<mlt_filter>(mlt_frame_pop_service(frame)));

    *format = mlt_image_rgba;
    auto error = mlt_frame_get_image(frame, image, format, width, height, 0);

    if (error == 0) {
        Mlt::Frame fr(frame);
        auto position = filter.get_position(fr);
        auto length = filter.get_length2(fr);
        slice_desc desc{.color = filter.anim_get_color("color", position, length),
                        .thickness = filter.anim_get_int("thickness", position, length),
                        .original = *image};

        mlt_image_set_values(&desc.image, nullptr, *format, *width, *height);
        mlt_image_alloc_data(&desc.image);
        ::memcpy(desc.image.data, *image, mlt_image_calculate_size(&desc.image));
        *image = static_cast<uint8_t *>(desc.image.data);
        mlt_frame_set_image(frame,
                            *image,
                            mlt_image_calculate_size(&desc.image),
                            (mlt_destructor) mlt_pool_release);
        mlt_slices_run_normal(0, sliced_proc, &desc);
    }

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

static void filter_close(mlt_filter filter)
{
    filter->child = nullptr;
    filter->close = nullptr;
    filter->parent.close = nullptr;
    mlt_service_close(&filter->parent);
}

extern "C" {

mlt_filter filter_outline_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter) {
        filter->process = filter_process;
        filter->close = filter_close;
        mlt_properties_set_color(MLT_FILTER_PROPERTIES(filter), "color", {255, 255, 255, 255});
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "thickness", 4);
    }
    return filter;
}
}
