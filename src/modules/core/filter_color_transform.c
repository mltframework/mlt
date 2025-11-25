/*
 * filter_color_transform.c -- color transform filter
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

#include <framework/mlt_cache.h>
#include <framework/mlt_factory.h>
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_profile.h>

#include <string.h>

static const char *av_trc_str(mlt_color_trc trc)
{
    switch (trc) {
    case mlt_color_trc_bt709:
        return "bt709";
    case mlt_color_trc_gamma22:
        return "bt470m";
    case mlt_color_trc_gamma28:
        return "bt470bg";
    case mlt_color_trc_smpte170m:
        return "smpte170m";
    case mlt_color_trc_smpte240m:
        return "smpte240m";
    case mlt_color_trc_linear:
        return "linear";
    case mlt_color_trc_log:
        return "log100";
    case mlt_color_trc_log_sqrt:
        return "log316";
    case mlt_color_trc_iec61966_2_4:
        return "iec61966-2-4";
    case mlt_color_trc_iec61966_2_1:
        return "iec61966-2-1";
    case mlt_color_trc_bt2020_10:
        return "bt2020_10";
    case mlt_color_trc_bt2020_12:
        return "bt2020_12";
    case mlt_color_trc_smpte2084:
        return "smpte2084";
    case mlt_color_trc_arib_std_b67:
        return "arib-std-b67";
    case mlt_color_trc_smpte428:
        return "smpte428";
    case mlt_color_trc_bt1361_ecg:
        return "bt1361e";
    case mlt_color_trc_none:
    case mlt_color_trc_unspecified:
    case mlt_color_trc_reserved:
    case mlt_color_trc_invalid:
        break;
    }
    return "unknown";
}

static void create_t_filter(mlt_filter self, mlt_frame frame, mlt_color_trc out_trc)
{
    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(self));
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(self);
    const char *method = mlt_properties_get(filter_properties, "method");
    mlt_filter t_filter = mlt_factory_filter(profile, method, NULL);
    if (!t_filter) {
        return;
    }
    mlt_properties cs_properties = MLT_FILTER_PROPERTIES(t_filter);
    if (!strcmp(method, "avfilter.zscale")) {
        mlt_properties_set(cs_properties, "av.t", av_trc_str(out_trc));
    } else if (!strcmp(method, "avfilter.scale")) {
        mlt_properties_set(cs_properties, "av.out_transfer", av_trc_str(out_trc));
    }
    mlt_service_cache_put(MLT_FILTER_SERVICE(self),
                          "t_filter",
                          t_filter,
                          0,
                          (mlt_destructor) mlt_filter_close);
}

static void ensure_color_properties(mlt_filter self, mlt_frame frame)
{
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_image_format format = mlt_properties_get_int(frame_properties, "format");
    int height = mlt_properties_get_int(frame_properties, "height");

    const char *colorspace_str = mlt_properties_get(frame_properties, "colorspace");
    if (!colorspace_str) {
        mlt_colorspace colorspace = mlt_image_default_colorspace(format, height);
        mlt_properties_set_int(frame_properties, "colorspace", colorspace);
        colorspace_str = mlt_properties_get(frame_properties, "colorspace");
    } else {
        mlt_colorspace colorspace = mlt_image_colorspace_id(colorspace_str);
        if (format == mlt_image_rgb || format == mlt_image_rgba || format == mlt_image_rgba64) {
            // Correct the colorspace. For RGB, only support rgb.
            if (colorspace != mlt_colorspace_rgb) {
                mlt_properties_set_int(frame_properties, "colorspace", mlt_colorspace_rgb);
                colorspace_str = mlt_properties_get(frame_properties, "colorspace");
            }
        }
    }
    mlt_colorspace colorspace = mlt_image_colorspace_id(colorspace_str);

    const char *color_trc_str = mlt_properties_get(frame_properties, "color_trc");
    if (!color_trc_str) {
        mlt_color_trc color_trc = mlt_image_default_trc(colorspace);
        mlt_properties_set_int(frame_properties, "color_trc", color_trc);
    } else {
        mlt_color_trc color_trc = mlt_image_color_trc_id(color_trc_str);
        if (format == mlt_image_rgb || format == mlt_image_rgba || format == mlt_image_rgba64) {
            // Correct the TRC. For RGB, only support linear or iec61966-2-1.
            if (color_trc != mlt_color_trc_linear && color_trc != mlt_color_trc_iec61966_2_1) {
                mlt_properties_set_int(frame_properties, "color_trc", mlt_color_trc_iec61966_2_1);
            }
        }
    }

    const char *color_primaries_str = mlt_properties_get(frame_properties, "color_primaries");
    if (!color_primaries_str) {
        mlt_color_primaries primaries = mlt_image_default_primaries(colorspace, height);
        mlt_properties_set_int(frame_properties, "color_primaries", primaries);
    }

    const char *full_range_str = mlt_properties_get(frame_properties, "full_range");
    if (!full_range_str) {
        int full_range = 0;
        switch (format) {
        case mlt_image_rgb:
        case mlt_image_rgba:
        case mlt_image_rgba64:
        case mlt_image_movit:
        case mlt_image_opengl_texture:
            full_range = 1;
            break;
        case mlt_image_yuv422:
        case mlt_image_yuv420p:
        case mlt_image_yuv422p16:
        case mlt_image_yuv420p10:
        case mlt_image_yuv444p10:
        case mlt_image_none:
        case mlt_image_invalid:
            break;
        }
        mlt_properties_set_int(frame_properties, "full_range", full_range);
    } else {
        int full_range = mlt_properties_get_int(frame_properties, "full_range");
        if (format == mlt_image_rgb || format == mlt_image_rgba || format == mlt_image_rgba64) {
            // Correct full range if it is set incorrectly for RGB
            if (!full_range) {
                mlt_properties_set_int(frame_properties, "full_range", 1);
            }
        }
    }
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter self = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(self);
    mlt_image_format requested_format = *format;
    int ret = mlt_frame_get_image(frame, image, format, width, height, writable);
    if (ret || requested_format == mlt_image_movit || requested_format == mlt_image_none)
        return ret;

    const char *out_trc_str = mlt_properties_get(filter_properties, "force_trc");
    if (!out_trc_str)
        out_trc_str = mlt_properties_get(frame_properties, "consumer.mlt_color_trc");
    if (!out_trc_str)
        return 0;

    ensure_color_properties(self, frame);

    const char *frame_trc_str = mlt_properties_get(frame_properties, "color_trc");
    mlt_color_trc frame_trc = mlt_image_color_trc_id(frame_trc_str);
    mlt_color_trc out_trc = mlt_image_color_trc_id(out_trc_str);
    if (*format == mlt_image_rgb || *format == mlt_image_rgba || *format == mlt_image_rgba64) {
        // Correct the TRC. For RGB, only support linear or iec61966-2-1.
        if (out_trc != mlt_color_trc_linear) {
            out_trc = mlt_color_trc_iec61966_2_1;
        }
    }
    if (out_trc == frame_trc) {
        // TRC matches. No conversion required
        return 0;
    }
    if (out_trc == mlt_color_trc_none || frame_trc == mlt_color_trc_none) {
        mlt_log_info(MLT_FILTER_SERVICE(self),
                     "Missing TRC - Frame: %s\tOut: %s\n",
                     mlt_image_color_trc_name(frame_trc),
                     mlt_image_color_trc_name(out_trc));
        return 0;
    }

    mlt_log_debug(MLT_FILTER_SERVICE(self),
                  "color trc: %s -> %s\n",
                  mlt_image_color_trc_name(frame_trc),
                  mlt_image_color_trc_name(out_trc));

    // Create a temporary frame to process the image
    mlt_frame clone_frame = mlt_frame_clone_image(frame, 0);

    mlt_service_lock(MLT_FILTER_SERVICE(self));

    // Retrieve the saved filter
    mlt_cache_item cache_item = mlt_service_cache_get(MLT_FILTER_SERVICE(self), "t_filter");
    if (!cache_item) {
        create_t_filter(self, clone_frame, out_trc);
        cache_item = mlt_service_cache_get(MLT_FILTER_SERVICE(self), "t_filter");
    }
    if (!cache_item) {
        mlt_log_error(MLT_FILTER_SERVICE(self), "Unable to create colorspace filter\n");
        return 1;
    }
    mlt_filter t_filter = mlt_cache_item_data(cache_item, NULL);
    // Process the cloned frame. The cloned frame references the same image
    // as the original frame.
    mlt_filter_process(t_filter, clone_frame);
    ret = mlt_frame_get_image(clone_frame, image, format, width, height, 0);
    mlt_cache_item_close(cache_item);
    mlt_service_unlock(MLT_FILTER_SERVICE(self));

    mlt_properties_pass_list(frame_properties,
                             MLT_FRAME_PROPERTIES(clone_frame),
                             "colorspace color_trc color_primaries");
    mlt_frame_close(clone_frame);
    return ret;
}

static mlt_frame filter_process(mlt_filter self, mlt_frame frame)
{
    mlt_frame_push_service(frame, self);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

static void filter_close(mlt_filter self)
{
    mlt_service_cache_purge(MLT_FILTER_SERVICE(self));
    self->child = NULL;
    self->close = NULL;
    self->parent.close = NULL;
    mlt_service_close(&self->parent);
}

mlt_filter filter_color_transform_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg)
{
    const char *method = arg ? arg : "auto";

    // Test if the embedded filter is available.
    mlt_filter test_filter = NULL;
    if (!strcmp(method, "auto")) {
        if ((test_filter = mlt_factory_filter(profile, "avfilter.zscale", NULL))) {
            method = "avfilter.zscale";
        } else if ((test_filter = mlt_factory_filter(profile, "avfilter.scale", NULL))) {
            method = "avfilter.scale";
        }
    } else if (!strcmp(method, "avfilter.zscale")) {
        test_filter = mlt_factory_filter(profile, "avfilter.zscale", NULL);
    } else if (!strcmp(method, "avfilter.scale")) {
        test_filter = mlt_factory_filter(profile, "avfilter.scale", NULL);
    }
    if (!test_filter) {
        mlt_log_error(NULL, "[filter_color_transform] unable to create filter %s\n", method);
        return NULL;
    }
    mlt_filter_close(test_filter);

    mlt_filter self = mlt_filter_new();
    if (self) {
        self->process = filter_process;
        self->close = filter_close;
        mlt_properties_set(MLT_FILTER_PROPERTIES(self), "method", method);
    }
    return self;
}
