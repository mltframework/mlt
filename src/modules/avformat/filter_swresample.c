/*
 * filter_swresample.c -- convert from one format/ configuration to another
 * Copyright (C) 2018-2024 Meltytech, LLC
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
#include "common_swr.h"

#include <framework/mlt.h>

#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>

static int filter_get_audio(mlt_frame frame,
                            void **buffer,
                            mlt_audio_format *format,
                            int *frequency,
                            int *channels,
                            int *samples)
{
    int requested_samples = *samples;
    mlt_filter filter = mlt_frame_pop_audio(frame);
    mlt_swr_private_data *pdata = (mlt_swr_private_data *) filter->child;
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    struct mlt_audio_s in;
    struct mlt_audio_s out;

    mlt_audio_set_values(&in, *buffer, *frequency, *format, *samples, *channels);
    mlt_audio_set_values(&out, NULL, *frequency, *format, *samples, *channels);

    // Get the producer's audio
    int error
        = mlt_frame_get_audio(frame, &in.data, &in.format, &in.frequency, &in.channels, &in.samples);
    if (error || in.format == mlt_audio_none || out.format == mlt_audio_none || in.frequency <= 0
        || out.frequency <= 0 || in.channels <= 0 || out.channels <= 0) {
        // Error situation. Do not attempt to convert.
        mlt_audio_get_values(&in, buffer, frequency, format, samples, channels);
        mlt_log_error(MLT_FILTER_SERVICE(filter),
                      "Invalid Parameters: %dS - %dHz %dC %s -> %dHz %dC %s\n",
                      in.samples,
                      in.frequency,
                      in.channels,
                      mlt_audio_format_name(in.format),
                      out.frequency,
                      out.channels,
                      mlt_audio_format_name(out.format));
        return error;
    }

    if (in.samples == 0) {
        // Nothing to convert.
        *samples = 0;
        return error;
    }

    // Determine the input/output channel layout.
    in.layout = mlt_get_channel_layout_or_default(mlt_properties_get(frame_properties,
                                                                     "channel_layout"),
                                                  in.channels);
    out.layout = mlt_get_channel_layout_or_default(mlt_properties_get(frame_properties,
                                                                      "consumer.channel_layout"),
                                                   out.channels);

    if (in.format == out.format && in.frequency == out.frequency && in.channels == out.channels
        && in.layout == out.layout) {
        // No change necessary
        mlt_audio_get_values(&in, buffer, frequency, format, samples, channels);
        return error;
    }

    mlt_service_lock(MLT_FILTER_SERVICE(filter));

    // Detect configuration change
    if (!pdata->ctx || pdata->in_format != in.format || pdata->out_format != out.format
        || pdata->in_frequency != in.frequency || pdata->out_frequency != out.frequency
        || pdata->in_channels != in.channels || pdata->out_channels != out.channels
        || pdata->in_layout != in.layout || pdata->out_layout != out.layout) {
        // Save the configuration
        pdata->in_format = in.format;
        pdata->out_format = out.format;
        pdata->in_frequency = in.frequency;
        pdata->out_frequency = out.frequency;
        pdata->in_channels = in.channels;
        pdata->out_channels = out.channels;
        pdata->in_layout = in.layout;
        pdata->out_layout = out.layout;
        // Reconfigure the context
        error = mlt_configure_swr_context(MLT_FILTER_SERVICE(filter), pdata);
    }

    if (!error) {
        out.samples = requested_samples;
        mlt_audio_alloc_data(&out);

        mlt_audio_get_planes(&in, pdata->in_buffers);
        mlt_audio_get_planes(&out, pdata->out_buffers);

        int received_samples = swr_convert(pdata->ctx,
                                           pdata->out_buffers,
                                           out.samples,
                                           (const uint8_t **) pdata->in_buffers,
                                           in.samples);
        if (received_samples >= 0) {
            if (received_samples == 0) {
                mlt_log_info(MLT_FILTER_SERVICE(filter), "Precharge required - return silence\n");
                mlt_audio_silence(&out, out.samples, 0);
            } else if (received_samples < requested_samples) {
                // Duplicate samples to return the exact number requested.
                mlt_audio_copy(&out, &out, received_samples, 0, requested_samples - received_samples);
            } else if (received_samples > requested_samples) {
                // Discard samples to return the exact number requested.
                mlt_audio_shrink(&out, requested_samples);
            }
            mlt_frame_set_audio(frame, out.data, out.format, 0, out.release_data);
            mlt_audio_get_values(&out, buffer, frequency, format, samples, channels);
            mlt_properties_set(frame_properties,
                               "channel_layout",
                               mlt_audio_channel_layout_name(out.layout));
        } else {
            mlt_log_error(MLT_FILTER_SERVICE(filter),
                          "swr_convert() failed. Alloc: %d\tIn: %d\tOut: %d\n",
                          out.samples,
                          in.samples,
                          received_samples);
            out.release_data(out.data);
            error = 1;
        }
    }

    mlt_service_unlock(MLT_FILTER_SERVICE(filter));
    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_frame_push_audio(frame, filter);
    mlt_frame_push_audio(frame, filter_get_audio);
    return frame;
}

static void filter_close(mlt_filter filter)
{
    mlt_swr_private_data *pdata = (mlt_swr_private_data *) filter->child;

    if (pdata) {
        mlt_free_swr_context(pdata);
        free(pdata);
    }
    filter->child = NULL;
    filter->close = NULL;
    filter->parent.close = NULL;
    mlt_service_close(&filter->parent);
}

mlt_filter filter_swresample_init(mlt_profile profile, char *arg)
{
    mlt_filter filter = mlt_filter_new();
    mlt_swr_private_data *pdata = (mlt_swr_private_data *) calloc(1, sizeof(mlt_swr_private_data));

    if (filter && pdata) {
        filter->close = filter_close;
        filter->process = filter_process;
        filter->child = pdata;
    } else {
        mlt_filter_close(filter);
        free(pdata);
    }

    return filter;
}
