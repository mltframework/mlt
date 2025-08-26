/*
 * filter_swscale.c -- image scaling filter
 * Copyright (C) 2008-2025 Meltytech, LLC
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
#include <framework/mlt_factory.h>
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_slices.h>

// ffmpeg Header files
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_THREADS (6)

static inline int convert_mlt_to_av_cs(mlt_image_format format)
{
    int value = 0;

    switch (format) {
    case mlt_image_rgb:
        value = AV_PIX_FMT_RGB24;
        break;
    case mlt_image_rgba:
        value = AV_PIX_FMT_RGBA;
        break;
    case mlt_image_yuv422:
        value = AV_PIX_FMT_YUYV422;
        break;
    case mlt_image_yuv420p:
        value = AV_PIX_FMT_YUV420P;
        break;
    case mlt_image_yuv420p10:
        value = AV_PIX_FMT_YUV420P10LE;
        break;
    case mlt_image_yuv444p10:
        value = AV_PIX_FMT_YUV444P10LE;
        break;
    case mlt_image_yuv422p16:
        value = AV_PIX_FMT_YUV422P16LE;
        break;
    case mlt_image_rgba64:
        value = AV_PIX_FMT_RGBA64LE;
        break;
    default:
        mlt_log_error(NULL, "[filter swscale] Invalid format %s\n", mlt_image_format_name(format));
        break;
    }

    return value;
}

static int filter_scale(mlt_frame frame,
                        uint8_t **image,
                        mlt_image_format *format,
                        int iwidth,
                        int iheight,
                        int owidth,
                        int oheight)
{
    int result = 0;

    // Get the properties
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);

    // Get the requested interpolation method
    char *interps = mlt_properties_get(properties, "consumer.rescale");

    // Convert to the SwScale flag
    int interp = SWS_BILINEAR;
    if (strcmp(interps, "nearest") == 0 || strcmp(interps, "neighbor") == 0)
        interp = SWS_POINT;
    else if (strcmp(interps, "tiles") == 0 || strcmp(interps, "fast_bilinear") == 0)
        interp = SWS_FAST_BILINEAR;
    else if (strcmp(interps, "bilinear") == 0)
        interp = SWS_BILINEAR;
    else if (strcmp(interps, "bicubic") == 0)
        interp = SWS_BICUBIC;
    else if (strcmp(interps, "bicublin") == 0)
        interp = SWS_BICUBLIN;
    else if (strcmp(interps, "gauss") == 0)
        interp = SWS_GAUSS;
    else if (strcmp(interps, "sinc") == 0)
        interp = SWS_SINC;
    else if (strcmp(interps, "hyper") == 0 || strcmp(interps, "lanczos") == 0)
        interp = SWS_LANCZOS;
    else if (strcmp(interps, "spline") == 0)
        interp = SWS_SPLINE;

    // Set swscale flags to get good quality
    interp |= SWS_FULL_CHR_H_INP | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND;

    // Convert the pixel formats
    int avformat = convert_mlt_to_av_cs(*format);

    // Determine the output image size.
    int out_size = mlt_image_format_size(*format, owidth, oheight, NULL);
    uint8_t *outbuf = mlt_pool_alloc(out_size);

    // Create the context
    struct SwsContext *context = sws_alloc_context();
    if (outbuf && context) {
        AVFrame *avinframe = av_frame_alloc();
        AVFrame *avoutframe = av_frame_alloc();

        av_opt_set_int(context, "srcw", iwidth, 0);
        av_opt_set_int(context, "srch", iheight, 0);
        av_opt_set_int(context, "src_format", avformat, 0);
        av_opt_set_int(context, "dstw", owidth, 0);
        av_opt_set_int(context, "dsth", oheight, 0);
        av_opt_set_int(context, "dst_format", avformat, 0);
        av_opt_set_int(context, "sws_flags", interp, 0);
#if LIBSWSCALE_VERSION_MAJOR >= 6
        av_opt_set_int(context, "threads", MIN(mlt_slices_count_normal(), MAX_THREADS), 0);
#endif
        result = sws_init_context(context, NULL, NULL);
        if (result < 0) {
            mlt_log_error(NULL,
                          "[filter swscale] Initializing swscale failed with %d (%s)\n",
                          result,
                          av_err2str(result));
            result = 1;
            goto exit;
        }

        mlt_profile profile = mlt_service_profile(
            MLT_PRODUCER_SERVICE(mlt_frame_get_original_producer(frame)));
        int dst_colorspace = profile ? profile->colorspace : 601;
        int src_colorspace = mlt_properties_get_int(properties, "colorspace");
        int src_full_range = mlt_properties_get_int(properties, "full_range");
        const char *dst_color_range = mlt_properties_get(properties, "consumer.color_range");
        int dst_full_range = mlt_image_full_range(dst_color_range);

        result = mlt_set_luma_transfer(context,
                                       src_colorspace,
                                       dst_colorspace,
                                       src_full_range,
                                       dst_full_range);
        if (result < 0) {
            mlt_log_error(NULL,
                          "[filter swscale] Setting swscale color options failed with %d (%s)\n",
                          result,
                          av_err2str(result));
            result = 1;
            goto exit;
        }

        // Setup the input image
        avinframe->width = iwidth;
        avinframe->height = iheight;
        avinframe->format = avformat;
        avinframe->sample_aspect_ratio = av_d2q(mlt_frame_get_aspect_ratio(frame), 1024);
        if (!mlt_properties_get_int(properties, "progressive"))
            avinframe->flags |= AV_FRAME_FLAG_INTERLACED;
        if (mlt_properties_get_int(properties, "top_field_first"))
            avinframe->flags |= AV_FRAME_FLAG_TOP_FIELD_FIRST;
        av_image_fill_arrays(avinframe->data,
                             avinframe->linesize,
                             *image,
                             avinframe->format,
                             iwidth,
                             iheight,
                             1);

        // Setup the output image
        av_frame_copy_props(avoutframe, avinframe);
        avoutframe->width = owidth;
        avoutframe->height = oheight;
        avoutframe->format = avformat;

        result = av_frame_get_buffer(avoutframe, 0);
        if (result < 0) {
            mlt_log_error(NULL, "[filter swscale] Cannot allocate output frame buffer\n");
            result = 1;
            goto exit;
        }

        // Perform the scaling
#if LIBSWSCALE_VERSION_MAJOR >= 6
        result = sws_scale_frame(context, avoutframe, avinframe);
#else
        result = sws_scale(context,
                           (const uint8_t **) avinframe->data,
                           avinframe->linesize,
                           0,
                           iheight,
                           avoutframe->data,
                           avoutframe->linesize);
#endif
        if (result < 0) {
            mlt_log_error(NULL,
                          "[filter swscale] sws_scale_frame failed with %d (%s)\n",
                          result,
                          av_err2str(result));
            result = 1;
            goto exit;
        }
        sws_freeContext(context);
        context = NULL;

        // Sanity check the output frame
        if (owidth != avoutframe->width || oheight != avoutframe->height) {
            mlt_log_error(NULL, "[filter swscale] Unexpected output size\n");
            result = 1;
            goto exit;
        }

        // Copy the filter output into the output buffer
        if (*format == mlt_image_yuv420p || *format == mlt_image_yuv420p10
            || *format == mlt_image_yuv444p10 || *format == mlt_image_yuv422p16) {
            int i = 0;
            int p = 0;
            int strides[4];
            uint8_t *planes[4];
            int heights[3] = {oheight, oheight, oheight};
            if (*format == mlt_image_yuv420p || *format == mlt_image_yuv420p10) {
                heights[1] >>= 1;
                heights[2] >>= 1;
            }
            mlt_image_format_planes(*format, owidth, oheight, outbuf, planes, strides);
            for (p = 0; p < 3; p++) {
                uint8_t *src = avoutframe->data[p];
                for (i = 0; i < heights[p]; i++) {
                    memcpy(&planes[p][i * strides[p]],
                           &src[i * avoutframe->linesize[p]],
                           strides[p]);
                }
            }
        } else {
            int i;
            uint8_t *dst = outbuf;
            uint8_t *src = avoutframe->data[0];
            int stride = mlt_image_format_size(*format, owidth, 1, NULL);
            for (i = 0; i < oheight; i++) {
                memcpy(dst, src, stride);
                dst += stride;
                src += avoutframe->linesize[0];
            }
        }

        // Now update the MLT frame
        mlt_frame_set_image(frame, outbuf, out_size, mlt_pool_release);
        mlt_properties_set_int(properties, "full_range", dst_full_range);

        // Return the output image
        *image = outbuf;

        // Scale the alpha channel only if exists and not correct size
        int alpha_size = 0;
        uint8_t *alpha = mlt_frame_get_alpha_size(frame, &alpha_size);
        if (alpha && alpha_size > 0 && alpha_size != (owidth * oheight)) {
            // Create the context and output image
            outbuf = mlt_pool_alloc(owidth * oheight);
            context = sws_alloc_context();

            if (outbuf && context) {
                av_frame_unref(avinframe);
                av_frame_unref(avoutframe);

                avformat = AV_PIX_FMT_GRAY8;
                av_opt_set_int(context, "srcw", iwidth, 0);
                av_opt_set_int(context, "srch", iheight, 0);
                av_opt_set_int(context, "src_format", avformat, 0);
                av_opt_set_int(context, "dstw", owidth, 0);
                av_opt_set_int(context, "dsth", oheight, 0);
                av_opt_set_int(context, "dst_format", avformat, 0);
                av_opt_set_int(context, "sws_flags", interp, 0);
#if LIBSWSCALE_VERSION_MAJOR >= 6
                av_opt_set_int(context, "threads", MIN(mlt_slices_count_normal(), MAX_THREADS), 0);
#endif
                result = sws_init_context(context, NULL, NULL);
                if (result < 0) {
                    mlt_log_error(
                        NULL,
                        "[filter swscale] Initializing swscale alpha failed with %d (%s)\n",
                        result,
                        av_err2str(result));
                    result = 1;
                    goto exit;
                }

                // Setup the input image
                avinframe->width = iwidth;
                avinframe->height = iheight;
                avinframe->format = avformat;
                avinframe->data[0] = alpha;
                avinframe->linesize[0] = iwidth;

                // Setup the output image
                avoutframe->width = owidth;
                avoutframe->height = oheight;
                avoutframe->format = avformat;

                result = av_frame_get_buffer(avoutframe, 0);
                if (result < 0) {
                    mlt_log_error(NULL, "[filter swscale] Cannot allocate alpha frame buffer\n");
                    result = 1;
                    goto exit;
                }

                // Perform the scaling
#if LIBSWSCALE_VERSION_MAJOR >= 6
                result = sws_scale_frame(context, avoutframe, avinframe);
#else
                result = sws_scale(context,
                                   (const uint8_t **) avinframe->data,
                                   avinframe->linesize,
                                   0,
                                   iheight,
                                   avoutframe->data,
                                   avoutframe->linesize);
#endif
                if (result < 0) {
                    mlt_log_error(
                        NULL,
                        "[filter swscale] sws_scale_frame alpha failed with %d (%s) %d %d\n",
                        result,
                        av_err2str(result),
                        avoutframe->width,
                        avoutframe->height);
                    result = 1;
                    goto exit;
                }
                sws_freeContext(context);
                context = NULL;

                // Sanity check the output frame
                if (owidth != avoutframe->width || oheight != avoutframe->height) {
                    mlt_log_error(NULL, "[filter swscale] Unexpected output alpha size\n");
                    result = 1;
                    goto exit;
                }

                int i;
                uint8_t *dst = outbuf;
                uint8_t *src = avoutframe->data[0];
                for (i = 0; i < oheight; i++) {
                    memcpy(dst, src, owidth);
                    dst += owidth;
                    src += avoutframe->linesize[0];
                }

                // Set it back on the frame
                mlt_frame_set_alpha(frame, outbuf, owidth * oheight, mlt_pool_release);
            }
        }

    exit:
        av_frame_free(&avinframe);
        av_frame_free(&avoutframe);
        sws_freeContext(context);
    }
    return result;
}

/** Constructor for the filter.
*/

mlt_filter filter_swscale_init(mlt_profile profile, void *arg)
{
    // Create a new scaler
    mlt_filter filter = mlt_factory_filter(profile, "rescale", NULL);

    // If successful, then initialise it
    if (filter != NULL) {
        // Get the properties
        mlt_properties properties = MLT_FILTER_PROPERTIES(filter);

        // Set the inerpolation
        mlt_properties_set(properties, "interpolation", "bilinear");

        // Set the method
        mlt_properties_set_data(properties, "method", filter_scale, 0, NULL, NULL);
    }

    return filter;
}
