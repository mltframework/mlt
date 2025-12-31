/*
 * producer_xml-clip.c -- A wrapper for mlt XML in native profile
 * Copyright (C) 2024-2025 Meltytech, LLC
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
#include <framework/mlt_log.h>

#include <stdlib.h>

static int producer_get_audio(mlt_frame frame,
                              void **audio,
                              mlt_audio_format *format,
                              int *frequency,
                              int *channels,
                              int *samples)
{
    mlt_producer self = (mlt_producer) mlt_frame_pop_audio(frame);
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_properties unique_properties = mlt_frame_get_unique_properties(frame,
                                                                       MLT_PRODUCER_SERVICE(self));
    mlt_frame xml_frame = (mlt_frame) mlt_properties_get_data(unique_properties, "xml_frame", NULL);
    if (!xml_frame) {
        mlt_log_error(MLT_PRODUCER_SERVICE(self), "XML Frame not found\n");
        return 1;
    }
    mlt_properties xml_frame_properties = MLT_FRAME_PROPERTIES(xml_frame);
    mlt_properties_copy(xml_frame_properties, frame_properties, "consumer.");

    int error = mlt_frame_get_audio(xml_frame, audio, format, frequency, channels, samples);
    if (error) {
        mlt_log_error(MLT_PRODUCER_SERVICE(self), "No audio\n");
        return 1;
    }
    if (*format == mlt_audio_none) {
        mlt_log_error(MLT_LINK_SERVICE(self), "Audio none\n");
        return 1;
    }
    mlt_frame_set_audio(frame, *audio, *format, 0, NULL);
    mlt_properties_set_int(frame_properties, "audio_frequency", *frequency);
    mlt_properties_set_int(frame_properties, "audio_channels", *channels);
    mlt_properties_set_int(frame_properties, "audio_samples", *samples);
    mlt_properties_set_int(frame_properties, "audio_format", *format);
    mlt_properties_pass_property(frame_properties, xml_frame_properties, "channel_layout");
    return 0;
}

static int producer_get_image(mlt_frame frame,
                              uint8_t **image,
                              mlt_image_format *format,
                              int *width,
                              int *height,
                              int writable)
{
    mlt_producer self = mlt_frame_pop_service(frame);
    mlt_profile profile = mlt_service_profile(MLT_PRODUCER_SERVICE(self));
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(frame);
    mlt_properties unique_properties = mlt_frame_get_unique_properties(frame,
                                                                       MLT_PRODUCER_SERVICE(self));
    mlt_frame xml_frame = (mlt_frame) mlt_properties_get_data(unique_properties, "xml_frame", NULL);
    if (!xml_frame) {
        mlt_log_error(MLT_PRODUCER_SERVICE(self), "XML Frame not found\n");
        return 1;
    }
    mlt_properties xml_frame_properties = MLT_FRAME_PROPERTIES(xml_frame);
    mlt_properties_copy(xml_frame_properties, frame_properties, "consumer.");

    // Force the dimension request to match the profile
    *width = profile->width;
    *height = profile->height;

    int error = mlt_frame_get_image(xml_frame, image, format, width, height, writable);
    if (error) {
        mlt_log_error(MLT_PRODUCER_SERVICE(self), "Failed to get image from xml producer\n");
        return error;
    }
    mlt_frame_set_image(frame, *image, 0, NULL);

    int size;
    uint8_t *data = mlt_frame_get_alpha_size(xml_frame, &size);
    if (data) {
        mlt_frame_set_alpha(frame, data, size, NULL);
    }

    mlt_properties_set_int(frame_properties, "format", *format);
    mlt_properties_set_int(frame_properties, "width", *width);
    mlt_properties_set_int(frame_properties, "height", *height);
    mlt_properties_pass_list(
        frame_properties,
        xml_frame_properties,
        "colorspace aspect_ratio progressive full_range top_field_first color_trc color_primaries");

    return error;
}

static int producer_get_frame(mlt_producer self, mlt_frame_ptr frame, int index)
{
    mlt_producer xml_producer = self->child;

    *frame = mlt_frame_init(MLT_PRODUCER_SERVICE(self));
    mlt_frame_set_position(*frame, mlt_producer_position(self));

    mlt_properties unique_properties = mlt_frame_unique_properties(*frame,
                                                                   MLT_PRODUCER_SERVICE(self));
    if (!unique_properties) {
        mlt_log_error(MLT_PRODUCER_SERVICE(self), "Unique properties missing\n");
        return 1;
    }

    if (mlt_producer_frame(self) != mlt_producer_frame(xml_producer)) {
        mlt_producer_seek(xml_producer, mlt_producer_frame(self));
    }
    mlt_frame xml_frame = NULL;
    int result = mlt_service_get_frame(MLT_PRODUCER_SERVICE(xml_producer), &xml_frame, index);
    if (result) {
        mlt_log_error(MLT_PRODUCER_SERVICE(self), "Unable to get frame from xml producer\n");
        return result;
    }

    mlt_frame_push_service(*frame, self);
    mlt_frame_push_get_image(*frame, producer_get_image);
    mlt_frame_push_audio(*frame, (void *) self);
    mlt_frame_push_audio(*frame, producer_get_audio);

    mlt_profile profile = mlt_service_profile(MLT_PRODUCER_SERVICE(self));
    mlt_properties frame_properties = MLT_FRAME_PROPERTIES(*frame);
    mlt_properties_set_double(frame_properties, "aspect_ratio", mlt_profile_sar(profile));
    mlt_properties_set_int(frame_properties, "width", profile->width);
    mlt_properties_set_int(frame_properties, "height", profile->height);
    mlt_properties_set_int(frame_properties, "meta.media.width", profile->width);
    mlt_properties_set_int(frame_properties, "meta.media.height", profile->height);
    mlt_properties_set_int(frame_properties, "progressive", profile->progressive);
    mlt_properties_set_int(frame_properties, "colorspace", profile->colorspace);

    // Save the xml frame for future use and destruction
    mlt_properties_set_data(unique_properties,
                            "xml_frame",
                            xml_frame,
                            0,
                            (mlt_destructor) mlt_frame_close,
                            NULL);

    mlt_producer_prepare_next(self);

    return result;
}

static void producer_close(mlt_producer self)
{
    mlt_producer xml_producer = self->child;
    mlt_producer_close(xml_producer);
    self->close = NULL;
    mlt_producer_close(self);
    free(self);
}

mlt_producer producer_xmlclip_init(mlt_profile profile,
                                   mlt_service_type type,
                                   const char *id,
                                   char *arg)
{
    mlt_profile native_profile = mlt_profile_init(NULL);
    mlt_producer xml_producer = mlt_factory_producer(native_profile, "xml", arg);
    mlt_producer self = mlt_producer_new(native_profile);

    if (!self || !native_profile || !xml_producer) {
        mlt_log_error(NULL, "[xml-clip] Failed to allocate producer\n");
        mlt_producer_close(self);
        mlt_producer_close(xml_producer);
        mlt_profile_close(native_profile);
        return NULL;
    }

    self->get_frame = producer_get_frame;
    self->close = (mlt_destructor) producer_close;
    self->child = xml_producer;

    mlt_properties properties = MLT_PRODUCER_PROPERTIES(self);
    mlt_properties_set_data(properties,
                            "_profile",
                            native_profile,
                            0,
                            (mlt_destructor) mlt_profile_close,
                            NULL);
    mlt_properties_set(properties, "resource", arg);
    mlt_properties_pass_list(properties, MLT_PRODUCER_PROPERTIES(xml_producer), "out, length");
    mlt_properties_set_int(properties, "meta.media.width", native_profile->width);
    mlt_properties_set_int(properties, "meta.media.height", native_profile->height);
    mlt_properties_set_int(properties, "meta.media.progressive", native_profile->progressive);
    mlt_properties_set_int(properties, "meta.media.frame_rate_num", native_profile->frame_rate_num);
    mlt_properties_set_int(properties, "meta.media.frame_rate_den", native_profile->frame_rate_den);
    mlt_properties_set_int(properties,
                           "meta.media.sample_aspect_num",
                           native_profile->sample_aspect_num);
    mlt_properties_set_int(properties,
                           "meta.media.sample_aspect_den",
                           native_profile->sample_aspect_den);
    mlt_properties_set_int(properties, "meta.media.colorspace", native_profile->colorspace);
    mlt_properties_set_int(properties, "seekable", 1);
    // "static_profile" indicates that this producer does not support changing the profile.
    mlt_properties_set_int(properties, "static_profile", 1);

    return self;
}
