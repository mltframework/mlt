/*
 * filter_lines.c -- lines filter
 * Copyright (c) 2007 Marco Gittler <g.marco@freenet.de>
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
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_profile.h>
#include <framework/mlt_slices.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    uint8_t *image;
    int width;
    int height;
    int dx;
    int ystart;
    int yend;
    int xmid;
    int type;
    double maxdarker;
    double maxlighter;
    int min;
    int max_luma;
    int max_chroma;
} slice_desc;

static int slice_proc(int id, int index, int jobs, void *data)
{
    (void) id; // unused
    slice_desc *d = (slice_desc *) data;
    int slice_line_start,
        slice_height = mlt_slices_size_slice(jobs, index, d->height, &slice_line_start);
    uint8_t *p = d->image;

    for (int y = MAX(d->ystart, slice_line_start);
         y < MIN(d->yend, slice_line_start + slice_height);
         y++) {
        for (int x = -(d->dx); x < d->dx && (x + d->xmid) < d->width; x++) {
            if (x + d->xmid > 0) {
                int i = (y * d->width + x + d->xmid) * 2;
                double diff = 1.0 - (double) abs(x) / d->dx;
                switch (d->type) {
                case 1: //blackline
                    p[i] = CLAMP(p[i] - (double) p[i] * diff * d->maxdarker / 100.0,
                                 d->min,
                                 d->max_luma);
                    break;
                case 2: //whiteline
                    p[i] = CLAMP(p[i] + (255.0 - (double) p[i]) * diff * d->maxlighter / 100.0,
                                 d->min,
                                 d->max_luma);
                    break;
                case 3: //greenline
                    p[i + 1] = CLAMP(p[i + 1] - (double) p[i + 1] * diff * d->maxlighter / 100.0,
                                     d->min,
                                     d->max_chroma);
                    break;
                }
            }
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
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FILTER_PROPERTIES(filter);
    mlt_position pos = mlt_filter_get_position(filter, frame);
    mlt_position len = mlt_filter_get_length2(filter, frame);

    *format = mlt_image_yuv422;
    int error = mlt_frame_get_image(frame, image, format, width, height, 1);

    if (error == 0 && *image) {
        int line_width = mlt_properties_anim_get_int(properties, "line_width", pos, len);
        int num = mlt_properties_anim_get_int(properties, "num", pos, len);
        double maxdarker = (double) mlt_properties_anim_get_int(properties, "darker", pos, len);
        double maxlighter = (double) mlt_properties_anim_get_int(properties, "lighter", pos, len);
        int full_range = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "full_range");
        int min = full_range ? 0 : 16;
        int max_luma = full_range ? 255 : 235;
        int max_chroma = full_range ? 255 : 240;

        char buf[256];
        char typebuf[256];

        mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
        double scale = mlt_profile_scale_width(profile, *width);
        if (line_width > 1 && scale > 0.0)
            line_width = MAX(2, lrint(line_width * scale));

        if (line_width < 1)
            return 0;

        double position = mlt_filter_get_progress(filter, frame);
        oldfilm_rand_seed seed;

        mlt_service_lock(MLT_FILTER_SERVICE(filter));

        while (num--) {
            oldfilm_init_seed(&seed, position * 10000 + num);
            int type = (oldfilm_fast_rand(&seed) % 3) + 1;
            int xmid = (double) (*width) * oldfilm_fast_rand(&seed) / INT_MAX;
            int dx = oldfilm_fast_rand(&seed) % line_width;
            int ystart = oldfilm_fast_rand(&seed) % (*height);
            int yend = oldfilm_fast_rand(&seed) % (*height);

            sprintf(buf, "line%d", num);
            sprintf(typebuf, "typeline%d", num);
            maxlighter += oldfilm_fast_rand(&seed) % 30 - 15;
            maxdarker += oldfilm_fast_rand(&seed) % 30 - 15;

            if (!mlt_properties_get_int(properties, buf)) {
                mlt_properties_set_int(properties, buf, xmid);
            }

            if (!mlt_properties_get_int(properties, typebuf)) {
                mlt_properties_set_int(properties, typebuf, type);
            }

            xmid = mlt_properties_get_int(properties, buf);
            type = mlt_properties_get_int(properties, typebuf);
            if (position != mlt_properties_get_double(properties, "last_oldfilm_line_pos")) {
                xmid += (oldfilm_fast_rand(&seed) % 11 - 5);
            }

            if (yend < ystart) {
                yend = *height;
            }
            if (dx) {
                slice_desc desc = {.image = *image,
                                   .width = *width,
                                   .height = *height,
                                   .dx = dx,
                                   .ystart = ystart,
                                   .yend = yend,
                                   .xmid = xmid,
                                   .type = type,
                                   .maxdarker = maxdarker,
                                   .maxlighter = maxlighter,
                                   .min = min,
                                   .max_luma = max_luma,
                                   .max_chroma = max_chroma};
                mlt_slices_run_normal(0, slice_proc, &desc);
            }
            mlt_properties_set_int(properties, buf, xmid);
        }
        mlt_properties_set_double(properties, "last_oldfilm_line_pos", position);
        mlt_service_unlock(MLT_FILTER_SERVICE(filter));
    }

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

mlt_filter filter_lines_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter != NULL) {
        filter->process = filter_process;
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "line_width", 2);
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "num", 5);
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "darker", 40);
        mlt_properties_set_int(MLT_FILTER_PROPERTIES(filter), "lighter", 40);
    }
    return filter;
}
