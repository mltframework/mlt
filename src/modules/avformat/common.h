/*
 * common.h
 * Copyright (C) 2018 Meltytech, LLC
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

#ifndef COMMON_H
#define COMMON_H

#include <framework/mlt.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>

#define MLT_AVFILTER_SWS_FLAGS "bicubic+accurate_rnd+full_chroma_int+full_chroma_inp"

int mlt_to_av_sample_format(mlt_audio_format format);
int64_t mlt_to_av_channel_layout(mlt_channel_layout layout);
mlt_channel_layout av_channel_layout_to_mlt(int64_t layout);
mlt_channel_layout mlt_get_channel_layout_or_default(const char *name, int channels);
int mlt_set_luma_transfer(struct SwsContext *context,
                          int src_colorspace,
                          int dst_colorspace,
                          int src_full_range,
                          int dst_full_range);
int mlt_get_sws_flags(
    int srcwidth, int srcheight, int srcformat, int dstwidth, int dstheight, int dstformat);
int mlt_to_av_image_format(mlt_image_format format);
mlt_image_format mlt_get_supported_image_format(mlt_image_format format);
void mlt_image_to_avframe(mlt_image image, mlt_frame mltframe, AVFrame *avframe);
void avframe_to_mlt_image(AVFrame *avframe, mlt_image image);

#endif // COMMON_H
