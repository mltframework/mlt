/*
 * filter_colorspace.c -- colorspace filter
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

#include <stdio.h>
#include <stdlib.h>
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

static const char *av_colorspace_str(mlt_colorspace colorspace, int height)
{
    switch (colorspace) {
    case mlt_colorspace_rgb:
        return "gbr";
    case mlt_colorspace_bt709:
        return "bt709";
    case mlt_colorspace_fcc:
        return "fcc";
    case mlt_colorspace_bt470bg:
        return "bt470bg";
    case mlt_colorspace_smpte170m:
        return "smpte170m";
    case mlt_colorspace_smpte240m:
        return "smpte240m";
    case mlt_colorspace_ycgco:
        return "ycgco";
    case mlt_colorspace_bt2020_ncl:
        return "bt2020nc";
    case mlt_colorspace_bt2020_cl:
        return "bt2020c";
    case mlt_colorspace_smpte2085:
        return "smpte2085";
    case mlt_colorspace_bt601:
        return height < 576 ? "smpte170m" : "bt470bg";
    case mlt_colorspace_invalid:
    case mlt_colorspace_unspecified:
    case mlt_colorspace_reserved:
        break;
    }
    // Guess a reasonable default
    if (height < 576) {
        return "smpte170m";
    } else if (height < 720) {
        return "bt470bg";
    }
    return "bt709";
}

static const char *av_color_primaries_str(mlt_color_primaries primaries, int height)
{
    switch (primaries) {
    case mlt_color_pri_bt709:
        return "bt709";
    case mlt_color_pri_bt470m:
        return "bt470m";
    case mlt_color_pri_bt470bg:
        return "bt470bg";
    case mlt_color_pri_smpte170m:
        return "smpte170m";
    case mlt_color_pri_film:
        return "film";
    case mlt_color_pri_bt2020:
        return "bt2020";
    case mlt_color_pri_smpte428:
        return "smpte428";
    case mlt_color_pri_smpte431:
        return "smpte431";
    case mlt_color_pri_smpte432:
        return "smpte432";
    case mlt_color_pri_none:
    case mlt_color_pri_invalid:
        break;
    }
    // Guess a reasonable default
    if (height < 576) {
        return "smpte170m";
    } else if (height < 720) {
        return "bt470bg";
    }
    return "bt709";
}

static void create_cs_filter(mlt_filter filter, mlt_frame frame, mlt_color_trc out_trc)
{
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    const char *method = mlt_properties_get(filter_properties, "method");
    mlt_filter cs_filter = mlt_factory_filter(profile, method, NULL);
    if (!cs_filter) {
        return;
    }
    mlt_properties cs_properties = MLT_FILTER_PROPERTIES(cs_filter);
    if (!strcmp(method, "avfilter.zscale")) {
        mlt_properties_set(cs_properties, "av.t", av_trc_str(out_trc));
    } else if (!strcmp(method, "avfilter.colorspace")) {
        const char *colorspace_str = mlt_properties_get(frame_properties, "consumer.colorspace");
        if (!colorspace_str)
            colorspace_str = mlt_properties_get(frame_properties, "colorspace");
        mlt_colorspace colorspace = mlt_image_colorspace_id(colorspace_str);
        int height = mlt_properties_get_int(frame_properties, "height");
        const char *primaries_str = mlt_properties_get(frame_properties, "consumer.color_primaries");
        mlt_color_primaries primaries = mlt_image_color_pri_id(primaries_str);
        mlt_properties_set(cs_properties, "av.space", av_colorspace_str(colorspace, height));
        mlt_properties_set(cs_properties, "av.primaries", av_color_primaries_str(primaries, height));
        mlt_properties_set(cs_properties, "av.trc", av_trc_str(out_trc));
    }
    mlt_service_cache_put(MLT_FILTER_SERVICE(filter),
                          "cs_filter",
                          cs_filter,
                          0,
                          (mlt_destructor) mlt_filter_close);
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *format,
                            int *width,
                            int *height,
                            int writable)
{
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    mlt_image_format requested_format = *format;
    int ret = mlt_frame_get_image(frame, image, format, width, height, writable);
    if (ret || requested_format == mlt_image_movit)
        return ret;

    const char *out_trc_str = mlt_properties_get(filter_properties, "force_trc");
    if (!out_trc_str)
        out_trc_str = mlt_properties_get(frame_properties, "consumer.mlt_color_trc");
    if (!out_trc_str)
        return 0;

    const char *frame_trc_str = mlt_properties_get(frame_properties, "color_trc");
    mlt_color_trc out_trc = mlt_image_color_trc_id(out_trc_str);
    mlt_color_trc frame_trc = mlt_image_color_trc_id(frame_trc_str);
    if (out_trc == frame_trc) {
        // TRC matches. No conversion required
        return 0;
    }
    if (out_trc == mlt_color_trc_none || frame_trc == mlt_color_trc_none) {
        mlt_log_info(MLT_FILTER_SERVICE(filter),
                     "Missing TRC - Frame: %s\tOut: %s\n",
                     mlt_image_color_trc_name(frame_trc),
                     mlt_image_color_trc_name(out_trc));
        return 0;
    }

    mlt_log_verbose(MLT_FILTER_SERVICE(filter),
                    "color trc: %s -> %s\n",
                    mlt_image_color_trc_name(frame_trc),
                    mlt_image_color_trc_name(out_trc));

    mlt_cache_item cache_item = mlt_service_cache_get(MLT_FILTER_SERVICE(filter), "cs_filter");
    if (!cache_item) {
        create_cs_filter(filter, frame, out_trc);
        cache_item = mlt_service_cache_get(MLT_FILTER_SERVICE(filter), "cs_filter");
    }
    if (!cache_item) {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Unable to create colorspace filter\n");
        return 1;
    }
    mlt_filter cs_filter = mlt_cache_item_data(cache_item, NULL);
    mlt_filter_process(cs_filter, frame);
    mlt_cache_item_close(cache_item);
    return mlt_frame_get_image(frame, image, format, width, height, writable);
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);
    return frame;
}

static void filter_close(mlt_filter filter)
{
    mlt_service_cache_purge(MLT_FILTER_SERVICE(filter));
    filter->child = NULL;
    filter->close = NULL;
    filter->parent.close = NULL;
    mlt_service_close(&filter->parent);
}

mlt_filter filter_colorspace_init(mlt_profile profile,
                                  mlt_service_type type,
                                  const char *id,
                                  char *arg)
{
    const char *method = arg;
    if (!method)
        method = "auto";

    // Test if the embedded filter is available.
    mlt_filter test_filter = NULL;
    if (!strcmp(method, "auto")) {
        if ((test_filter = mlt_factory_filter(profile, "avfilter.zscale", NULL))) {
            method = "avfilter.zscale";
        } else if ((test_filter = mlt_factory_filter(profile, "avfilter.colorspace", NULL))) {
            method = "avfilter.colorspace";
        }
    } else if (!strcmp(method, "avfilter.zscale")) {
        test_filter = mlt_factory_filter(profile, "avfilter.zscale", NULL);
    } else if (!strcmp(method, "avfilter.colorspace")) {
        test_filter = mlt_factory_filter(profile, "avfilter.colorspace", NULL);
    }
    if (!test_filter) {
        mlt_log_error(NULL, "[filter_colorspace] unable to create filter %s\n", method);
        return NULL;
    }
    mlt_filter_close(test_filter);

    mlt_filter filter = mlt_filter_new();
    if (filter) {
        filter->process = filter_process;
        filter->close = filter_close;
        mlt_properties_set(MLT_FILTER_PROPERTIES(filter), "method", method);
    }
    return filter;
}
