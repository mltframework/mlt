/*
 * common_swr.c
 * Copyright (C) 2022-2024 Meltytech, LLC
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

#include "common_swr.h"
#include "common.h"

#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>

int mlt_configure_swr_context(mlt_service service, mlt_swr_private_data *pdata)
{
    int error = 0;

    mlt_log_info(service,
                 "%d(%s) %s %dHz -> %d(%s) %s %dHz\n",
                 pdata->in_channels,
                 mlt_audio_channel_layout_name(pdata->in_layout),
                 mlt_audio_format_name(pdata->in_format),
                 pdata->in_frequency,
                 pdata->out_channels,
                 mlt_audio_channel_layout_name(pdata->out_layout),
                 mlt_audio_format_name(pdata->out_format),
                 pdata->out_frequency);

    mlt_free_swr_context(pdata);

    pdata->ctx = swr_alloc();
    if (!pdata->ctx) {
        mlt_log_error(service, "Cannot allocate context\n");
        return 1;
    }

    // Configure format, frequency and channels.
    av_opt_set_int(pdata->ctx, "osf", mlt_to_av_sample_format(pdata->out_format), 0);
    av_opt_set_int(pdata->ctx, "osr", pdata->out_frequency, 0);
    av_opt_set_int(pdata->ctx, "isf", mlt_to_av_sample_format(pdata->in_format), 0);
    av_opt_set_int(pdata->ctx, "isr", pdata->in_frequency, 0);
#if !HAVE_FFMPEG_CH_LAYOUT
    av_opt_set_int(pdata->ctx, "och", pdata->out_channels, 0);
    av_opt_set_int(pdata->ctx, "ich", pdata->in_channels, 0);
#endif

    if (pdata->in_layout != mlt_channel_independent
        && pdata->out_layout != mlt_channel_independent) {
        // Use standard channel layout and matrix for known channel configurations.
#if HAVE_FFMPEG_CH_LAYOUT
        AVChannelLayout ochl = AV_CHANNEL_LAYOUT_MASK(pdata->out_channels,
                                                      mlt_to_av_channel_layout(pdata->out_layout));
        AVChannelLayout ichl = AV_CHANNEL_LAYOUT_MASK(pdata->in_channels,
                                                      mlt_to_av_channel_layout(pdata->in_layout));
        av_opt_set_chlayout(pdata->ctx, "ochl", &ochl, 0);
        av_opt_set_chlayout(pdata->ctx, "ichl", &ichl, 0);
#else
        av_opt_set_int(pdata->ctx, "ocl", mlt_to_av_channel_layout(pdata->out_layout), 0);
        av_opt_set_int(pdata->ctx, "icl", mlt_to_av_channel_layout(pdata->in_layout), 0);
#endif
    } else {
        // Use a custom channel layout and matrix for independent channels.
        // This matrix will simply map input channels to output channels in order.
        // If input channels > output channels, channels will be dropped.
        // If input channels < output channels, silent channels will be added.
        int64_t custom_in_layout = 0;
        int64_t custom_out_layout = 0;
        double *matrix = av_calloc(pdata->in_channels * pdata->out_channels, sizeof(double));
        int stride = pdata->in_channels;
        int i = 0;

        for (i = 0; i < pdata->in_channels; i++) {
            custom_in_layout = (custom_in_layout << 1) | 0x01;
        }
        for (i = 0; i < pdata->out_channels; i++) {
            custom_out_layout = (custom_out_layout << 1) | 0x01;
            if (i < pdata->in_channels) {
                double *matrix_row = matrix + (stride * i);
                matrix_row[i] = 1.0;
            }
        }
#if HAVE_FFMPEG_CH_LAYOUT
        AVChannelLayout ochl
            = {AV_CHANNEL_ORDER_UNSPEC, pdata->out_channels, {custom_out_layout}, NULL};
        AVChannelLayout ichl
            = {AV_CHANNEL_ORDER_UNSPEC, pdata->in_channels, {custom_in_layout}, NULL};
        av_opt_set_chlayout(pdata->ctx, "ochl", &ochl, 0);
        av_opt_set_chlayout(pdata->ctx, "ichl", &ichl, 0);
#else
        av_opt_set_int(pdata->ctx, "ocl", custom_out_layout, 0);
        av_opt_set_int(pdata->ctx, "icl", custom_in_layout, 0);
#endif
        error = swr_set_matrix(pdata->ctx, matrix, stride);
        av_free(matrix);
        if (error != 0) {
            swr_free(&pdata->ctx);
            mlt_log_error(service, "Unable to create custom matrix\n");
            return error;
        }
    }

    error = swr_init(pdata->ctx);
    if (error != 0) {
        swr_free(&pdata->ctx);
        mlt_log_error(service, "Cannot initialize context\n");
        return error;
    }

    // Allocate the channel buffer pointers
    pdata->in_buffers = av_calloc(pdata->in_channels, sizeof(uint8_t *));
    pdata->out_buffers = av_calloc(pdata->out_channels, sizeof(uint8_t *));

    return error;
}

void mlt_free_swr_context(mlt_swr_private_data *pdata)
{
    if (pdata) {
        swr_free(&pdata->ctx);
        av_freep(&pdata->in_buffers);
        av_freep(&pdata->out_buffers);
    }
}
