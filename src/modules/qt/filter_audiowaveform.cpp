/*
 * filter_audiowaveform.cpp -- audio waveform visualization filter
 * Copyright (c) 2015-2025 Meltytech, LLC
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
#include "graph.h"
#include <framework/mlt.h>
#include <framework/mlt_log.h>
#include <QImage>
#include <QPainter>
#include <QVector>

static const qreal MAX_S16_AMPLITUDE = 32768.0;

namespace {
// Private Types
typedef struct
{
    char *buffer_prop_name;
    int reset_window;
    int16_t *window_buffer;
    int window_samples;
    int window_frequency;
    int window_channels;
} private_data;

typedef struct
{
    int16_t *buffer;
    int samples;
    int channels;
} save_buffer;

} // namespace

static save_buffer *create_save_buffer(int samples, int channels, int16_t *buffer)
{
    save_buffer *ret = (save_buffer *) calloc(1, sizeof(save_buffer));
    int buffer_size = samples * channels * sizeof(int16_t);
    ret->samples = samples;
    ret->channels = channels;
    ret->buffer = (int16_t *) calloc(1, buffer_size);
    memcpy(ret->buffer, buffer, buffer_size);
    return ret;
}

static void destory_save_buffer(void *ptr)
{
    if (!ptr) {
        mlt_log_error(NULL, "Invalid save_buffer ptr.\n");
        return;
    }
    save_buffer *buff = (save_buffer *) ptr;
    free(buff->buffer);
    free(buff);
}

static void property_changed(mlt_service owner, mlt_filter filter, mlt_event_data event_data)
{
    const char *name = mlt_event_data_to_string(event_data);
    if (name && !strcmp(name, "window")) {
        private_data *pdata = (private_data *) filter->child;
        pdata->reset_window = 1;
    }
}

static int filter_get_audio(mlt_frame frame,
                            void **buffer,
                            mlt_audio_format *format,
                            int *frequency,
                            int *channels,
                            int *samples)
{
    int error = 0;
    mlt_filter filter = (mlt_filter) mlt_frame_pop_audio(frame);
    private_data *pdata = (private_data *) filter->child;

    if (*format != mlt_audio_s16 && *format != mlt_audio_float) {
        *format = mlt_audio_float;
    }

    error = mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    if (error) {
        return error;
    }

    if (*frequency != pdata->window_frequency || *channels != pdata->window_channels) {
        pdata->reset_window = true;
    }

    if (pdata->reset_window) {
        mlt_log_info(MLT_FILTER_SERVICE(filter),
                     "Reset window buffer: %d.\n",
                     mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "window"));
        mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
        double fps = mlt_profile_fps(profile);
        int frame_samples = mlt_audio_calculate_frame_samples(fps,
                                                              *frequency,
                                                              mlt_frame_get_position(frame));
        int window_ms = mlt_properties_get_int(MLT_FILTER_PROPERTIES(filter), "window");
        pdata->window_frequency = *frequency;
        pdata->window_channels = *channels;
        pdata->window_samples = window_ms * *frequency / 1000;
        if (pdata->window_samples < frame_samples) {
            pdata->window_samples = frame_samples;
        }
        free(pdata->window_buffer);
        pdata->window_buffer = (int16_t *) calloc(1,
                                                  pdata->window_samples * pdata->window_channels
                                                      * sizeof(int16_t));
        pdata->reset_window = 0;
    }

    int new_sample_count = *samples;
    if (new_sample_count > pdata->window_samples) {
        new_sample_count = pdata->window_samples;
    }
    int old_sample_count = pdata->window_samples - new_sample_count;
    int window_buff_bytes = pdata->window_samples * pdata->window_channels * sizeof(int16_t);
    int new_sample_bytes = new_sample_count * pdata->window_channels * sizeof(int16_t);
    int old_sample_bytes = old_sample_count * pdata->window_channels * sizeof(int16_t);

    // Move the old samples ahead in the window buffer to make room for new samples.
    if (new_sample_bytes < window_buff_bytes) {
        char *old_sample_src = (char *) pdata->window_buffer + new_sample_bytes;
        char *old_sample_dst = (char *) pdata->window_buffer;
        memmove(old_sample_dst, old_sample_src, old_sample_bytes);
    }

    // Copy the new samples to the back of the window buffer.
    if (*format == mlt_audio_s16) {
        char *new_sample_src = (char *) *buffer;
        char *new_sample_dst = (char *) pdata->window_buffer + old_sample_bytes;
        memcpy(new_sample_dst, new_sample_src, new_sample_bytes);
    } else // mlt_audio_float
    {
        for (int c = 0; c < pdata->window_channels; c++) {
            float *src = (float *) *buffer + (*samples * c);
            int16_t *dst = pdata->window_buffer + (old_sample_count * pdata->window_channels) + c;
            for (int s = 0; s < new_sample_count; s++) {
                *dst = *src * MAX_S16_AMPLITUDE;
                src++;
                dst += pdata->window_channels;
            }
        }
    }

    // Copy the window buffer and pass it along with the frame.
    save_buffer *out = create_save_buffer(pdata->window_samples,
                                          pdata->window_channels,
                                          pdata->window_buffer);
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(frame),
                            pdata->buffer_prop_name,
                            out,
                            sizeof(save_buffer),
                            destory_save_buffer,
                            NULL);

    return 0;
}

static void paint_waveform(
    QPainter &p, QRectF &rect, int16_t *audio, int samples, int channels, int fill)
{
    int width = rect.width();
    const int16_t *q = audio;

    qreal half_height = rect.height() / 2.0;
    qreal center_y = rect.y() + half_height;

    if (samples < width) {
        // For each x position on the waveform, find the sample value that
        // applies to that position and draw a point at that location.
        QPoint point(0, *q * half_height / MAX_S16_AMPLITUDE + center_y);
        QPoint lastPoint = point;
        int lastSample = 0;
        for (int x = 0; x < width; x++) {
            int sample = (x * samples) / width;
            if (sample != lastSample) {
                lastSample = sample;
                q += channels;
            }

            lastPoint.setX(x + rect.x());
            lastPoint.setY(point.y());
            point.setX(x + rect.x());
            point.setY(*q * half_height / MAX_S16_AMPLITUDE + center_y);

            if (fill) {
                // Draw the line all the way to 0 to "fill" it in.
                if ((point.y() > center_y && lastPoint.y() > center_y)
                    || (point.y() < center_y && lastPoint.y() < center_y)) {
                    lastPoint.setY(center_y);
                }
            }

            if (point.y() == lastPoint.y()) {
                p.drawPoint(point);
            } else {
                p.drawLine(lastPoint, point);
            }
        }
    } else {
        // For each x position on the waveform, find the min and max sample
        // values that apply to that position. Draw a vertical line from the
        // min value to the max value.
        QPoint high;
        QPoint low;
        qreal max = *q;
        qreal min = *q;
        int lastX = 0;
        for (int s = 0; s <= samples; s++) {
            int x = (s * width) / samples;
            if (x != lastX) {
                // The min and max have been determined for the previous x
                // So draw the line

                if (fill) {
                    // Draw the line all the way to 0 to "fill" it in.
                    if (max > 0 && min > 0) {
                        min = 0;
                    } else if (min < 0 && max < 0) {
                        max = 0;
                    }
                }

                high.setX(lastX + rect.x());
                high.setY(max * half_height / MAX_S16_AMPLITUDE + center_y);
                low.setX(lastX + rect.x());
                low.setY(min * half_height / MAX_S16_AMPLITUDE + center_y);

                if (high.y() == low.y()) {
                    p.drawPoint(high);
                } else {
                    p.drawLine(low, high);
                }
                lastX = x;
                // Swap max and min so that the next line picks up where
                // this one left off.
                int tmp = max;
                max = min;
                min = tmp;
            }
            if (*q > max)
                max = *q;
            if (*q < min)
                min = *q;
            q += channels;
        }
    }
}

static void draw_waveforms(mlt_filter filter,
                           mlt_frame frame,
                           QImage *qimg,
                           int16_t *audio,
                           int channels,
                           int samples,
                           int width,
                           int height)
{
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
    mlt_position position = mlt_filter_get_position(filter, frame);
    mlt_position length = mlt_filter_get_length2(filter, frame);
    mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
    int show_channel
        = mlt_properties_anim_get_int(filter_properties, "show_channel", position, length);
    int fill = mlt_properties_get_int(filter_properties, "fill");
    mlt_rect rect = mlt_properties_anim_get_rect(filter_properties, "rect", position, length);
    if (strchr(mlt_properties_get(filter_properties, "rect"), '%')) {
        rect.x *= qimg->width();
        rect.w *= qimg->width();
        rect.y *= qimg->height();
        rect.h *= qimg->height();
    }
    double scale = mlt_profile_scale_width(profile, width);
    rect.x *= scale;
    rect.w *= scale;
    scale = mlt_profile_scale_height(profile, height);
    rect.y *= scale;
    rect.h *= scale;

    QRectF r(rect.x, rect.y, rect.w, rect.h);

    QPainter p(qimg);

    setup_graph_painter(p, r, filter_properties, position, length);

    if (show_channel == -1) // Combine all channels
    {
        if (channels > 1) {
            int16_t *in = audio;
            int16_t *out = audio;
            for (int s = 0; s < samples; s++) {
                double acc = 0.0;
                for (int c = 0; c < channels; c++) {
                    acc += *in++;
                }
                *out++ = acc / channels;
            }
            channels = 1;
        }
        show_channel = 1;
    }

    if (show_channel == 0) // Show all channels
    {
        QRectF c_rect = r;
        qreal c_height = r.height() / channels;
        for (int c = 0; c < channels; c++) {
            // Divide the rectangle into smaller rectangles for each channel.
            c_rect.setY(r.y() + c_height * c);
            c_rect.setHeight(c_height);
            setup_graph_pen(p, c_rect, filter_properties, scale, position, length);
            paint_waveform(p, c_rect, audio + c, samples, channels, fill);
        }
    } else if (show_channel > 0) { // Show one specific channel
        if (show_channel > channels) {
            // Sanity
            show_channel = 1;
        }
        setup_graph_pen(p, r, filter_properties, scale, position, length);
        paint_waveform(p, r, audio + show_channel - 1, samples, channels, fill);
    }

    p.end();
}

static int filter_get_image(mlt_frame frame,
                            uint8_t **image,
                            mlt_image_format *image_format,
                            int *width,
                            int *height,
                            int writable)
{
    int error = 0;
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_filter filter = (mlt_filter) mlt_frame_pop_service(frame);
    private_data *pdata = (private_data *) filter->child;
    save_buffer *audio = (save_buffer *) mlt_properties_get_data(frame_properties,
                                                                 pdata->buffer_prop_name,
                                                                 NULL);

    if (audio) {
        // Get the current image
        *image_format = choose_image_format(*image_format);
        error = mlt_frame_get_image(frame, image, image_format, width, height, writable);

        // Draw the waveforms
        if (!error) {
            QImage qimg(*width, *height, QImage::Format_ARGB32);
            convert_mlt_to_qimage(*image, &qimg, *width, *height, *image_format);
            draw_waveforms(filter,
                           frame,
                           &qimg,
                           audio->buffer,
                           audio->channels,
                           audio->samples,
                           *width,
                           *height);
            convert_qimage_to_mlt(&qimg, *image, *width, *height);
        }
    } else {
        // This filter depends on the consumer processing the audio before
        // the video.
        mlt_log_warning(MLT_FILTER_SERVICE(filter), "Audio not preprocessed.\n");
        return mlt_frame_get_image(frame, image, image_format, width, height, writable);
    }

    return error;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter filter, mlt_frame frame)
{
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    if (mlt_frame_is_test_card(frame)) {
        // The producer does not generate video. This filter will create an
        // image on the producer's behalf.
        mlt_profile profile = mlt_service_profile(MLT_FILTER_SERVICE(filter));
        mlt_properties_set_int(frame_properties, "progressive", 1);
        mlt_properties_set_double(frame_properties, "aspect_ratio", mlt_profile_sar(profile));
        mlt_properties_set_int(frame_properties, "meta.media.width", profile->width);
        mlt_properties_set_int(frame_properties, "meta.media.height", profile->height);
        // Tell the framework that there really is an image.
        mlt_properties_set_int(frame_properties, "test_image", 0);
        // Push a callback to create the image.
        mlt_frame_push_get_image(frame, create_image);
    }
    mlt_frame_push_audio(frame, filter);
    mlt_frame_push_audio(frame, (void *) filter_get_audio);
    mlt_frame_push_service(frame, filter);
    mlt_frame_push_get_image(frame, filter_get_image);

    return frame;
}

static void filter_close(mlt_filter filter)
{
    private_data *pdata = (private_data *) filter->child;

    if (pdata) {
        free(pdata->window_buffer);
        free(pdata->buffer_prop_name);
        free(pdata);
    }
    filter->child = NULL;
    filter->close = NULL;
    filter->parent.close = NULL;
    mlt_service_close(&filter->parent);
}

/** Constructor for the filter.
*/

