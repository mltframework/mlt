/*
 * filter_sws_colortransform.c -- linear color transfer filter using libswscale
 * Copyright (C) 2025 Meltytech, LLC
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

#include <libswscale/swscale.h>

#if LIBSWSCALE_VERSION_MAJOR >= 9

#include "common.h"
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_pool.h>
#include <framework/mlt_profile.h>
#include <framework/mlt_slices.h>

#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

#include <assert.h>
#include <string.h>

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);

    // Get the image first
    int error = mlt_frame_get_image(frame, image, format, width, height, 0);
    if (error)
        return error;

    // Only process if we have a valid image
    if (!*image || *format == mlt_image_none || *format == mlt_image_movit
        || *format == mlt_image_opengl_texture)
        return 0;

    // Get the current color transfer characteristics
    const char *frame_trc_str = mlt_properties_get(properties, "color_trc");
    if (!frame_trc_str) {
        mlt_log_info(MLT_FILTER_SERVICE(filter), "No color_trc property found\n");
        return 0;
    }
    mlt_color_trc frame_trc = mlt_image_color_trc_id(frame_trc_str);

    // Get the target TRC from the transfer property
    const char *transfer_str = mlt_properties_get(filter_properties, "transfer");
    if (!transfer_str) {
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "No transfer property set\n");
        return 0;
    }
    mlt_color_trc target_trc = mlt_image_color_trc_id(transfer_str);

    // Check if conversion is needed
    if (target_trc == frame_trc) {
        // Already at target TRC
        mlt_log_verbose(MLT_FILTER_SERVICE(filter),
                        "No TRC conversion needed: %s\n",
                        mlt_image_color_trc_name(frame_trc));
        return 0;
    }

    if (target_trc == mlt_color_trc_none || target_trc == mlt_color_trc_invalid
        || target_trc == mlt_color_trc_unspecified || target_trc == mlt_color_trc_reserved) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Invalid transfer: %s\n", transfer_str);
        return 1;
    }

    // Get colorspace information
    const char *colorspace_str = mlt_properties_get(properties, "colorspace");
    if (!colorspace_str) {
        colorspace_str = mlt_properties_get(properties, "consumer.colorspace");
    }
    mlt_colorspace colorspace = mlt_image_colorspace_id(colorspace_str);
    const char *primaries_str = mlt_properties_get(properties, "color_primaries");
    mlt_color_primaries primaries = mlt_image_color_pri_id(primaries_str);
    if (primaries == mlt_color_pri_none || primaries == mlt_color_pri_invalid)
        primaries = mlt_image_default_primaries(colorspace, *height);
    int full_range = mlt_properties_get_int(properties, "full_range");

    mlt_log_debug(MLT_FILTER_SERVICE(filter),
                  "Converting TRC: %s (%d) -> %s (%d) colorspace %s full_range %d format %s\n",
                  mlt_image_color_trc_name(frame_trc),
                  mlt_to_av_color_trc(frame_trc),
                  mlt_image_color_trc_name(target_trc),
                  mlt_to_av_color_trc(target_trc),
                  mlt_image_colorspace_name(colorspace),
                  full_range,
                  mlt_image_format_name(*format));

    // Convert the pixel format
    int avformat = mlt_to_av_image_format(*format);
    if (avformat == AV_PIX_FMT_NONE)
        return 1;

    AVFrame *avinframe = av_frame_alloc();
    AVFrame *avoutframe = av_frame_alloc();
    int result = 0;

    if (!avinframe || !avoutframe) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to allocate AVFrames\n");
        result = 1;
        goto cleanup;
    }

    // Configure swscale context
    SwsContext *context = mlt_properties_get_data(filter_properties, "context", NULL);
    if (!context) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to get SwsScale context\n");
        result = 1;
        goto cleanup;
    }
    context->threads = 0;

    // Setup the input frame
    avinframe->width = *width;
    avinframe->height = *height;
    avinframe->format = avformat;
    avinframe->color_trc = mlt_to_av_color_trc(frame_trc);
    avinframe->colorspace = mlt_to_av_colorspace(colorspace, *height);
    avinframe->color_primaries = mlt_to_av_color_primaries(primaries);
    avinframe->color_range = mlt_to_av_color_range(full_range);

    av_image_fill_arrays(avinframe->data,
                         avinframe->linesize,
                         *image,
                         avinframe->format,
                         *width,
                         *height,
                         1);

    // Setup the output frame
    av_frame_copy_props(avoutframe, avinframe);
    avoutframe->width = *width;
    avoutframe->height = *height;
    avoutframe->format = avformat;
    avoutframe->color_trc = mlt_to_av_color_trc(target_trc);

    if (sws_is_noop(avoutframe, avinframe)) {
        mlt_log_debug(MLT_FILTER_SERVICE(filter), "swscale noop\n");
        goto cleanup;
    }

    result = av_frame_get_buffer(avoutframe, 0);
    if (result < 0) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Cannot allocate output frame buffer\n");
        result = 1;
        goto cleanup;
    }

    // Perform the conversion
    result = sws_scale_frame(context, avoutframe, avinframe);
    if (result < 0) {
        mlt_log_error(MLT_FILTER_SERVICE(filter),
                      "sws_scale_frame failed with %d (%s)\n",
                      result,
                      av_err2str(result));
        result = 1;
        goto cleanup;
    }
    assert(avoutframe->data[0] != avinframe->data[0]);

    // Copy the output to the buffer
    if (*format == mlt_image_yuv420p || *format == mlt_image_yuv420p10
        || *format == mlt_image_yuv444p10 || *format == mlt_image_yuv422p16) {
        int i = 0;
        int p = 0;
        int strides[4];
        uint8_t *planes[4];
        int heights[3] = {*height, *height, *height};
        if (*format == mlt_image_yuv420p || *format == mlt_image_yuv420p10) {
            heights[1] >>= 1;
            heights[2] >>= 1;
        }
        mlt_image_format_planes(*format, *width, *height, *image, planes, strides);
        for (p = 0; p < 3; p++) {
            uint8_t *src = avoutframe->data[p];
            for (i = 0; i < heights[p]; i++) {
                memcpy(&planes[p][i * strides[p]], &src[i * avoutframe->linesize[p]], strides[p]);
            }
        }
    } else {
        int i;
        uint8_t *dst = *image;
        uint8_t *src = avoutframe->data[0];
        int stride = mlt_image_format_size(*format, *width, 1, NULL);
        for (i = 0; i < *height; i++) {
            memcpy(dst, src, stride);
            dst += stride;
            src += avoutframe->linesize[0];
        }
    }

    mlt_properties_set_int(properties, "color_trc", target_trc);
    result = 0;

cleanup:
    av_frame_free(&avinframe);
    av_frame_free(&avoutframe);

    return result;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

static void release_context(SwsContext *context)
{
    sws_free_context(&context);
}

mlt_filter filter_sws_colortransform_init(mlt_profile profile,
                                          mlt_service_type type,
                                          const char *id,
                                          char *arg)
{
    mlt_filter filter = mlt_filter_new();
    if (filter) {
        filter->process = filter_process;
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

        // Create the swscale context
        SwsContext *context = sws_alloc_context();
        if (!context) {
            mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to allocate swscale context\n");
            mlt_filter_close(filter);
            return NULL;
        }
        mlt_properties_set_data(properties,
                                "context",
                                context,
                                sizeof(*context),
                                (mlt_destructor) release_context,
                                NULL);

        // Set default transfer (linear)
        mlt_properties_set(properties, "transfer", arg ? arg : "linear");
    }
    return filter;
}

#endif
