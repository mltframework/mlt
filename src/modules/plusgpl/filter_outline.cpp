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

            // Sample in a true circular pattern to avoid artifacts on angles
            auto radius = desc->thickness;
            auto radiusSquared = radius * radius;
            for (auto dy = -radius; dy <= radius; dy++) {
                for (auto dx = -radius; dx <= radius; dx++) {
                    // Skip the center pixel and only sample within the circular radius
                    auto distanceSquared = dx * dx + dy * dy;
                    if (distanceSquared > 0 && distanceSquared <= radiusSquared) {
                        auto sampleX = std::max(0, std::min(x + dx, maxX));
                        auto sampleY = std::max(0, std::min(y + dy, maxY));
                        oa = desc->alphaF(oa, sampleX, sampleY);
                    }
                }
            }

            auto src = &desc->original[y * stride + 4 * x];
            auto dst = &desc->image.planes[0][y * stride + 4 * x];
            auto a = src[3] / 255.f;
            dst[0] = a * src[0] + (1.f - a) * (oa * desc->color.r);
            dst[1] = a * src[1] + (1.f - a) * (oa * desc->color.g);
            dst[2] = a * src[2] + (1.f - a) * (oa * desc->color.b);
            dst[3] = a * src[3] + (1.f - a) * (oa * desc->color.a);
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

    if (!error) {
        Mlt::Frame fr(frame);
        auto position = filter.get_position(fr);
        auto length = filter.get_length2(fr);
        slice_desc desc{.color = filter.anim_get_color("color", position, length),
                        .thickness = filter.anim_get_int("thickness", position, length),
                        .original = *image};

        mlt_image_set_values(&desc.image, nullptr, *format, *width, *height);
        mlt_image_alloc_data(&desc.image);

        filter.lock(); // Block frame-threading since the slice threads are heavy
        mlt_slices_run_normal(0, sliced_proc, &desc);
        filter.unlock();

        *image = static_cast<uint8_t *>(desc.image.data);
        fr.set_image(*image, mlt_image_calculate_size(&desc.image), mlt_pool_release);
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
