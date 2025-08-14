/*
 * link_timeremap.c
 * Copyright (C) 2020-2025 Meltytech, LLC
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

#include <framework/mlt_factory.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_link.h>
#include <framework/mlt_log.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Private Types
typedef struct
{
    mlt_position prev_integration_position;
    double prev_integration_time;
    mlt_frame prev_frame;
    mlt_filter resample_filter;
    mlt_filter pitch_filter;
} private_data;

static void property_changed(mlt_service owner, mlt_link self, mlt_event_data event_data)
{
    const char *name = mlt_event_data_to_string(event_data);

    if (!name)
        return;

    if (strcmp("map", name) == 0) {
        // Copy the deprecated "map" parameter to the new "time_map" parameter.
        const char *value = mlt_properties_get(MLT_LINK_PROPERTIES(self), "map");
        mlt_properties_set(MLT_LINK_PROPERTIES(self), "time_map", value);
    } else if (strcmp("speed_map", name) == 0) {
        // speed_map changed. Need to re-integrate from the beginning.
        private_data *pdata = (private_data *) self->child;
        pdata->prev_integration_position = 0;
        pdata->prev_integration_time = 0.0;
    }
}

static double integrate_source_time(mlt_link self, mlt_position position)
{
    private_data *pdata = (private_data *) self->child;
    mlt_properties properties = MLT_LINK_PROPERTIES(self);
    mlt_position length = mlt_producer_get_length(MLT_LINK_PRODUCER(self));
    mlt_position in = mlt_producer_get_in(MLT_LINK_PRODUCER(self));
    double link_fps = mlt_producer_get_fps(MLT_LINK_PRODUCER(self));
    mlt_position integration_distance = abs(pdata->prev_integration_position - position);

    if (pdata->prev_integration_position < in || integration_distance > (position - in)) {
        // Restart integration from the beginning
        pdata->prev_integration_position = in;
        pdata->prev_integration_time = 0.0;
    }

    double source_time = pdata->prev_integration_time;

    if (pdata->prev_integration_position < position) {
        for (mlt_position p = pdata->prev_integration_position; p < position; p++) {
            double speed = mlt_properties_anim_get_double(properties, "speed_map", p - in, length);
            double time_delta = speed / link_fps;
            source_time += time_delta;
        }
    } else if (pdata->prev_integration_position > position) {
        for (mlt_position p = position; p < pdata->prev_integration_position; p++) {
            double speed = mlt_properties_anim_get_double(properties, "speed_map", p - in, length);
            double time_delta = speed / link_fps;
            source_time -= time_delta;
        }
    }

    // Remember the integration results so that the next frame can start
    // integration from this position instead of from the beginning.
    pdata->prev_integration_position = position;
    pdata->prev_integration_time = source_time;
    return source_time;
}

static void link_configure(mlt_link self, mlt_profile chain_profile)
{
    // Timeremap must always work with the same profile as the chain so that
    // animation, in and out will work.
    mlt_service_set_profile(MLT_LINK_SERVICE(self), chain_profile);
}

static void change_movit_format(mlt_frame frame, mlt_frame src_frame, mlt_image_format *format)
{
    if (*format == mlt_image_movit) {
        const mlt_image_format src_format = mlt_properties_get_int(MLT_FRAME_PROPERTIES(src_frame),
                                                                   "format");
        // First priority, preserve the alpha channel in the source
        if (src_format == mlt_image_rgba) {
            // Producers typically set frame property "format" to rgba in their get_frame callback
            *format = mlt_image_rgba;
        } else {
            // Use a 10-bit output if the end consumer wants it.
            // TODO add a "consumer.format" property to find other criteria for 10-bit
            const char *trc = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "consumer.color_trc");
            *format = (trc && !strcmp("arib-std-b67", trc)) ? mlt_image_yuv444p10 : mlt_image_rgba;
        }
    }
}

static int link_get_audio(mlt_frame frame,
                          void **audio,
                          mlt_audio_format *format,
                          int *frequency,
                          int *channels,
                          int *samples)
{
    int requested_frequency = *frequency;
    int requested_samples = *samples;
    mlt_link self = (mlt_link) mlt_frame_pop_audio(frame);
    private_data *pdata = (private_data *) self->child;
    mlt_properties unique_properties = mlt_frame_get_unique_properties(frame,
                                                                       MLT_LINK_SERVICE(self));
    if (!unique_properties) {
        return 1;
    }
    double source_time = mlt_properties_get_double(unique_properties, "source_time");
    double source_duration = mlt_properties_get_double(unique_properties, "source_duration");
    double source_fps = mlt_properties_get_double(unique_properties, "source_fps");
    double source_speed = mlt_properties_get_double(unique_properties, "source_speed");
    source_speed = fabs(source_speed);
    double link_fps = mlt_producer_get_fps(MLT_LINK_PRODUCER(self));

    // Validate the request
    *channels = *channels <= 0 ? 2 : *channels;
    *frequency = *frequency <= 0 ? 48000 : *frequency;
    *format = *format == mlt_audio_none ? mlt_audio_float : *format;

    if (source_speed < 0.1 || source_speed > 10) {
        // Return silent samples for speeds less than 0.1 or > 10
        mlt_position position = mlt_frame_original_position(frame);
        *samples = mlt_audio_calculate_frame_samples(link_fps, *frequency, position);
        int size = mlt_audio_format_size(*format, *samples, *channels);
        *audio = mlt_pool_alloc(size);
        memset(*audio, 0, size);
        mlt_frame_set_audio(frame, *audio, *format, size, mlt_pool_release);
        return 0;
    }

    int src_frequency = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), "audio_frequency");
    if (src_frequency > 0) {
        *frequency = src_frequency;
    }

    // Calculate the samples to get from the input frames
    int link_sample_count = mlt_audio_calculate_frame_samples(link_fps,
                                                              *frequency,
                                                              mlt_frame_get_position(frame));
    int sample_count = lrint((double) link_sample_count * source_speed);
    mlt_position in_frame_pos = floor(source_time * source_fps);
    int64_t first_out_sample = llrint(source_time * (double) *frequency);

    // Attempt to maintain sample continuity with the previous frame
    static const int64_t SAMPLE_CONTINUITY_ERROR_MARGIN = 4;
    int64_t continuity_sample = mlt_properties_get_int64(MLT_LINK_PROPERTIES(self),
                                                         "_continuity_sample");
    int64_t continuity_delta = continuity_sample - first_out_sample;
    if (source_duration > 0.0 && continuity_delta != 0) {
        // Forward: Continue from where the previous frame left off if within the margin of error
        if (continuity_delta > -SAMPLE_CONTINUITY_ERROR_MARGIN
            && continuity_delta < SAMPLE_CONTINUITY_ERROR_MARGIN) {
            sample_count = sample_count - continuity_delta;
            first_out_sample = continuity_sample;
            mlt_log_debug(MLT_LINK_SERVICE(self),
                          "Maintain Forward Continuity: %d\n",
                          (int) continuity_delta);
        }
    } else if (source_duration < 0.0 && continuity_delta != sample_count) {
        // Reverse: End where the previous frame left off if within the margin of error
        continuity_delta -= sample_count;
        if (continuity_delta > -SAMPLE_CONTINUITY_ERROR_MARGIN
            && continuity_delta < SAMPLE_CONTINUITY_ERROR_MARGIN) {
            sample_count = sample_count + continuity_delta;
            mlt_log_debug(MLT_LINK_SERVICE(self),
                          "Maintain Reverse Continuity: %d\n",
                          (int) continuity_delta);
        }
    }

    int64_t first_in_sample = mlt_audio_calculate_samples_to_position(source_fps,
                                                                      *frequency,
                                                                      in_frame_pos);
    int samples_to_skip = first_out_sample - first_in_sample;
    if (samples_to_skip < 0) {
        mlt_log_error(MLT_LINK_SERVICE(self),
                      "Audio too late: %d\t%d\n",
                      (int) first_out_sample,
                      (int) first_in_sample);
        samples_to_skip = 0;
    }

    // Allocate the out buffer
    struct mlt_audio_s out;
    mlt_audio_set_values(&out, NULL, *frequency, *format, sample_count, *channels);
    mlt_audio_alloc_data(&out);

    // Copy audio from the input frames to the output buffer
    int samples_copied = 0;
    int samples_needed = sample_count;

    while (samples_needed > 0) {
        char key[19];
        sprintf(key, "%d", in_frame_pos);
        mlt_frame src_frame = (mlt_frame) mlt_properties_get_data(unique_properties, key, NULL);
        if (!src_frame) {
            mlt_log_error(MLT_LINK_SERVICE(self), "Frame not found: %d\n", in_frame_pos);
            break;
        }

        int in_samples = mlt_audio_calculate_frame_samples(source_fps, *frequency, in_frame_pos);
        struct mlt_audio_s in;
        mlt_audio_set_values(&in, NULL, *frequency, *format, in_samples, *channels);

        int error = mlt_frame_get_audio(src_frame,
                                        &in.data,
                                        &in.format,
                                        &in.frequency,
                                        &in.channels,
                                        &in.samples);
        if (error) {
            mlt_log_error(MLT_LINK_SERVICE(self), "No audio: %d\n", in_frame_pos);
            break;
        }

        if (in.format == mlt_audio_none) {
            mlt_log_error(MLT_LINK_SERVICE(self), "Audio none: %d\n", in_frame_pos);
            error = 1;
            break;
        }

        int samples_to_copy = in.samples - samples_to_skip;
        if (samples_to_copy > samples_needed) {
            samples_to_copy = samples_needed;
        }
        mlt_log_debug(MLT_LINK_SERVICE(self),
                      "Copy: %d\t%d\t%d\t%d\n",
                      samples_to_skip,
                      samples_to_skip + samples_to_copy - 1,
                      samples_to_copy,
                      in.samples);

        if (samples_to_copy > 0) {
            mlt_audio_copy(&out, &in, samples_to_copy, samples_to_skip, samples_copied);
            samples_copied += samples_to_copy;
            samples_needed -= samples_to_copy;
        }

        samples_to_skip = 0;
        in_frame_pos++;
    }

    if (samples_copied != sample_count) {
        mlt_log_error(MLT_LINK_SERVICE(self),
                      "Sample under run: %d\t%d\n",
                      samples_copied,
                      sample_count);
        mlt_audio_shrink(&out, samples_copied);
    }

    if (source_duration < 0.0) {
        // Going backwards
        mlt_audio_reverse(&out);
        mlt_properties_set_int64(MLT_LINK_PROPERTIES(self), "_continuity_sample", first_out_sample);
    } else {
        mlt_properties_set_int64(MLT_LINK_PROPERTIES(self),
                                 "_continuity_sample",
                                 first_out_sample + sample_count);
    }

    int in_frequency = *frequency;
    out.frequency = lrint((double) out.frequency * (double) sample_count
                          / (double) link_sample_count);
    mlt_frame_set_audio(frame, out.data, out.format, 0, out.release_data);
    mlt_audio_get_values(&out, audio, frequency, format, samples, channels);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "audio_frequency", *frequency);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "audio_channels", *channels);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "audio_samples", *samples);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "audio_format", *format);

    // Apply pitch compensation if requested
    int pitch_compensate = mlt_properties_get_int(MLT_LINK_PROPERTIES(self), "pitch");
    if (pitch_compensate) {
        if (!pdata->pitch_filter) {
            pdata->pitch_filter = mlt_factory_filter(mlt_service_profile(MLT_LINK_SERVICE(self)),
                                                     "rbpitch",
                                                     NULL);
        }
        if (pdata->pitch_filter) {
            mlt_properties_set_int(MLT_FILTER_PROPERTIES(pdata->pitch_filter), "stretch", 1);
            // Set the pitchscale to compensate for the difference between the input sampling frequency and the requested sampling frequency.
            double pitchscale = (double) in_frequency / (double) requested_frequency;
            mlt_properties_set_double(MLT_FILTER_PROPERTIES(pdata->pitch_filter),
                                      "pitchscale",
                                      pitchscale);
            mlt_filter_process(pdata->pitch_filter, frame);
        }
    }
    // Apply a resampler if rbpitch is not stretching to provide
    if (!pitch_compensate || !pdata->pitch_filter) {
        if (!pdata->resample_filter) {
            pdata->resample_filter = mlt_factory_filter(mlt_service_profile(MLT_LINK_SERVICE(self)),
                                                        "resample",
                                                        NULL);
            if (!pdata->resample_filter) {
                pdata->resample_filter = mlt_factory_filter(mlt_service_profile(
                                                                MLT_LINK_SERVICE(self)),
                                                            "swresample",
                                                            NULL);
            }
        }
        if (pdata->resample_filter) {
            mlt_filter_process(pdata->resample_filter, frame);
        }
    }

    // Final call to get_audio() to apply the pitch or resample filter
    *frequency = requested_frequency;
    *samples = requested_samples;
    int error = mlt_frame_get_audio(frame, audio, format, frequency, channels, samples);

    return error;
}

#define MAX_BLEND_IMAGES 10

static int link_get_image_blend(mlt_frame frame,
                                uint8_t **image,
                                mlt_image_format *format,
                                int *width,
                                int *height,
                                int writable)
{
    mlt_link self = (mlt_link) mlt_frame_pop_get_image(frame);
    mlt_properties unique_properties = mlt_frame_get_unique_properties(frame,
                                                                       MLT_LINK_SERVICE(self));
    if (!unique_properties) {
        return 1;
    }
    int requested_width = *width;
    int requested_height = *height;
    double source_time = mlt_properties_get_double(unique_properties, "source_time");
    double source_fps = mlt_properties_get_double(unique_properties, "source_fps");

    // Get pointers to all the images for this frame
    uint8_t *images[MAX_BLEND_IMAGES];
    int image_count = 0;
    mlt_position in_frame_pos = floor(source_time * source_fps);
    char key[19];
    sprintf(key, "%d", in_frame_pos);
    mlt_frame src_frame = mlt_properties_get_data(unique_properties, key, NULL);

    while (src_frame && image_count < MAX_BLEND_IMAGES) {
        mlt_service_lock(MLT_LINK_SERVICE(self));

        mlt_properties_pass_list(MLT_FRAME_PROPERTIES(src_frame),
                                 MLT_FRAME_PROPERTIES(frame),
                                 "crop.left crop.right crop.top crop.bottom crop.original_width "
                                 "crop.original_height meta.media.width meta.media.height");
        mlt_properties_copy(MLT_FRAME_PROPERTIES(src_frame),
                            MLT_FRAME_PROPERTIES(frame),
                            "consumer.");

        change_movit_format(frame, src_frame, format);

        int error = mlt_frame_get_image(src_frame,
                                        &images[image_count],
                                        format,
                                        &requested_width,
                                        &requested_height,
                                        0);
        mlt_service_unlock(MLT_LINK_SERVICE(self));
        if (error) {
            mlt_log_error(MLT_LINK_SERVICE(self), "Failed to get image %s\n", key);
            break;
        }
        if (*width != requested_width || *height != requested_height) {
            mlt_log_error(MLT_LINK_SERVICE(self),
                          "Dimension Mismatch (%s): %dx%d != %dx%d\n",
                          key,
                          requested_width,
                          requested_height,
                          *width,
                          *height);
            break;
        }
        in_frame_pos++;
        image_count++;
    }

    if (image_count <= 0) {
        mlt_log_error(MLT_LINK_SERVICE(self), "No images to blend\n");
        return 1;
    }

    // Sum all the images into one image with 16 bit components
    int size = mlt_image_format_size(*format, *width, *height, NULL);
    *image = mlt_pool_alloc(size);
    int s = 0;
    uint8_t *p = *image;
    for (s = 0; s < size; s++) {
        int16_t sum = 0;
        int i = 0;
        for (i = 0; i < image_count; i++) {
            sum += *(images[i]);
            images[i]++;
        }
        *p = sum / image_count;
        p++;
    }
    mlt_frame_set_image(frame, *image, size, mlt_pool_release);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "format", *format);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "width", *width);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "height", *height);
    mlt_properties_pass_list(MLT_FRAME_PROPERTIES(frame),
                             MLT_FRAME_PROPERTIES(src_frame),
                             "colorspace color_primaries color_trc full_range");

    return 0;
}

static int link_get_image_nearest(mlt_frame frame,
                                  uint8_t **image,
                                  mlt_image_format *format,
                                  int *width,
                                  int *height,
                                  int writable)
{
    mlt_link self = (mlt_link) mlt_frame_pop_get_image(frame);
    mlt_properties unique_properties = mlt_frame_get_unique_properties(frame,
                                                                       MLT_LINK_SERVICE(self));
    if (!unique_properties) {
        return 1;
    }
    double source_time = mlt_properties_get_double(unique_properties, "source_time");
    double source_fps = mlt_properties_get_double(unique_properties, "source_fps");
    mlt_position in_frame_pos = floor(source_time * source_fps);
    char key[19];
    sprintf(key, "%d", in_frame_pos);

    mlt_frame src_frame = (mlt_frame) mlt_properties_get_data(unique_properties, key, NULL);
    if (src_frame) {
        uint8_t *in_image;
        mlt_service_lock(MLT_LINK_SERVICE(self));

        mlt_properties_pass_list(MLT_FRAME_PROPERTIES(src_frame),
                                 MLT_FRAME_PROPERTIES(frame),
                                 "crop.left crop.right crop.top crop.bottom crop.original_width "
                                 "crop.original_height meta.media.width meta.media.height");
        mlt_properties_copy(MLT_FRAME_PROPERTIES(src_frame),
                            MLT_FRAME_PROPERTIES(frame),
                            "consumer.");

        change_movit_format(frame, src_frame, format);

        int error = mlt_frame_get_image(src_frame, &in_image, format, width, height, 0);
        mlt_service_unlock(MLT_LINK_SERVICE(self));
        if (!error) {
            int size = mlt_image_format_size(*format, *width, *height, NULL);
            *image = mlt_pool_alloc(size);
            memcpy(*image, in_image, size);
            mlt_frame_set_image(frame, *image, size, mlt_pool_release);
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "format", *format);
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "width", *width);
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(frame), "height", *height);
            mlt_properties_pass_list(MLT_FRAME_PROPERTIES(frame),
                                     MLT_FRAME_PROPERTIES(src_frame),
                                     "colorspace color_primaries color_trc full_range");
            uint8_t *in_alpha = mlt_frame_get_alpha(src_frame);
            if (in_alpha) {
                size = *width * *height;
                uint8_t *out_alpha = mlt_pool_alloc(size);
                memcpy(out_alpha, in_alpha, size);
                mlt_frame_set_alpha(frame, out_alpha, size, mlt_pool_release);
            };
            return 0;
        }
    }

    return 1;
}

static int link_get_frame(mlt_link self, mlt_frame_ptr frame, int index)
{
    mlt_properties properties = MLT_LINK_PROPERTIES(self);
    private_data *pdata = (private_data *) self->child;
    mlt_position position = mlt_producer_position(MLT_LINK_PRODUCER(self));
    mlt_position length = mlt_producer_get_length(MLT_LINK_PRODUCER(self));
    double source_time = 0.0;
    double source_duration = 0.0;
    double source_fps = mlt_producer_get_fps(self->next);
    double link_fps = mlt_producer_get_fps(MLT_LINK_PRODUCER(self));
    // Assume that the user wants normal speed before the in point.
    mlt_position in = mlt_producer_get_in(MLT_LINK_PRODUCER(self));
    double in_time = (double) in / link_fps;
    int result = 0;

    // Create a frame
    *frame = mlt_frame_init(MLT_LINK_SERVICE(self));
    mlt_frame_set_position(*frame, mlt_producer_position(MLT_LINK_PRODUCER(self)));
    mlt_properties unique_properties = mlt_frame_unique_properties(*frame, MLT_LINK_SERVICE(self));

    // Calculate the frames from the next link to be used
    if (mlt_properties_exists(properties, "speed_map")) {
        // Speed mapping
        source_time = integrate_source_time(self, position) + in_time;
        double next_source_time = integrate_source_time(self, position + 1) + in_time;
        source_duration = next_source_time - source_time;
    } else if (mlt_properties_exists(properties, "time_map")) {
        source_time = mlt_properties_anim_get_double(properties, "time_map", position - in, length)
                      + in_time;
        double next_source_time
            = mlt_properties_anim_get_double(properties, "time_map", position - in + 1, length)
              + in_time;
        source_duration = next_source_time - source_time;
    } else {
        // No mapping specified. Assume 1.0 speed.
        // This can be used as a frame rate converter.
        source_time = (double) position / link_fps;
        source_duration = 1.0 / link_fps;
    }

    double frame_duration = 1.0 / link_fps;
    double source_speed = 0.0;
    if (source_duration != 0.0) {
        source_speed = source_duration / frame_duration;
    }

    mlt_properties_set_double(unique_properties, "source_fps", source_fps);
    mlt_properties_set_double(unique_properties, "source_time", source_time);
    mlt_properties_set_double(unique_properties, "source_duration", source_duration);
    mlt_properties_set_double(unique_properties, "source_speed", source_speed);

    mlt_log_debug(MLT_LINK_SERVICE(self),
                  "Get Frame: %f -> %f\t%d\t%d\n",
                  source_fps,
                  link_fps,
                  position,
                  mlt_producer_get_in(MLT_LINK_PRODUCER(self)));

    // Get frames from the next link and pass them along with the new frame
    int in_frame_count = 0;
    mlt_frame src_frame = NULL;
    mlt_position prev_frame_position = pdata->prev_frame ? mlt_frame_get_position(pdata->prev_frame)
                                                         : -1;
    mlt_position in_frame_pos = floor(source_time * source_fps);
    double frame_time = (double) in_frame_pos / source_fps;
    double source_end_time = source_time + fabs(source_duration);
    if (frame_time == source_end_time) {
        // Force one frame to be sent.
        source_end_time += 0.0000000001;
    }
    while (frame_time < source_end_time) {
        if (in_frame_pos == prev_frame_position) {
            // Reuse the previous frame to avoid seeking.
            src_frame = pdata->prev_frame;
            mlt_properties_inc_ref(MLT_FRAME_PROPERTIES(src_frame));
        } else {
            mlt_producer_seek(self->next, in_frame_pos);
            result = mlt_service_get_frame(MLT_PRODUCER_SERVICE(self->next), &src_frame, index);
            if (result) {
                break;
            }
        }
        // Save the source frame on the output frame
        char key[19];
        sprintf(key, "%d", in_frame_pos);
        mlt_properties_set_data(unique_properties,
                                key,
                                src_frame,
                                0,
                                (mlt_destructor) mlt_frame_close,
                                NULL);

        // Calculate the next frame
        in_frame_pos++;
        frame_time = (double) in_frame_pos / source_fps;
        in_frame_count++;
    }

    if (!src_frame) {
        mlt_frame_close(*frame);
        *frame = NULL;
        return 1;
    }

    // Copy some useful properties from one of the source frames.
    (*frame)->convert_image = src_frame->convert_image;
    (*frame)->convert_audio = src_frame->convert_audio;
    mlt_filter cpu_csc = (mlt_filter) mlt_properties_get_data(MLT_FRAME_PROPERTIES(src_frame),
                                                              "_movit cpu_convert",
                                                              NULL);
    if (cpu_csc) {
        mlt_properties_inc_ref(MLT_FILTER_PROPERTIES(cpu_csc));
        mlt_properties_set_data(MLT_FRAME_PROPERTIES(*frame),
                                "_movit cpu_convert",
                                cpu_csc,
                                0,
                                (mlt_destructor) mlt_filter_close,
                                NULL);
    }
    mlt_properties_pass_list(MLT_FRAME_PROPERTIES(*frame),
                             MLT_FRAME_PROPERTIES(src_frame),
                             "audio_frequency");
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(*frame),
                            "_producer",
                            mlt_frame_get_original_producer(src_frame),
                            0,
                            NULL,
                            NULL);

    if (src_frame != pdata->prev_frame) {
        // Save the last source frame because it might be requested for the next frame.
        mlt_frame_close(pdata->prev_frame);
        mlt_properties_inc_ref(MLT_FRAME_PROPERTIES(src_frame));
        pdata->prev_frame = src_frame;
    }

    // Setup callbacks
    char *mode = mlt_properties_get(properties, "image_mode");
    mlt_frame_push_get_image(*frame, (void *) self);
    if (in_frame_count == 1 || !mode || !strcmp(mode, "nearest")) {
        mlt_frame_push_get_image(*frame, link_get_image_nearest);
    } else {
        mlt_frame_push_get_image(*frame, link_get_image_blend);
    }

    mlt_frame_push_audio(*frame, (void *) self);
    mlt_frame_push_audio(*frame, link_get_audio);

    mlt_producer_prepare_next(MLT_LINK_PRODUCER(self));
    mlt_properties_set_double(properties, "speed", source_speed);

    return result;
}

static void link_close(mlt_link self)
{
    if (self) {
        private_data *pdata = (private_data *) self->child;
        if (pdata) {
            mlt_frame_close(pdata->prev_frame);
            mlt_filter_close(pdata->resample_filter);
            mlt_filter_close(pdata->pitch_filter);
            free(pdata);
        }
        self->close = NULL;
        mlt_link_close(self);
        free(self);
    }
}

mlt_link link_timeremap_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_link self = mlt_link_init();
    private_data *pdata = (private_data *) calloc(1, sizeof(private_data));

    if (self && pdata) {
        self->child = pdata;

        // Callback registration
        self->configure = link_configure;
        self->get_frame = link_get_frame;
        self->close = link_close;

        // Signal that this link performs frame rate conversion
        mlt_properties_set_int(MLT_LINK_PROPERTIES(self), "_frc", 1);

        mlt_events_listen(MLT_LINK_PROPERTIES(self),
                          self,
                          "property-changed",
                          (mlt_listener) property_changed);
    } else {
        free(pdata);
        mlt_link_close(self);
        self = NULL;
    }
    return self;
}
