/**
 * \file mlt_audio.h
 * \brief Audio class
 * \see mlt_audio_s
 *
 * Copyright (C) 2020 Meltytech, LLC
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

#ifndef MLT_AUDIO_H
#define MLT_AUDIO_H

#include "mlt_types.h"

/** \brief Audio class
 *
 * Audio is the data object that represents audio for a period of time.
 */

struct mlt_audio_s
{
    void *data;
    int frequency;
    mlt_audio_format format;
    int samples;
    int channels;
    mlt_channel_layout layout;
    mlt_destructor release_data;
    mlt_destructor close;
};

extern mlt_audio mlt_audio_new();
extern void mlt_audio_close(mlt_audio self);
extern void mlt_audio_set_values(
    mlt_audio self, void *data, int frequency, mlt_audio_format format, int samples, int channels);
extern void mlt_audio_get_values(mlt_audio self,
                                 void **data,
                                 int *frequency,
                                 mlt_audio_format *format,
                                 int *samples,
                                 int *channels);
extern void mlt_audio_alloc_data(mlt_audio self);
extern int mlt_audio_calculate_size(mlt_audio self);
extern int mlt_audio_plane_count(mlt_audio self);
extern int mlt_audio_plane_size(mlt_audio self);
extern void mlt_audio_get_planes(mlt_audio self, uint8_t **planes);
extern void mlt_audio_silence(mlt_audio self, int samples, int start);
extern void mlt_audio_shrink(mlt_audio self, int samples);
extern void mlt_audio_reverse(mlt_audio self);
extern void mlt_audio_copy(mlt_audio dst, mlt_audio src, int samples, int src_start, int dst_start);
extern int mlt_audio_calculate_frame_samples(float fps, int frequency, int64_t position);
extern int64_t mlt_audio_calculate_samples_to_position(float fps, int frequency, int64_t position);
extern const char *mlt_audio_format_name(mlt_audio_format format);
extern int mlt_audio_format_size(mlt_audio_format format, int samples, int channels);
extern const char *mlt_audio_channel_layout_name(mlt_channel_layout layout);
extern mlt_channel_layout mlt_audio_channel_layout_id(const char *name);
extern int mlt_audio_channel_layout_channels(mlt_channel_layout layout);
extern mlt_channel_layout mlt_audio_channel_layout_default(int channels);

#endif
