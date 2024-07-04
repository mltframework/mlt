/*
 * link_avfilter.c -- provide various links based on libavfilter
 * Copyright (C) 2023-2024 Meltytech, LLC
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

#if !defined(_XOPEN_SOURCE) || _XOPEN_SOURCE < 700
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include "common.h"

#include <framework/mlt.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>

#define PARAM_PREFIX "av."
#define PARAM_PREFIX_LEN (sizeof(PARAM_PREFIX) - 1)

typedef struct
{
    AVFilter *avfilter;
    AVFilterContext *avbuffsink_ctx;
    AVFilterContext *avbuffsrc_ctx;
    AVFilterContext *avfilter_ctx;
    AVFilterContext *scale_ctx;
    AVFilterContext *pad_ctx;
    AVFilterGraph *avfilter_graph;
    AVFrame *avinframe;
    AVFrame *avoutframe;
    int format;
    int width;
    int height;
    int channels;
    int frequency;
    int reset;
    mlt_position expected_frame;
    mlt_position continuity_frame;
} private_data;

#if LIBAVUTIL_VERSION_INT >= ((56 << 16) + (35 << 8) + 101)
static int animatable_avoption(const AVOption *opt)
{
    return opt && (opt->flags & AV_OPT_FLAG_RUNTIME_PARAM) && opt->type != AV_OPT_TYPE_COLOR;
}
#endif

static void property_changed(mlt_service owner, mlt_link self, mlt_event_data event_data)
{
    const char *name = mlt_event_data_to_string(event_data);
    if (name && strncmp(PARAM_PREFIX, name, PARAM_PREFIX_LEN) == 0) {
        private_data *pdata = (private_data *) self->child;
        if (pdata->avfilter_ctx) {
            mlt_service_lock(MLT_LINK_SERVICE(self));
            const AVOption *opt
                = av_opt_find(pdata->avfilter_ctx->priv, name + PARAM_PREFIX_LEN, 0, 0, 0);
#if LIBAVUTIL_VERSION_INT >= ((56 << 16) + (35 << 8) + 101)
            pdata->reset = opt
                           && !(animatable_avoption(opt)
                                && mlt_properties_is_anim(MLT_LINK_PROPERTIES(self), name));
#else
            pdata->reset = opt && !mlt_properties_is_anim(MLT_LINK_PROPERTIES(self), name);
#endif
            mlt_service_unlock(MLT_LINK_SERVICE(self));
        }
    }
}

static int future_frames_needed(mlt_link self)
{
    if (!self || !self->child) {
        return 0;
    }

    private_data *pdata = (private_data *) self->child;
    int future_frames = 0;

    if (strcmp(pdata->avfilter->name, "adeclick") == 0) {
        int windowms = mlt_properties_get_int(MLT_LINK_PROPERTIES(self), "av.window");
        if (windowms <= 0) {
            // Default value is 100
            windowms = 100;
        }
        double fps = mlt_profile_fps(mlt_service_profile(MLT_LINK_SERVICE(self)));
        // Provide future frames for 1.5x window duration (determined empirically)
        future_frames = ceil(1.5 * fps * windowms / 1000);
    }

    return future_frames;
}

static void set_avfilter_options(mlt_link self, double scale)
{
    private_data *pdata = (private_data *) self->child;
    mlt_properties link_properties = MLT_LINK_PROPERTIES(self);
    int i;
    int count = mlt_properties_count(link_properties);
    mlt_properties scale_map = mlt_properties_get_data(link_properties, "_resolution_scale", NULL);

    for (i = 0; i < count; i++) {
        const char *param_name = mlt_properties_get_name(link_properties, i);
        if (param_name && strncmp(PARAM_PREFIX, param_name, PARAM_PREFIX_LEN) == 0) {
            const AVOption *opt
                = av_opt_find(pdata->avfilter_ctx->priv, param_name + PARAM_PREFIX_LEN, 0, 0, 0);
            const char *value = mlt_properties_get_value(link_properties, i);
#if LIBAVUTIL_VERSION_INT >= ((56 << 16) + (35 << 8) + 101)
            if (opt
                && !(animatable_avoption(opt)
                     && mlt_properties_is_anim(link_properties, param_name)))
#else
            if (opt && !mlt_properties_is_anim(link_properties, param_name))
#endif
            {
                if (scale != 1.0) {
                    double scale2 = mlt_properties_get_double(scale_map, opt->name);
                    if (scale2 != 0.0) {
                        double x = mlt_properties_get_double(link_properties, param_name);
                        x *= scale * scale2;
                        mlt_properties_set_double(link_properties, "_avfilter_temp", x);
                        value = mlt_properties_get(link_properties, "_avfilter_temp");
                    }
                }
                av_opt_set(pdata->avfilter_ctx->priv, opt->name, value, 0);
            }
        }
    }
}

static void send_avformat_commands(mlt_link self, mlt_frame frame, private_data *pdata, double scale)
{
#if LIBAVUTIL_VERSION_INT >= ((56 << 16) + (35 << 8) + 101)
    mlt_properties prop = MLT_LINK_PROPERTIES(self);
    mlt_position position = mlt_producer_position(MLT_LINK_PRODUCER(self))
                            - mlt_producer_get_in(MLT_LINK_PRODUCER(self));
    mlt_position length = mlt_producer_get_length(MLT_LINK_PRODUCER(self));
    mlt_properties scale_map = mlt_properties_get_data(prop, "_resolution_scale", NULL);
    int count = mlt_properties_count(prop);
    int i;

    for (i = 0; i < count; i++) {
        char *name = mlt_properties_get_name(prop, i);
        if (!strncmp(name, PARAM_PREFIX, PARAM_PREFIX_LEN)) {
            const AVOption *opt
                = av_opt_find(pdata->avfilter_ctx->priv, name + PARAM_PREFIX_LEN, 0, 0, 0);
            if (animatable_avoption(opt) && mlt_properties_is_anim(prop, name)) {
                double x = mlt_properties_anim_get_double(prop, name, position, length);
                if (scale != 1.0) {
                    double scale2 = mlt_properties_get_double(scale_map, opt->name);
                    if (scale2 != 0.0) {
                        x *= scale * scale2;
                    }
                }
                mlt_properties_set_double(prop, "_avfilter_temp", x);
                char *new_val = mlt_properties_get(prop, "_avfilter_temp");
                char *cur_val = NULL;
                av_opt_get(pdata->avfilter_ctx->priv,
                           name + PARAM_PREFIX_LEN,
                           AV_OPT_SEARCH_CHILDREN,
                           (uint8_t **) &cur_val);
                if (new_val && cur_val && strcmp(new_val, cur_val)) {
                    avfilter_graph_send_command(pdata->avfilter_graph,
                                                pdata->avfilter->name,
                                                name + PARAM_PREFIX_LEN,
                                                new_val,
                                                NULL,
                                                0,
                                                0);
                }
                av_free(cur_val);
            }
        }
    }
#endif
}

static void init_audio_filtergraph(mlt_link self,
                                   mlt_audio_format format,
                                   int frequency,
                                   int channels)
{
    private_data *pdata = (private_data *) self->child;
    const AVFilter *abuffersrc = avfilter_get_by_name("abuffer");
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    int sample_fmts[] = {-1, -1};
    int sample_rates[] = {-1, -1};
    int channel_counts[] = {-1, -1};
    char channel_layout_str[64];
    int ret;

    pdata->format = format;

    // Set up formats
    sample_fmts[0] = mlt_to_av_sample_format(format);
    sample_rates[0] = frequency;
    channel_counts[0] = channels;
#if HAVE_FFMPEG_CH_LAYOUT
    AVChannelLayout ch_layout;
    av_channel_layout_default(&ch_layout, channels);
    av_channel_layout_describe(&ch_layout, channel_layout_str, sizeof(channel_layout_str));
    av_channel_layout_uninit(&ch_layout);
#else
    int64_t channel_layouts[] = {-1, -1};
    channel_layouts[0] = av_get_default_channel_layout(channels);
    av_get_channel_layout_string(channel_layout_str,
                                 sizeof(channel_layout_str),
                                 0,
                                 channel_layouts[0]);
#endif

    // Destroy the current filter graph
    avfilter_graph_free(&pdata->avfilter_graph);

    // Create the new filter graph
    pdata->avfilter_graph = avfilter_graph_alloc();
    if (!pdata->avfilter_graph) {
        mlt_log_error(self, "Cannot create filter graph\n");
        goto fail;
    }

    // Set thread count if supported.
    if (pdata->avfilter->flags & AVFILTER_FLAG_SLICE_THREADS) {
        av_opt_set_int(pdata->avfilter_graph,
                       "threads",
                       FFMAX(0, mlt_properties_get_int(MLT_LINK_PROPERTIES(self), "av.threads")),
                       0);
    }

    // Initialize the buffer source filter context
    pdata->avbuffsrc_ctx = avfilter_graph_alloc_filter(pdata->avfilter_graph, abuffersrc, "in");
    if (!pdata->avbuffsrc_ctx) {
        mlt_log_error(self, "Cannot create audio buffer source\n");
        goto fail;
    }
    ret = av_opt_set_int(pdata->avbuffsrc_ctx,
                         "sample_rate",
                         sample_rates[0],
                         AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set src sample rate %d\n", sample_rates[0]);
        goto fail;
    }
    ret = av_opt_set_int(pdata->avbuffsrc_ctx, "sample_fmt", sample_fmts[0], AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set src sample format %d\n", sample_fmts[0]);
        goto fail;
    }
    ret = av_opt_set_int(pdata->avbuffsrc_ctx,
                         "channels",
                         channel_counts[0],
                         AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set src channels %d\n", channel_counts[0]);
        goto fail;
    }
    ret = av_opt_set(pdata->avbuffsrc_ctx,
                     "channel_layout",
                     channel_layout_str,
                     AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set src channel layout %s\n", channel_layout_str);
        goto fail;
    }
    ret = avfilter_init_str(pdata->avbuffsrc_ctx, NULL);
    if (ret < 0) {
        mlt_log_error(self, "Cannot init buffer source\n");
        goto fail;
    }

    // Initialize the buffer sink filter context
    pdata->avbuffsink_ctx = avfilter_graph_alloc_filter(pdata->avfilter_graph, abuffersink, "out");
    if (!pdata->avbuffsink_ctx) {
        mlt_log_error(self, "Cannot create audio buffer sink\n");
        goto fail;
    }
    ret = av_opt_set_int_list(pdata->avbuffsink_ctx,
                              "sample_fmts",
                              sample_fmts,
                              -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set sink sample formats\n");
        goto fail;
    }
    ret = av_opt_set_int_list(pdata->avbuffsink_ctx,
                              "sample_rates",
                              sample_rates,
                              -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set sink sample rates\n");
        goto fail;
    }
#if HAVE_FFMPEG_CH_LAYOUT
    ret = av_opt_set(pdata->avbuffsink_ctx,
                     "ch_layouts",
                     channel_layout_str,
                     AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set sink ch_layouts\n");
        goto fail;
    }
#else
    ret = av_opt_set_int_list(pdata->avbuffsink_ctx,
                              "channel_counts",
                              channel_counts,
                              -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set sink channel counts\n");
        goto fail;
    }
    ret = av_opt_set_int_list(pdata->avbuffsink_ctx,
                              "channel_layouts",
                              channel_layouts,
                              -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set sink channel_layouts\n");
        goto fail;
    }
#endif
    ret = avfilter_init_str(pdata->avbuffsink_ctx, NULL);
    if (ret < 0) {
        mlt_log_error(self, "Cannot init buffer sink\n");
        goto fail;
    }

    // Initialize the filter context
    pdata->avfilter_ctx = avfilter_graph_alloc_filter(pdata->avfilter_graph,
                                                      pdata->avfilter,
                                                      pdata->avfilter->name);
    if (!pdata->avfilter_ctx) {
        mlt_log_error(self, "Cannot create audio filter\n");
        goto fail;
    }
    set_avfilter_options(self, 1.0);
    ret = avfilter_init_str(pdata->avfilter_ctx, NULL);
    if (ret < 0) {
        mlt_log_error(self, "Cannot init filter\n");
        goto fail;
    }

    // Connect the filters
    ret = avfilter_link(pdata->avbuffsrc_ctx, 0, pdata->avfilter_ctx, 0);
    if (ret < 0) {
        mlt_log_error(self, "Cannot link src to filter\n");
        goto fail;
    }
    ret = avfilter_link(pdata->avfilter_ctx, 0, pdata->avbuffsink_ctx, 0);
    if (ret < 0) {
        mlt_log_error(self, "Cannot link filter to sink\n");
        goto fail;
    }

    // Configure the graph.
    ret = avfilter_graph_config(pdata->avfilter_graph, NULL);
    if (ret < 0) {
        mlt_log_error(self, "Cannot configure the filter graph\n");
        goto fail;
    }

    return;

fail:
    avfilter_graph_free(&pdata->avfilter_graph);
}

static void init_image_filtergraph(
    mlt_link self, mlt_image_format format, int width, int height, double resolution_scale)
{
    private_data *pdata = (private_data *) self->child;
    mlt_profile profile = mlt_service_profile(MLT_LINK_SERVICE(self));
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    const AVFilter *scale = avfilter_get_by_name("scale");
    const AVFilter *pad = avfilter_get_by_name("pad");
    mlt_properties p = mlt_properties_new();
    enum AVPixelFormat pixel_fmts[] = {-1, -1};
    AVRational sar = (AVRational){profile->sample_aspect_num, profile->sample_aspect_den};
    AVRational timebase = (AVRational){profile->frame_rate_den, profile->frame_rate_num};
    AVRational framerate = (AVRational){profile->frame_rate_num, profile->frame_rate_den};
    int ret;

    pdata->format = format;
    pdata->width = width;
    pdata->height = height;

    // Set up formats
    pixel_fmts[0] = mlt_to_av_image_format(format);

    // Destroy the current filter graph
    avfilter_graph_free(&pdata->avfilter_graph);

    // Create the new filter graph
    pdata->avfilter_graph = avfilter_graph_alloc();
    if (!pdata->avfilter_graph) {
        mlt_log_error(self, "Cannot create filter graph\n");
        goto fail;
    }
    pdata->avfilter_graph->scale_sws_opts = av_strdup("flags=" MLT_AVFILTER_SWS_FLAGS);

    // Set thread count if supported.
    if (pdata->avfilter->flags & AVFILTER_FLAG_SLICE_THREADS) {
        av_opt_set_int(pdata->avfilter_graph,
                       "threads",
                       FFMAX(0, mlt_properties_get_int(MLT_LINK_PROPERTIES(self), "av.threads")),
                       0);
    }

    // Initialize the buffer source filter context
    pdata->avbuffsrc_ctx = avfilter_graph_alloc_filter(pdata->avfilter_graph, buffersrc, "in");
    if (!pdata->avbuffsrc_ctx) {
        mlt_log_error(self, "Cannot create image buffer source\n");
        goto fail;
    }
    ret = av_opt_set_int(pdata->avbuffsrc_ctx, "width", width, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set src width %d\n", width);
        goto fail;
    }
    ret = av_opt_set_int(pdata->avbuffsrc_ctx, "height", height, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set src height %d\n", height);
        goto fail;
    }
    ret = av_opt_set_pixel_fmt(pdata->avbuffsrc_ctx,
                               "pix_fmt",
                               pixel_fmts[0],
                               AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set src pixel format %d\n", pixel_fmts[0]);
        goto fail;
    }
    ret = av_opt_set_q(pdata->avbuffsrc_ctx, "sar", sar, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set src sar %d/%d\n", sar.num, sar.den);
        goto fail;
    }
    ret = av_opt_set_q(pdata->avbuffsrc_ctx, "time_base", timebase, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set src time_base %d/%d\n", timebase.num, timebase.den);
        goto fail;
    }
    ret = av_opt_set_q(pdata->avbuffsrc_ctx, "frame_rate", framerate, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set src frame_rate %d/%d\n", framerate.num, framerate.den);
        goto fail;
    }
    ret = avfilter_init_str(pdata->avbuffsrc_ctx, NULL);
    if (ret < 0) {
        mlt_log_error(self, "Cannot init buffer source\n");
        goto fail;
    }

    // Initialize the buffer sink filter context
    pdata->avbuffsink_ctx = avfilter_graph_alloc_filter(pdata->avfilter_graph, buffersink, "out");
    if (!pdata->avbuffsink_ctx) {
        mlt_log_error(self, "Cannot create image buffer sink\n");
        goto fail;
    }
    ret = av_opt_set_int_list(pdata->avbuffsink_ctx,
                              "pix_fmts",
                              pixel_fmts,
                              -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set sink pixel formats\n");
        goto fail;
    }
    ret = avfilter_init_str(pdata->avbuffsink_ctx, NULL);
    if (ret < 0) {
        mlt_log_error(self, "Cannot init buffer sink\n");
        goto fail;
    }

    // Initialize the filter context
    pdata->avfilter_ctx = avfilter_graph_alloc_filter(pdata->avfilter_graph,
                                                      pdata->avfilter,
                                                      pdata->avfilter->name);
    if (!pdata->avfilter_ctx) {
        mlt_log_error(self, "Cannot create video filter\n");
        goto fail;
    }
    set_avfilter_options(self, resolution_scale);

    if (!strcmp("lut3d", pdata->avfilter->name)) {
#if defined(__GLIBC__) || defined(__APPLE__) || (__FreeBSD__)
        // LUT data files use period for the decimal point regardless of LC_NUMERIC.
        mlt_locale_t posix_locale = newlocale(LC_NUMERIC_MASK, "POSIX", NULL);
        // Get the current locale and switch to POSIX local.
        mlt_locale_t orig_locale = uselocale(posix_locale);
        // Initialize the filter.
        ret = avfilter_init_str(pdata->avfilter_ctx, NULL);
        // Restore the original locale.
        uselocale(orig_locale);
        freelocale(posix_locale);
#else
        // Get the current locale and switch to POSIX local.
        char *orig_localename = strdup(setlocale(LC_NUMERIC, NULL));
        setlocale(LC_NUMERIC, "C");
        // Initialize the filter.
        ret = avfilter_init_str(pdata->avfilter_ctx, NULL);
        // Restore the original locale.
        setlocale(LC_NUMERIC, orig_localename);
        free(orig_localename);
#endif
    } else {
        ret = avfilter_init_str(pdata->avfilter_ctx, NULL);
    }
    if (ret < 0) {
        mlt_log_error(self, "Cannot init scale filter: %s\n", av_err2str(ret));
        goto fail;
    }

    // Initialize the scale filter context
    pdata->scale_ctx = avfilter_graph_alloc_filter(pdata->avfilter_graph, scale, "scale");
    if (!pdata->scale_ctx) {
        mlt_log_error(self, "Cannot create scale filer\n");
        goto fail;
    }
    mlt_properties_set_int(p, "w", width);
    mlt_properties_set_int(p, "h", height);
    const AVOption *opt = av_opt_find(pdata->scale_ctx->priv, "w", 0, 0, 0);
    if (opt) {
        ret = av_opt_set(pdata->scale_ctx->priv, opt->name, mlt_properties_get(p, "w"), 0);
        if (ret < 0) {
            mlt_log_error(self, "Cannot set scale width\n");
            goto fail;
        }
    }
    opt = av_opt_find(pdata->scale_ctx->priv, "h", 0, 0, 0);
    if (opt) {
        ret = av_opt_set(pdata->scale_ctx->priv, opt->name, mlt_properties_get(p, "h"), 0);
        if (ret < 0) {
            mlt_log_error(self, "Cannot set scale height\n");
            goto fail;
        }
    }
    ret = av_opt_set_int(pdata->scale_ctx, "force_original_aspect_ratio", 1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        mlt_log_error(self, "Cannot set scale force_original_aspect_ratio\n");
        goto fail;
    }
    opt = av_opt_find(pdata->scale_ctx->priv, "flags", 0, 0, 0);
    if (opt) {
        ret = av_opt_set(pdata->scale_ctx->priv, opt->name, MLT_AVFILTER_SWS_FLAGS, 0);
        if (ret < 0) {
            mlt_log_error(self, "Cannot set scale flags\n");
            goto fail;
        }
    }
    ret = avfilter_init_str(pdata->scale_ctx, NULL);
    if (ret < 0) {
        mlt_log_error(self, "Cannot init scale filter\n");
        goto fail;
    }

    // Initialize the padding filter context
    pdata->pad_ctx = avfilter_graph_alloc_filter(pdata->avfilter_graph, pad, "pad");
    if (!pdata->pad_ctx) {
        mlt_log_error(self, "Cannot create pad filter\n");
        goto fail;
    }
    opt = av_opt_find(pdata->pad_ctx->priv, "w", 0, 0, 0);
    if (opt) {
        ret = av_opt_set(pdata->pad_ctx->priv, opt->name, mlt_properties_get(p, "w"), 0);
        if (ret < 0) {
            mlt_log_error(self, "Cannot set pad width\n");
            goto fail;
        }
    }
    opt = av_opt_find(pdata->pad_ctx->priv, "h", 0, 0, 0);
    if (opt) {
        ret = av_opt_set(pdata->pad_ctx->priv, opt->name, mlt_properties_get(p, "h"), 0);
        if (ret < 0) {
            mlt_log_error(self, "Cannot pad scale height\n");
            goto fail;
        }
    }
    opt = av_opt_find(pdata->pad_ctx->priv, "x", 0, 0, 0);
    if (opt) {
        ret = av_opt_set(pdata->pad_ctx->priv, opt->name, "(ow-iw)/2", 0);
        if (ret < 0) {
            mlt_log_error(self, "Cannot set pad x\n");
            goto fail;
        }
    }
    opt = av_opt_find(pdata->pad_ctx->priv, "y", 0, 0, 0);
    if (opt) {
        ret = av_opt_set(pdata->pad_ctx->priv, opt->name, "(oh-ih)/2", 0);
        if (ret < 0) {
            mlt_log_error(self, "Cannot set pad y\n");
            goto fail;
        }
    }
    ret = avfilter_init_str(pdata->pad_ctx, NULL);
    if (ret < 0) {
        mlt_log_error(self, "Cannot init pad filter\n");
        goto fail;
    }

    // Connect the filters
    ret = avfilter_link(pdata->avbuffsrc_ctx, 0, pdata->avfilter_ctx, 0);
    if (ret < 0) {
        mlt_log_error(self, "Cannot link src to filter\n");
        goto fail;
    }
    ret = avfilter_link(pdata->avfilter_ctx, 0, pdata->scale_ctx, 0);
    if (ret < 0) {
        mlt_log_error(self, "Cannot link filter to scale\n");
        goto fail;
    }
    ret = avfilter_link(pdata->scale_ctx, 0, pdata->pad_ctx, 0);
    if (ret < 0) {
        mlt_log_error(self, "Cannot link scale to pad\n");
        goto fail;
    }
    ret = avfilter_link(pdata->pad_ctx, 0, pdata->avbuffsink_ctx, 0);
    if (ret < 0) {
        mlt_log_error(self, "Cannot link pad to sink\n");
        goto fail;
    }

    // Configure the graph.
    ret = avfilter_graph_config(pdata->avfilter_graph, NULL);
    if (ret < 0) {
        mlt_log_error(self, "Cannot configure the filter graph\n");
        goto fail;
    }

    return;

fail:
    mlt_properties_close(p);
    avfilter_graph_free(&pdata->avfilter_graph);
}

static mlt_position get_position(mlt_link self, mlt_frame frame)
{
    mlt_position position = mlt_frame_get_position(frame);
    const char *pos_type = mlt_properties_get(MLT_LINK_PROPERTIES(self), "position");
    if (pos_type) {
        if (!strcmp("link", pos_type)) {
            position = mlt_producer_position(MLT_LINK_PRODUCER(self));
        } else if (!strcmp("source", pos_type)) {
            position = mlt_frame_original_position(frame);
        }
    } else {
        private_data *pdata = (private_data *) self->child;
        if (!strcmp("subtitles", pdata->avfilter->name))
            position = mlt_frame_original_position(frame);
    }
    return position;
}

static void link_configure(mlt_link self, mlt_profile chain_profile)
{
    // Operate at the same frame rate as the next link
    mlt_service_set_profile(MLT_LINK_SERVICE(self),
                            mlt_service_profile(MLT_PRODUCER_SERVICE(self->next)));
}

static int link_get_audio(mlt_frame frame,
                          void **buffer,
                          mlt_audio_format *format,
                          int *frequency,
                          int *channels,
                          int *samples)
{
    int error = 0;
    mlt_link self = mlt_frame_pop_audio(frame);
    private_data *pdata = (private_data *) self->child;
    double fps = mlt_profile_fps(mlt_service_profile(MLT_LINK_SERVICE(self)));
    int ret;

    mlt_service_lock(MLT_LINK_SERVICE(self));

    if (pdata->reset || pdata->format != *format || pdata->channels != *channels
        || pdata->frequency != *frequency
        || pdata->expected_frame != mlt_frame_get_position(frame)) {
        mlt_log_error(MLT_LINK_SERVICE(self),
                      "Init: %s\t%dc\t%dHz\n",
                      mlt_audio_format_name(*format),
                      *channels,
                      *frequency);
        init_audio_filtergraph(self, *format, *frequency, *channels);
        pdata->reset = 0;
        pdata->format = *format;
        pdata->channels = *channels;
        pdata->frequency = *frequency;
        pdata->continuity_frame = mlt_frame_get_position(frame);
        pdata->expected_frame = mlt_frame_get_position(frame);
    }

    pdata->expected_frame++;

    while (pdata->avfilter_graph) {
        // Choose the frame to use (maybe a future frame)
        mlt_frame src_frame = NULL;
        if (pdata->continuity_frame == mlt_frame_get_position(frame)) {
            src_frame = frame;
            pdata->continuity_frame++;
        } else {
            mlt_properties unique_properties
                = mlt_frame_get_unique_properties(frame, MLT_LINK_SERVICE(self));
            if (!unique_properties) {
                mlt_log_error(MLT_LINK_SERVICE(self), "Missing future frames\n");
                error = 1;
                break;
            }
            char key[19];
            int frame_delta = mlt_frame_get_position(frame) - mlt_frame_original_position(frame);
            sprintf(key, "%d", pdata->continuity_frame - frame_delta);
            src_frame = (mlt_frame) mlt_properties_get_data(unique_properties, key, NULL);
            if (!src_frame) {
                mlt_log_error(MLT_LINK_SERVICE(self), "Frame not found: %s\n", key);
                error = 1;
                break;
            }
            pdata->continuity_frame++;
        }

        // Get the producer's audio
        struct mlt_audio_s in;
        mlt_audio_set_values(&in, NULL, *frequency, *format, 0, *channels);
        in.samples = mlt_audio_calculate_frame_samples(mlt_producer_get_fps(MLT_LINK_PRODUCER(self)),
                                                       in.frequency,
                                                       mlt_frame_get_position(src_frame));
        error = mlt_frame_get_audio(src_frame,
                                    &in.data,
                                    &in.format,
                                    &in.frequency,
                                    &in.channels,
                                    &in.samples);
        if (error || in.format != *format || in.frequency != *frequency
            || in.channels != *channels) {
            // Error situation. Do not attempt to process.
            mlt_log_error(MLT_LINK_SERVICE(self),
                          "Invalid Return: E: %d %dS - %dHz %dC %s\n",
                          error,
                          in.samples,
                          in.frequency,
                          in.channels,
                          mlt_audio_format_name(in.format));
            error = 1;
            break;
        }

        // Set up the input frame
        mlt_channel_layout layout
            = mlt_get_channel_layout_or_default(mlt_properties_get(MLT_FRAME_PROPERTIES(src_frame),
                                                                   "channel_layout"),
                                                in.channels);
        int64_t samplepos = mlt_audio_calculate_samples_to_position(fps,
                                                                    *frequency,
                                                                    get_position(self, src_frame));
        int inbufsize = mlt_audio_format_size(in.format, in.samples, in.channels);
        pdata->avinframe->sample_rate = in.frequency;
        pdata->avinframe->format = mlt_to_av_sample_format(in.format);
#if HAVE_FFMPEG_CH_LAYOUT
        av_channel_layout_from_mask(&pdata->avinframe->ch_layout, mlt_to_av_channel_layout(layout));
#else
        pdata->avinframe->channel_layout = mlt_to_av_channel_layout(layout);
        pdata->avinframe->channels = in.channels;
#endif
        pdata->avinframe->nb_samples = in.samples;
        pdata->avinframe->pts = samplepos;
        ret = av_frame_get_buffer(pdata->avinframe, 1);
        if (ret < 0) {
            mlt_log_error(self, "Cannot get in frame buffer\n");
        }

        if (av_sample_fmt_is_planar(pdata->avinframe->format)) {
            int i = 0;
            int stride = inbufsize / *channels;
            for (i = 0; i < *channels; i++) {
                memcpy(pdata->avinframe->extended_data[i], (uint8_t *) in.data + stride * i, stride);
            }
        } else {
            memcpy(pdata->avinframe->extended_data[0], (uint8_t *) in.data, inbufsize);
        }
        send_avformat_commands(self, frame, pdata, 1.0);

        // Run the frame through the filter graph
        ret = av_buffersrc_add_frame(pdata->avbuffsrc_ctx, pdata->avinframe);
        if (ret < 0) {
            mlt_log_error(self, "Cannot add frame to buffer source\n");
        }
        ret = av_buffersink_get_samples(pdata->avbuffsink_ctx, pdata->avoutframe, *samples);
        if (ret == AVERROR(EAGAIN)) {
            mlt_log_debug(self, "Need more samples from next future frame\n");
            continue;
        } else if (ret < 0) {
            mlt_log_error(self, "Cannot get frame from buffer sink\n");
            error = 1;
            break;
        }

        // Sanity check the output frame
#if HAVE_FFMPEG_CH_LAYOUT
        if (*channels != pdata->avoutframe->ch_layout.nb_channels
#else
        if (*channels != pdata->avoutframe->channels
#endif
            || *samples != pdata->avoutframe->nb_samples
            || *frequency != pdata->avoutframe->sample_rate) {
            mlt_log_error(self,
                          "Unexpected return format c %d->%d\tf %d->%d\tf %d->%d\n",
                          *channels,
#if HAVE_FFMPEG_CH_LAYOUT
                          pdata->avoutframe->ch_layout.nb_channels,
#else
                          pdata->avoutframe->channels,
#endif
                          *samples,
                          pdata->avoutframe->nb_samples,
                          *frequency,
                          pdata->avoutframe->sample_rate);
            error = 1;
            break;
        }

        // Copy the filter output into the frame
        int bufsize = mlt_audio_format_size(*format, *samples, *channels);
        *buffer = mlt_pool_alloc(bufsize);
        if (av_sample_fmt_is_planar(pdata->avoutframe->format)) {
            int stride = bufsize / *channels;
            int i = 0;
            for (i = 0; i < *channels; i++) {
                memcpy((uint8_t *) *buffer + stride * i,
                       pdata->avoutframe->extended_data[i],
                       stride);
            }
        } else {
            memcpy((uint8_t *) *buffer, pdata->avoutframe->extended_data[0], bufsize);
        }
        mlt_frame_set_audio(frame, *buffer, *format, bufsize, mlt_pool_release);
        break;
    }

    av_frame_unref(pdata->avinframe);
    av_frame_unref(pdata->avoutframe);

    if (error) {
        // Return unprocessed audio if an error occurs
        error = mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    } else {
        // Clear the audio stack in case get_audio was not called on this frame.
        while (mlt_frame_pop_audio(frame)) {
        };
    }

    mlt_service_unlock(MLT_LINK_SERVICE(self));
    return error;
}

static int link_get_image(mlt_frame frame,
                          uint8_t **image,
                          mlt_image_format *format,
                          int *width,
                          int *height,
                          int writable)
{
    mlt_link self = mlt_frame_pop_service(frame);
    private_data *pdata = (private_data *) self->child;
    int64_t pos = get_position(self, frame);
    mlt_profile profile = mlt_service_profile(MLT_LINK_SERVICE(self));
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    int ret;

    mlt_log_debug(MLT_LINK_SERVICE(self), "position %" PRId64 "\n", pos);
    if (mlt_properties_get_int(MLT_LINK_PROPERTIES(self), "_yuv_only")) {
        *format = mlt_image_yuv422;
    } else {
        *format = mlt_get_supported_image_format(*format);
    }

    mlt_frame_get_image(frame, image, format, width, height, 0);

    mlt_service_lock(MLT_LINK_SERVICE(self));

    double scale = mlt_profile_scale_width(profile, *width);

    if (pdata->reset || pdata->format != *format || pdata->width != *width
        || pdata->height != *height) {
        init_image_filtergraph(self, *format, *width, *height, scale);
        pdata->reset = 0;
    }

    if (pdata->avfilter_graph) {
        pdata->avinframe->width = *width;
        pdata->avinframe->height = *height;
        pdata->avinframe->format = mlt_to_av_image_format(*format);
        pdata->avinframe->sample_aspect_ratio = (AVRational){profile->sample_aspect_num,
                                                             profile->sample_aspect_den};
        pdata->avinframe->pts = pos;
        pdata->avinframe->interlaced_frame = !mlt_properties_get_int(frame_properties,
                                                                     "progressive");
        pdata->avinframe->top_field_first = mlt_properties_get_int(frame_properties,
                                                                   "top_field_first");
        pdata->avinframe->color_primaries = mlt_properties_get_int(frame_properties,
                                                                   "color_primaries");
        pdata->avinframe->color_trc = mlt_properties_get_int(frame_properties, "color_trc");
        pdata->avinframe->color_range = mlt_properties_get_int(frame_properties, "full_range")
                                            ? AVCOL_RANGE_JPEG
                                            : AVCOL_RANGE_MPEG;

        switch (mlt_properties_get_int(frame_properties, "colorspace")) {
        case 240:
            pdata->avinframe->colorspace = AVCOL_SPC_SMPTE240M;
            break;
        case 601:
            pdata->avinframe->colorspace = AVCOL_SPC_BT470BG;
            break;
        case 709:
            pdata->avinframe->colorspace = AVCOL_SPC_BT709;
            break;
        case 2020:
            pdata->avinframe->colorspace = AVCOL_SPC_BT2020_NCL;
            break;
        case 2021:
            pdata->avinframe->colorspace = AVCOL_SPC_BT2020_CL;
            break;
        }

        ret = av_frame_get_buffer(pdata->avinframe, 1);
        if (ret < 0) {
            mlt_log_error(self, "Cannot get in frame buffer\n");
        }

        // Set up the input frame
        if (*format == mlt_image_yuv420p) {
            int i = 0;
            int p = 0;
            int widths[3] = {*width, *width / 2, *width / 2};
            int heights[3] = {*height, *height / 2, *height / 2};
            uint8_t *src = *image;
            for (p = 0; p < 3; p++) {
                uint8_t *dst = pdata->avinframe->data[p];
                for (i = 0; i < heights[p]; i++) {
                    memcpy(dst, src, widths[p]);
                    src += widths[p];
                    dst += pdata->avinframe->linesize[p];
                }
            }
        } else {
            int i;
            uint8_t *src = *image;
            uint8_t *dst = pdata->avinframe->data[0];
            int stride = mlt_image_format_size(*format, *width, 1, NULL);
            for (i = 0; i < *height; i++) {
                memcpy(dst, src, stride);
                src += stride;
                dst += pdata->avinframe->linesize[0];
            }
        }
        send_avformat_commands(self, frame, pdata, scale);

        // Run the frame through the filter graph
        ret = av_buffersrc_add_frame(pdata->avbuffsrc_ctx, pdata->avinframe);
        if (ret < 0) {
            mlt_log_error(self, "Cannot add frame to buffer source\n");
        }
        ret = av_buffersink_get_frame(pdata->avbuffsink_ctx, pdata->avoutframe);
        if (ret < 0) {
            mlt_log_error(self, "Cannot get frame from buffer sink\n");
        }

        // Sanity check the output frame
        if (*width != pdata->avoutframe->width || *height != pdata->avoutframe->height) {
            mlt_log_error(self, "Unexpected return format\n");
            goto exit;
        }

        // Copy the filter output into the original buffer
        if (*format == mlt_image_yuv420p) {
            int i = 0;
            int p = 0;
            int widths[3] = {*width, *width / 2, *width / 2};
            int heights[3] = {*height, *height / 2, *height / 2};
            uint8_t *dst = *image;
            for (p = 0; p < 3; p++) {
                uint8_t *src = pdata->avoutframe->data[p];
                for (i = 0; i < heights[p]; i++) {
                    memcpy(dst, src, widths[p]);
                    dst += widths[p];
                    src += pdata->avoutframe->linesize[p];
                }
            }
        } else {
            int i;
            uint8_t *dst = *image;
            uint8_t *src = pdata->avoutframe->data[0];
            int stride = mlt_image_format_size(*format, *width, 1, NULL);
            for (i = 0; i < *height; i++) {
                memcpy(dst, src, stride);
                dst += stride;
                src += pdata->avoutframe->linesize[0];
            }
        }
    }

exit:
    av_frame_unref(pdata->avinframe);
    av_frame_unref(pdata->avoutframe);
    mlt_service_unlock(MLT_LINK_SERVICE(self));
    return 0;
}

static int link_get_frame(mlt_link self, mlt_frame_ptr frame, int index)
{
    int error = 0;
    mlt_position frame_pos = mlt_producer_position(MLT_LINK_PRODUCER(self));

    mlt_producer_seek(self->next, frame_pos);
    error = mlt_service_get_frame(MLT_PRODUCER_SERVICE(self->next), frame, index);
    mlt_properties unique_properties = mlt_frame_unique_properties(*frame, MLT_LINK_SERVICE(self));

    // Pass future frames
    int future_frames = future_frames_needed(self);
    int i = 0;
    for (i = 0; i < future_frames; i++) {
        mlt_position future_pos = frame_pos + i + 1;
        mlt_frame future_frame = NULL;
        mlt_producer_seek(self->next, future_pos);
        error = mlt_service_get_frame(MLT_PRODUCER_SERVICE(self->next), &future_frame, index);
        if (error) {
            mlt_log_error(MLT_LINK_SERVICE(self), "Error getting frame: %d\n", (int) future_pos);
        }
        char key[19];
        sprintf(key, "%d", (int) future_pos);
        mlt_properties_set_data(unique_properties,
                                key,
                                future_frame,
                                0,
                                (mlt_destructor) mlt_frame_close,
                                NULL);
    }

    private_data *pdata = (private_data *) self->child;
    if (avfilter_pad_get_type(pdata->avfilter->inputs, 0) == AVMEDIA_TYPE_VIDEO) {
        mlt_frame_push_service(*frame, self);
        mlt_frame_push_get_image(*frame, link_get_image);
    } else if (avfilter_pad_get_type(pdata->avfilter->inputs, 0) == AVMEDIA_TYPE_AUDIO) {
        mlt_frame_push_audio(*frame, self);
        mlt_frame_push_audio(*frame, link_get_audio);
    }

    mlt_producer_prepare_next(MLT_LINK_PRODUCER(self));

    return error;
}

static void link_close(mlt_link self)
{
    if (self) {
        private_data *pdata = (private_data *) self->child;
        if (pdata) {
            avfilter_graph_free(&pdata->avfilter_graph);
            av_frame_free(&pdata->avinframe);
            av_frame_free(&pdata->avoutframe);
            free(pdata);
        }
        self->close = NULL;
        mlt_link_close(self);
        free(self);
    }
}

mlt_link link_avfilter_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_link self = mlt_link_init();
    private_data *pdata = (private_data *) calloc(1, sizeof(private_data));

    if (pdata && id) {
        id += 9; // Move past "avfilter."
        pdata->avfilter = (AVFilter *) avfilter_get_by_name(id);
    }

    if (self && pdata && pdata->avfilter) {
        pdata->avbuffsink_ctx = NULL;
        pdata->avbuffsrc_ctx = NULL;
        pdata->avfilter_ctx = NULL;
        pdata->avfilter_graph = NULL;
        pdata->avinframe = av_frame_alloc();
        pdata->avoutframe = av_frame_alloc();
        pdata->format = -1;
        pdata->width = -1;
        pdata->height = -1;
        pdata->reset = 1;

        self->child = pdata;

        // Callback registration
        self->configure = link_configure;
        self->get_frame = link_get_frame;
        self->close = link_close;

        mlt_events_listen(MLT_LINK_PROPERTIES(self),
                          self,
                          "property-changed",
                          (mlt_listener) property_changed);

        mlt_properties param_name_map = mlt_properties_get_data(mlt_global_properties(),
                                                                "avfilter.resolution_scale",
                                                                NULL);
        if (param_name_map) {
            // Lookup my plugin in the map
            param_name_map = mlt_properties_get_data(param_name_map, id, NULL);
            mlt_properties_set_data(MLT_LINK_PROPERTIES(self),
                                    "_resolution_scale",
                                    param_name_map,
                                    0,
                                    NULL,
                                    NULL);
        }

        mlt_properties yuv_only = mlt_properties_get_data(mlt_global_properties(),
                                                          "avfilter.yuv_only",
                                                          NULL);
        if (yuv_only) {
            if (mlt_properties_get(yuv_only, id)) {
                mlt_properties_set_int(MLT_LINK_PROPERTIES(self), "_yuv_only", 1);
            }
        }
    } else {
        free(pdata);
        mlt_link_close(self);
        self = NULL;
    }
    return self;
}
