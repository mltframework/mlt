/*
 * filter_autofade.c -- Automatically fade audio between clips in a playlist.
 * Copyright (C) 2023-2025 Meltytech, LLC
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

#include <framework/mlt.h>

#include <math.h>
#include <stdlib.h>

static float decay_factor(int position, int count)
{
    float factor = (float) position / (float) (count - 1);
    if (factor < 0) {
        factor = 0;
    } else if (factor > 1.0) {
        factor = 1.0;
    }
    return factor;
}

static int filter_get_audio(mlt_frame frame,
                            void **buffer,
                            mlt_audio_format *format,
                            int *frequency,
                            int *channels,
                            int *samples)
{
    mlt_filter filter = mlt_frame_pop_audio(frame);
    *format = mlt_audio_f32le;
    int ret = mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    if (ret != 0) {
        return ret;
    }

    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    int clip_position = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame),
                                               "meta.playlist.clip_position");
    int clip_length = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame),
                                             "meta.playlist.clip_length");
    int fade_duration = mlt_properties_get_int(filter_properties, "fade_duration");
    int fade_samples = fade_duration * *frequency / 1000;
    double fps = mlt_profile_fps(mlt_service_profile(MLT_FILTER_SERVICE(filter)));
    int64_t samples_to_frame_begin = mlt_audio_calculate_samples_to_position(fps,
                                                                             *frequency,
                                                                             clip_position);
    int64_t samples_in_clip = mlt_audio_calculate_samples_to_position(fps,
                                                                      *frequency,
                                                                      clip_length + 1);
    int64_t samples_to_clip_end = samples_in_clip - samples_to_frame_begin - *samples;
    struct mlt_audio_s audio;
    mlt_audio_set_values(&audio, *buffer, *frequency, *format, *samples, *channels);
    float *data = (float *) audio.data;
    if (samples_to_frame_begin <= fade_samples) {
        // Fade in
        for (int i = 0; i < audio.samples; i++) {
            float factor = decay_factor(samples_to_frame_begin + i, fade_samples);
            for (int c = 0; c < audio.channels; c++) {
                *data = *data * factor;
                data++;
            }
        }
    } else if ((samples_to_clip_end - *samples) <= fade_samples) {
        // Fade out
        for (int i = 0; i < audio.samples; i++) {
            float factor = decay_factor(samples_to_clip_end - i, fade_samples);
            for (int c = 0; c < audio.channels; c++) {
                *data = *data * factor;
                data++;
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
    int error = 0;
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);

    // Get the current image
    if (*format != mlt_image_rgba64) {
        *format = mlt_image_rgba;
    }
    error = mlt_frame_get_image(frame, image, format, width, height, 1);

    if (error)
        return error;

    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    int clip_position = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame),
                                               "meta.playlist.clip_position");
    int clip_length = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame),
                                             "meta.playlist.clip_length");
    double fade_duration = mlt_properties_get_int(filter_properties, "fade_duration");
    double fps = mlt_profile_fps(mlt_service_profile(MLT_FILTER_SERVICE(filter)));
    int fade_duration_frames = lrint(fade_duration * fps / 1000.0);
    float image_factor = 1.0;
    int frames_from_begining = clip_position + 1;
    int frames_to_end = clip_length - clip_position - 1;

    if (frames_from_begining <= fade_duration_frames) {
        // Fade In
        image_factor = decay_factor(clip_position, fade_duration_frames);
    } else if (frames_to_end <= fade_duration_frames) {
        // Fade Out
        image_factor = decay_factor(frames_to_end, fade_duration_frames);
    }

    if (image_factor < 1.0) {
        mlt_color color = mlt_properties_get_color(filter_properties, "fade_color");
        color = mlt_color_convert_trc(color,
                                      mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "color_trc"));
        float color_factor = 1.0 - image_factor;
        float r_value = color.r * color_factor;
        float g_value = color.g * color_factor;
        float b_value = color.b * color_factor;
        float a_value = color.a * color_factor;
        int pixels = *width * *height;
        if (*format == mlt_image_rgba64) {
            uint16_t *p = (uint16_t *) *image;
            for (int i = 0; i < pixels; i++) {
                p[0] = ((float) p[0] * image_factor) + r_value;
                p[1] = ((float) p[1] * image_factor) + g_value;
                p[2] = ((float) p[2] * image_factor) + b_value;
                p[3] = ((float) p[3] * image_factor) + a_value;
                p += 4;
            }
        } else { // mlt_image_rgba
            uint8_t *p = *image;
            for (int i = 0; i < pixels; i++) {
                p[0] = ((float) p[0] * image_factor) + r_value;
                p[1] = ((float) p[1] * image_factor) + g_value;
                p[2] = ((float) p[2] * image_factor) + b_value;
                p[3] = ((float) p[3] * image_factor) + a_value;
                p += 4;
            }
        }
    }

    return error;
}

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    int clip_position = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame),
                                               "meta.playlist.clip_position");
    int clip_length = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame),
                                             "meta.playlist.clip_length");
    int fade_duration = mlt_properties_get_int(filter_properties, "fade_duration");
    double fps = mlt_profile_fps(mlt_service_profile(MLT_FILTER_SERVICE(filter)));
    int ms_from_begining = (double) clip_position * 1000.0 / fps;
    int ms_from_end = (double) (clip_length - clip_position - 1) * 1000.0 / fps;
    int fade = 0;

    if (ms_from_begining <= fade_duration) {
        fade = 1;
        mlt_properties_set_int(filter_properties,
                               "fade_in_count",
                               mlt_properties_get_int(filter_properties, "fade_in_count") + 1);
    } else if (ms_from_end <= fade_duration) {
        fade = 1;
        mlt_properties_set_int(filter_properties,
                               "fade_out_count",
                               mlt_properties_get_int(filter_properties, "fade_out_count") + 1);
    }

    if (fade && mlt_properties_get_int(filter_properties, "fade_audio")) {
        mlt_frame_push_audio(frame, filter);
        mlt_frame_push_audio(frame, filter_get_audio);
    }
    if (fade && mlt_properties_get_int(filter_properties, "fade_video")) {
        mlt_frame_push_get_image(frame, (void *) filter);
        mlt_frame_push_get_image(frame, filter_get_image);
    }
    return frame;
}

static void filter_close(mlt_filter filter)
{
    filter->child = NULL;
    filter->close = NULL;
    filter->parent.close = NULL;
    mlt_service_close(&filter->parent);
}

mlt_filter filter_autofade_init(mlt_profile profile,
                                mlt_service_type type,
                                const char *id,
                                char *arg)
{
    mlt_filter filter = mlt_filter_new();

    if (filter) {
        filter->close = filter_close;
        filter->process = filter_process;
        mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set_int(filter_properties, "fade_duration", 20);
        mlt_properties_set_int(filter_properties, "fade_audio", 1);
        mlt_properties_set_int(filter_properties, "fade_video", 0);
        mlt_properties_set_string(filter_properties, "fade_color", "0x000000ff");
    }

    return filter;
}