extern "C" {

mlt_filter filter_audiowaveform_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *arg)
{
    mlt_filter filter = mlt_filter_new();
    private_data *pdata = (private_data *) calloc(1, sizeof(private_data));

    if (filter && pdata) {
        if (!createQApplicationIfNeeded(MLT_FILTER_SERVICE(filter))) {
            mlt_filter_close(filter);
            return NULL;
        }

        mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
        mlt_properties_set(filter_properties, "bgcolor", "0x00000000");
        mlt_properties_set(filter_properties, "color.1", "0xffffffff");
        mlt_properties_set(filter_properties, "thickness", "0");
        mlt_properties_set(filter_properties, "show_channel", "0");
        mlt_properties_set(filter_properties, "angle", "0");
        mlt_properties_set(filter_properties, "rect", "0 0 100% 100%");
        mlt_properties_set(filter_properties, "fill", "0");
        mlt_properties_set(filter_properties, "gorient", "v");
        mlt_properties_set_int(filter_properties, "window", 0);

        pdata->reset_window = 1;
        // Create a unique ID for storing data on the frame
        pdata->buffer_prop_name = (char *) calloc(1, 20);
        snprintf(pdata->buffer_prop_name, 20, "audiowave.%p", filter);
        pdata->buffer_prop_name[20 - 1] = '\0';

        filter->close = filter_close;
        filter->process = filter_process;
        filter->child = pdata;

        mlt_events_listen(filter_properties,
                          filter,
                          "property-changed",
                          (mlt_listener) property_changed);
    } else {
        mlt_log_error(MLT_FILTER_SERVICE(filter), "Failed to initialize\n");

        if (filter) {
            mlt_filter_close(filter);
        }

        if (pdata) {
            free(pdata);
        }

        filter = NULL;
    }

    return filter;
}
}
