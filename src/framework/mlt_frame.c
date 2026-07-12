/**
 * \file mlt_frame.c
 * \brief interface for all frame classes
 * \see mlt_frame_s
 *
 * Copyright (C) 2003-2026 Meltytech, LLC
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

#include "mlt_frame.h"
#include "mlt_factory.h"
#include "mlt_image.h"
#include "mlt_log.h"
#include "mlt_producer.h"
#include "mlt_profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Construct a frame object.
 *
 * \public \memberof mlt_frame_s
 * \param service the pointer to any service that can provide access to the profile
 * \return a frame object on success or NULL if there was an allocation error
 */

mlt_frame mlt_frame_init(mlt_service service)
{
    // Allocate a frame
    mlt_frame self = calloc(1, sizeof(struct mlt_frame_s));

    if (self != NULL) {
        mlt_profile profile = mlt_service_profile(service);

        // Initialise the properties
        mlt_properties properties = &self->parent;
        mlt_properties_init(properties, self);

        // Set default properties on the frame
        mlt_properties_set_position(properties, "_position", 0.0);
        mlt_properties_set_data(properties, "image", NULL, 0, NULL, NULL);
        mlt_properties_set_int(properties, "width", profile ? profile->width : 720);
        mlt_properties_set_int(properties, "height", profile ? profile->height : 576);
        mlt_properties_set_double(properties, "aspect_ratio", mlt_profile_sar(NULL));
        mlt_properties_set_data(properties, "audio", NULL, 0, NULL, NULL);
        mlt_properties_set_data(properties, "alpha", NULL, 0, NULL, NULL);

        // Construct stacks for frames and methods
        self->stack_image = mlt_deque_init();
        self->stack_audio = mlt_deque_init();
        self->stack_service = mlt_deque_init();
        self->convert_image = mlt_frame_convert_image;
    }

    return self;
}

/** Get a frame's properties.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return the frame's properties or NULL if an invalid frame is supplied
 */

mlt_properties mlt_frame_properties(mlt_frame self)
{
    return self != NULL ? &self->parent : NULL;
}

/** Determine if the frame will produce a test card image.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return true (non-zero) if this will produce from a test card
 */

int mlt_frame_is_test_card(mlt_frame self)
{
    mlt_properties properties = MLT_FRAME_PROPERTIES(self);
    return (mlt_deque_count(self->stack_image) == 0
            && !mlt_properties_get_data(properties, "image", NULL))
           || mlt_properties_get_int(properties, "test_image");
}

/** Determine if the frame will produce audio from a test card.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return true (non-zero) if this will produce from a test card
 */

int mlt_frame_is_test_audio(mlt_frame self)
{
    mlt_properties properties = MLT_FRAME_PROPERTIES(self);
    return (mlt_deque_count(self->stack_audio) == 0
            && !mlt_properties_get_data(properties, "audio", NULL))
           || mlt_properties_get_int(properties, "test_audio");
}

/** Get the sample aspect ratio of the frame.
 *
 * \public \memberof  mlt_frame_s
 * \param self a frame
 * \return the aspect ratio
 */

double mlt_frame_get_aspect_ratio(mlt_frame self)
{
    return mlt_properties_get_double(MLT_FRAME_PROPERTIES(self), "aspect_ratio");
}

/** Set the sample aspect ratio of the frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param value the new image sample aspect ratio
 * \return true if error
 */

int mlt_frame_set_aspect_ratio(mlt_frame self, double value)
{
    return mlt_properties_set_double(MLT_FRAME_PROPERTIES(self), "aspect_ratio", value);
}

/** Get the time position of this frame.
 *
 * This position is not necessarily the position as the original
 * producer knows it. It could be the position that the playlist,
 * multitrack, or tractor producer set.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return the position
 * \see mlt_frame_original_position
 */

mlt_position mlt_frame_get_position(mlt_frame self)
{
    int pos = mlt_properties_get_position(MLT_FRAME_PROPERTIES(self), "_position");
    return pos < 0 ? 0 : pos;
}

/** Get the original time position of this frame.
 *
 * This is the position that the original producer set on the frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return the position
 */

mlt_position mlt_frame_original_position(mlt_frame self)
{
    int pos = mlt_properties_get_position(MLT_FRAME_PROPERTIES(self), "original_position");
    return pos < 0 ? 0 : pos;
}

/** Set the time position of this frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param value the position
 * \return true if error
 */

int mlt_frame_set_position(mlt_frame self, mlt_position value)
{
    // Only set the original_position the first time.
    if (!mlt_properties_get(MLT_FRAME_PROPERTIES(self), "original_position"))
        mlt_properties_set_position(MLT_FRAME_PROPERTIES(self), "original_position", value);
    return mlt_properties_set_position(MLT_FRAME_PROPERTIES(self), "_position", value);
}

/** Stack a get_image callback.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param get_image the get_image callback
 * \return true if error
 */

int mlt_frame_push_get_image(mlt_frame self, mlt_get_image get_image)
{
    return mlt_deque_push_back(self->stack_image, get_image);
}

/** Pop a get_image callback.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return the get_image callback
 */

mlt_get_image mlt_frame_pop_get_image(mlt_frame self)
{
    return mlt_deque_pop_back(self->stack_image);
}

/** Push a frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param that the frame to push onto \p self
 * \return true if error
 */

int mlt_frame_push_frame(mlt_frame self, mlt_frame that)
{
    return mlt_deque_push_back(self->stack_image, that);
}

/** Pop a frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return a frame that was previously pushed
 */

mlt_frame mlt_frame_pop_frame(mlt_frame self)
{
    return mlt_deque_pop_back(self->stack_image);
}

/** Push a service.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param that an opaque pointer
 * \return true if error
 */

int mlt_frame_push_service(mlt_frame self, void *that)
{
    return mlt_deque_push_back(self->stack_image, that);
}

/** Pop a service.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return an opaque pointer to something previously pushed
 */

void *mlt_frame_pop_service(mlt_frame self)
{
    return mlt_deque_pop_back(self->stack_image);
}

/** Push a number.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param that an integer
 * \return true if error
 */

int mlt_frame_push_service_int(mlt_frame self, int that)
{
    return mlt_deque_push_back_int(self->stack_image, that);
}

/** Pop a number.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return an integer that was previously pushed
 */

int mlt_frame_pop_service_int(mlt_frame self)
{
    return mlt_deque_pop_back_int(self->stack_image);
}

/** Push an audio item on the stack.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param that an opaque pointer
 * \return true if error
 */

int mlt_frame_push_audio(mlt_frame self, void *that)
{
    return mlt_deque_push_back(self->stack_audio, that);
}

/** Pop an audio item from the stack
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return an opaque pointer to something that was pushed onto the frame's audio stack
 */

void *mlt_frame_pop_audio(mlt_frame self)
{
    return mlt_deque_pop_back(self->stack_audio);
}

/** Return the service stack
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return the service stack
 */

mlt_deque mlt_frame_service_stack(mlt_frame self)
{
    return self->stack_service;
}

/** Set a new image on the frame.
  *
  * \public \memberof mlt_frame_s
  * \param self a frame
  * \param image a pointer to the raw image data
  * \param size the size of the image data in bytes (optional)
  * \param destroy a function to deallocate \p image when the frame is closed (optional)
  * \return true if error
  */

int mlt_frame_set_image(mlt_frame self, uint8_t *image, int size, mlt_destructor destroy)
{
    return mlt_properties_set_data(MLT_FRAME_PROPERTIES(self), "image", image, size, destroy, NULL);
}

/** Set a new alpha channel on the frame.
  *
  * \public \memberof mlt_frame_s
  * \param self a frame
  * \param alpha a pointer to the alpha channel
  * \param size the size of the alpha channel in bytes (optional)
  * \param destroy a function to deallocate \p alpha when the frame is closed (optional)
  * \return true if error
  */

int mlt_frame_set_alpha(mlt_frame self, uint8_t *alpha, int size, mlt_destructor destroy)
{
    return mlt_properties_set_data(MLT_FRAME_PROPERTIES(self), "alpha", alpha, size, destroy, NULL);
}

/** Replace image stack with the information provided.
 *
 * This might prove to be unreliable and restrictive - the idea is that a transition
 * which normally uses two images may decide to only use the b frame (ie: in the case
 * of a composite where the b frame completely obscures the a frame).
 *
 * The image must be writable and the destructor for the image itself must be taken
 * care of on another frame and that frame cannot have a replace applied to it...
 * Further it assumes that no alpha mask is in use.
 *
 * For these reasons, it can only be used in a specific situation - when you have
 * multiple tracks each with their own transition and these transitions are applied
 * in a strictly reversed order (ie: highest numbered [lowest track] is processed
 * first).
 *
 * More reliable approach - the cases should be detected during the process phase
 * and the upper tracks should simply not be invited to stack...
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param image a new image
 * \param format the image format
 * \param width the width of the new image
 * \param height the height of the new image
 */

void mlt_frame_replace_image(
    mlt_frame self, uint8_t *image, mlt_image_format format, int width, int height)
{
    // Remove all items from the stack
    while (mlt_deque_pop_back(self->stack_image))
        ;

    // Update the information
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(self), "image", image, 0, NULL, NULL);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(self), "width", width);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(self), "height", height);
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(self), "format", format);
}

static int generate_test_image(mlt_properties properties,
                               uint8_t **buffer,
                               mlt_image_format *format,
                               int *width,
                               int *height,
                               int writable)
{
    mlt_producer producer = mlt_properties_get_data(properties, "test_card_producer", NULL);
    mlt_image_format requested_format = *format;
    int error = 1;

    if (producer) {
        mlt_frame test_frame = NULL;
        mlt_service_get_frame(MLT_PRODUCER_SERVICE(producer), &test_frame, 0);
        if (test_frame) {
            mlt_properties test_properties = MLT_FRAME_PROPERTIES(test_frame);
            mlt_properties_set_data(properties,
                                    "test_card_frame",
                                    test_frame,
                                    0,
                                    (mlt_destructor) mlt_frame_close,
                                    NULL);
            mlt_properties_set(test_properties,
                               "consumer.rescale",
                               mlt_properties_get(properties, "consumer.rescale"));
            error = mlt_frame_get_image(test_frame, buffer, format, width, height, writable);
            if (!error && buffer && *buffer) {
                mlt_properties_set_double(properties,
                                          "aspect_ratio",
                                          mlt_frame_get_aspect_ratio(test_frame));
                mlt_properties_set_int(properties, "width", *width);
                mlt_properties_set_int(properties, "height", *height);
                if (mlt_frame_has_convert_image(test_frame) && requested_format != mlt_image_none)
                    mlt_frame_convert_image(test_frame, buffer, format, requested_format);
                mlt_properties_set_int(properties, "format", *format);
            }
        } else {
            mlt_properties_set_data(properties, "test_card_producer", NULL, 0, NULL, NULL);
        }
    }
    if (error && buffer) {
        *width = *width == 0 ? 720 : *width;
        *height = *height == 0 ? 576 : *height;
        switch (*format) {
        case mlt_image_rgb:
        case mlt_image_rgba:
        case mlt_image_yuv422:
        case mlt_image_yuv420p:
        case mlt_image_yuv422p16:
        case mlt_image_yuv420p10:
        case mlt_image_yuv444p10:
        case mlt_image_rgba64:
            break;
        case mlt_image_none:
        case mlt_image_movit:
        case mlt_image_opengl_texture:
        case mlt_image_private:
            *format = mlt_image_yuv422;
            break;
        case mlt_image_invalid:
            *format = mlt_image_invalid;
            break;
        }

        struct mlt_image_s img;
        mlt_image_set_values(&img, NULL, *format, *width, *height);
        mlt_image_alloc_data(&img);

        if (mlt_properties_get_int(properties, "test_audio")) {
            const char *color_range = mlt_properties_get(properties, "consumer.color_range");
            mlt_image_fill_white(&img, mlt_image_full_range(color_range));
        } else {
            mlt_image_fill_checkerboard(&img, mlt_properties_get_double(properties, "aspect_ratio"));
        }

        *buffer = img.data;
        mlt_properties_set_int(properties, "format", *format);
        mlt_properties_set_int(properties, "width", *width);
        mlt_properties_set_int(properties, "height", *height);
        mlt_properties_set_data(properties, "image", *buffer, 0, img.release_data, NULL);
        mlt_properties_set_int(properties, "test_image", 1);
        error = 0;
    }
    return error;
}

/** Get the image associated to the frame.
 *
 * You should express the desired format, width, and height as inputs. As long
 * as the loader producer was used to generate this or the imageconvert filter
 * was attached, then you will get the image back in the format you desire.
 * However, you do not always get the width and height you request depending
 * on properties and filters. You do not need to supply a pre-allocated
 * buffer, but you should always supply the desired image format.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param[out] buffer an image buffer
 * \param[in,out] format the image format
 * \param[in,out] width the horizontal size in pixels
 * \param[in,out] height the vertical size in pixels
 * \param writable whether or not you will need to be able to write to the memory returned in \p buffer
 * \return true if error
 * \todo Better describe the width and height as inputs.
 */

int mlt_frame_get_image(mlt_frame self,
                        uint8_t **buffer,
                        mlt_image_format *format,
                        int *width,
                        int *height,
                        int writable)
{
    mlt_properties properties = MLT_FRAME_PROPERTIES(self);
    mlt_get_image get_image = mlt_frame_pop_get_image(self);
    mlt_image_format requested_format = *format;
    int error = 0;

    if (get_image) {
        mlt_properties_set_int(properties,
                               "image_count",
                               mlt_properties_get_int(properties, "image_count") - 1);
        error = get_image(self, buffer, format, width, height, writable);
        if (!error && buffer && *buffer) {
            mlt_properties_set_int(properties, "width", *width);
            mlt_properties_set_int(properties, "height", *height);
            if (mlt_frame_has_convert_image(self) && requested_format != mlt_image_none)
                mlt_frame_convert_image(self, buffer, format, requested_format);
            mlt_properties_set_int(properties, "format", *format);
        } else {
            error = generate_test_image(properties, buffer, format, width, height, writable);
        }
    } else if (mlt_properties_get_data(properties, "image", NULL) && buffer) {
        *format = mlt_properties_get_int(properties, "format");
        *buffer = mlt_properties_get_data(properties, "image", NULL);
        *width = mlt_properties_get_int(properties, "width");
        *height = mlt_properties_get_int(properties, "height");
        if (mlt_frame_has_convert_image(self) && *buffer && requested_format != mlt_image_none) {
            mlt_frame_convert_image(self, buffer, format, requested_format);
            mlt_properties_set_int(properties, "format", *format);
        }
    } else {
        error = generate_test_image(properties, buffer, format, width, height, writable);
    }

    return error;
}

/** Get the alpha channel associated to the frame (without creating if it has not).
 *
 * This returns NULL if the frame's image format is \p mlt_image_rgba or mlt_image_rgba64.
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return the alpha channel or NULL
 */

uint8_t *mlt_frame_get_alpha(mlt_frame self)
{
    uint8_t *alpha = NULL;
    if (self != NULL) {
        alpha = mlt_properties_get_data(&self->parent, "alpha", NULL);
        if (alpha) {
            mlt_image_format format = mlt_properties_get_int(&self->parent, "format");
            if (mlt_image_rgba == format || format == mlt_image_rgba64) {
                alpha = NULL;
            }
        }
    }
    return alpha;
}

/** Get the alpha channel associated to the frame and its size.
 *
 * This returns NULL and sets \p size to 0 if the frame's image format is
 * \p mlt_image_rgba or mlt_image_rgba64.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param[out] size the size of the alpha channel data in bytes
 * \return the alpha channel or NULL
 */

uint8_t *mlt_frame_get_alpha_size(mlt_frame self, int *size)
{
    uint8_t *alpha = NULL;
    if (self) {
        alpha = mlt_properties_get_data(&self->parent, "alpha", size);
        if (alpha) {
            mlt_image_format format = mlt_properties_get_int(&self->parent, "format");
            if (mlt_image_rgba == format || mlt_image_rgba64 == format) {
                alpha = NULL;
                if (size) {
                    size = 0;
                }
            }
        }
    }
    return alpha;
}

/** Get the audio associated to the frame.
 *
 * You should express the desired format, frequency, channels, and samples as inputs. As long
 * as the loader producer was used to generate this or the audioconvert filter
 * was attached, then you will get the audio back in the format you desire.
 * However, you do not always get the channels and samples you request depending
 * on properties and filters. You do not need to supply a pre-allocated
 * buffer, but you should always supply the desired audio format.
 * The audio is always in interleaved format.
 * You should use the \p mlt_audio_sample_calculator to determine the number of samples you want.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param[out] buffer an audio buffer
 * \param[in,out] format the audio format
 * \param[in,out] frequency the sample rate
 * \param[in,out] channels the number of audio channels
 * \param[in,out] samples the number of samples per frame
 * \return true if error
 */

int mlt_frame_get_audio(mlt_frame self,
                        void **buffer,
                        mlt_audio_format *format,
                        int *frequency,
                        int *channels,
                        int *samples)
{
    mlt_get_audio get_audio = mlt_frame_pop_audio(self);
    mlt_properties properties = MLT_FRAME_PROPERTIES(self);
    int hide = mlt_properties_get_int(properties, "test_audio");
    mlt_audio_format requested_format = *format;

    if (hide == 0 && get_audio != NULL) {
        get_audio(self, buffer, format, frequency, channels, samples);
        mlt_properties_set_int(properties, "audio_frequency", *frequency);
        mlt_properties_set_int(properties, "audio_channels", *channels);
        mlt_properties_set_int(properties, "audio_samples", *samples);
        mlt_properties_set_int(properties, "audio_format", *format);
        if (self->convert_audio && *buffer && requested_format != mlt_audio_none)
            self->convert_audio(self, buffer, format, requested_format);
    } else if (mlt_properties_get_data(properties, "audio", NULL)) {
        *buffer = mlt_properties_get_data(properties, "audio", NULL);
        *format = mlt_properties_get_int(properties, "audio_format");
        *frequency = mlt_properties_get_int(properties, "audio_frequency");
        *channels = mlt_properties_get_int(properties, "audio_channels");
        *samples = mlt_properties_get_int(properties, "audio_samples");
        if (self->convert_audio && *buffer && requested_format != mlt_audio_none)
            self->convert_audio(self, buffer, format, requested_format);
    } else {
        int size = 0;
        *samples = *samples <= 0 ? 1920 : *samples;
        *channels = *channels <= 0 ? 2 : *channels;
        *frequency = *frequency <= 0 ? 48000 : *frequency;
        *format = *format == mlt_audio_none ? mlt_audio_s16 : *format;
        mlt_properties_set_int(properties, "audio_frequency", *frequency);
        mlt_properties_set_int(properties, "audio_channels", *channels);
        mlt_properties_set_int(properties, "audio_samples", *samples);
        mlt_properties_set_int(properties, "audio_format", *format);

        size = mlt_audio_format_size(*format, *samples, *channels);
        if (size)
            *buffer = mlt_pool_alloc(size);
        else
            *buffer = NULL;
        if (*buffer)
            memset(*buffer, 0, size);
        mlt_properties_set_data(properties,
                                "audio",
                                *buffer,
                                size,
                                (mlt_destructor) mlt_pool_release,
                                NULL);
        mlt_properties_set_int(properties, "test_audio", 1);
    }

    // TODO: This does not belong here
    if (*format == mlt_audio_s16 && mlt_properties_get(properties, "meta.volume") && *buffer) {
        double value = mlt_properties_get_double(properties, "meta.volume");

        if (value == 0.0) {
            memset(*buffer, 0, *samples * *channels * 2);
        } else if (value != 1.0) {
            int total = *samples * *channels;
            int16_t *p = *buffer;
            while (total--) {
                *p = *p * value;
                p++;
            }
        }

        mlt_properties_set(properties, "meta.volume", NULL);
    }

    return 0;
}

/** Set the audio on a frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param buffer an buffer containing audio samples
 * \param format the format of the audio in the \p buffer
 * \param size the total size of the buffer (optional)
 * \param destructor a function that releases or deallocates the \p buffer
 * \return true if error
 */

int mlt_frame_set_audio(
    mlt_frame self, void *buffer, mlt_audio_format format, int size, mlt_destructor destructor)
{
    mlt_properties_set_int(MLT_FRAME_PROPERTIES(self), "audio_format", format);
    return mlt_properties_set_data(MLT_FRAME_PROPERTIES(self),
                                   "audio",
                                   buffer,
                                   size,
                                   destructor,
                                   NULL);
}

/** Get audio on a frame as a waveform image.
 *
 * This generates an 8-bit grayscale image representation of the audio in a
 * frame. Currently, this only really works for 2 channels.
 * This allocates the bitmap using mlt_pool so you should release the return
 * value with \p mlt_pool_release.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param w the width of the image
 * \param h the height of the image to create
 * \return a pointer to a new bitmap
 */

unsigned char *mlt_frame_get_waveform(mlt_frame self, int w, int h)
{
    int16_t *pcm = NULL;
    mlt_properties properties = MLT_FRAME_PROPERTIES(self);
    mlt_audio_format format = mlt_audio_s16;
    int frequency = 16000;
    int channels = 2;
    mlt_producer producer = mlt_frame_get_original_producer(self);
    double fps = mlt_producer_get_fps(mlt_producer_cut_parent(producer));
    int samples = mlt_audio_calculate_frame_samples(fps, frequency, mlt_frame_get_position(self));

    // Increase audio resolution proportional to requested image size
    while (samples < w) {
        frequency += 16000;
        samples = mlt_audio_calculate_frame_samples(fps, frequency, mlt_frame_get_position(self));
    }

    // Get the pcm data
    mlt_frame_get_audio(self, (void **) &pcm, &format, &frequency, &channels, &samples);

    // Make an 8-bit buffer large enough to hold rendering
    int size = w * h;
    if (size <= 0)
        return NULL;
    unsigned char *bitmap = (unsigned char *) mlt_pool_alloc(size);
    if (bitmap != NULL)
        memset(bitmap, 0, size);
    else
        return NULL;
    mlt_properties_set_data(properties,
                            "waveform",
                            bitmap,
                            size,
                            (mlt_destructor) mlt_pool_release,
                            NULL);

    // Render vertical lines
    int16_t *ubound = pcm + samples * channels;
    int skip = samples / w;
    skip = !skip ? 1 : skip;
    unsigned char gray = 0xFF / skip;
    int i, j, k;

    // Iterate sample stream and along x coordinate
    for (i = 0; pcm < ubound; i++) {
        // pcm data has channels interleaved
        for (j = 0; j < channels; j++, pcm++) {
            // Determine sample's magnitude from 2s complement;
            int pcm_magnitude = *pcm < 0 ? ~(*pcm) + 1 : *pcm;
            // The height of a line is the ratio of the magnitude multiplied by
            // the vertical resolution of a single channel
            int height = h * pcm_magnitude / channels / 2 / 32768;
            // Determine the starting y coordinate - left top, right bottom
            int displacement = h * (j * 2 + 1) / channels / 2 - (*pcm < 0 ? 0 : height);
            // Position buffer pointer using y coordinate, stride, and x coordinate
            unsigned char *p = bitmap + i / skip + displacement * w;

            // Draw vertical line
            for (k = 0; k < height + 1; k++)
                if (*pcm < 0)
                    p[w * k] = (k == 0) ? 0xFF : p[w * k] + gray;
                else
                    p[w * k] = (k == height) ? 0xFF : p[w * k] + gray;
        }
    }

    return bitmap;
}

/** Get the end service that produced self frame.
 *
 * This fetches the first producer of the frame and not any producers that
 * encapsulate it.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return a producer
 */

mlt_producer mlt_frame_get_original_producer(mlt_frame self)
{
    if (self != NULL)
        return mlt_properties_get_data(MLT_FRAME_PROPERTIES(self), "_producer", NULL);
    return NULL;
}

/** Destroy the frame.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 */

void mlt_frame_close(mlt_frame self)
{
    if (self != NULL && mlt_properties_dec_ref(MLT_FRAME_PROPERTIES(self)) <= 0) {
        mlt_deque_close(self->stack_image);
        mlt_deque_close(self->stack_audio);
        while (mlt_deque_peek_back(self->stack_service))
            mlt_service_close(mlt_deque_pop_back(self->stack_service));
        mlt_deque_close(self->stack_service);
        mlt_properties_close(&self->parent);
        free(self);
    }
}

/* ---- Image conversion callback list ---- */

#define CONVERT_IMAGE_CALLBACKS "_convert_image_callbacks"
#define CONVERT_IMAGE_IDX "_convert_image_idx"

static int do_convert_image(
    mlt_frame self, uint8_t **image, mlt_image_format *format, mlt_image_format output, int idx)
{
    mlt_properties props = MLT_FRAME_PROPERTIES(self);
    mlt_deque list = mlt_properties_get_data(props, CONVERT_IMAGE_CALLBACKS, NULL);
    int count = list ? mlt_deque_count(list) : 0;

    while (idx < count) {
        mlt_properties_set_int(props, CONVERT_IMAGE_IDX, idx);
        mlt_convert_image fn = (mlt_convert_image) mlt_deque_peek(list, idx);
        int error = fn(self, image, format, output);
        if (!error)
            return 0;
        // fn may have internally advanced _convert_image_idx via mlt_frame_next_convert_image;
        // skip past any indices already tried to avoid double-calling.
        int next = mlt_properties_get_int(props, CONVERT_IMAGE_IDX);
        idx = (next > idx) ? next + 1 : idx + 1;
    }
    return 1;
}

/** Register an image-conversion callback on the frame.
 *
 * Callbacks are appended to a list and dispatched in order by
 * \p mlt_frame_convert_image. If \p convert is already registered on this
 * frame it is silently ignored (deduplication). Has no effect if either
 * argument is NULL.
 * 
 * This function is primarily used by the \p loader producer per \p loader.ini
 * to add the default and fallback converters. Most modules or external callers
 * will want to use \p mlt_frame_prepend_convert_image.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param convert the conversion callback to register
 */
void mlt_frame_append_convert_image(mlt_frame self, mlt_convert_image convert)
{
    if (!self || !convert)
        return;
    mlt_properties props = MLT_FRAME_PROPERTIES(self);
    mlt_deque list = mlt_properties_get_data(props, CONVERT_IMAGE_CALLBACKS, NULL);
    if (!list) {
        list = mlt_deque_init();
        mlt_properties_set_data(props,
                                CONVERT_IMAGE_CALLBACKS,
                                list,
                                0,
                                (mlt_destructor) mlt_deque_close,
                                NULL);
    }
    // Deduplicate: don't register the same function pointer twice.
    for (int i = 0; i < mlt_deque_count(list); i++)
        if (mlt_deque_peek(list, i) == (void *) convert)
            return;
    mlt_deque_push_back(list, (void *) convert);
}

/** Register an image-conversion callback on the frame.
 *
 * Callbacks are prepended to a list and dispatched in order by
 * \p mlt_frame_convert_image. If \p convert is already registered on this
 * frame it is silently ignored (deduplication). Has no effect if either
 * argument is NULL.
 * 
 * This is more useful for a specialized or incomplete converter. Return 
 * error from your callback to hand-off to the next converters, one of which
 * ought to be able to do the conversion. This is not all-or-nothing; you can
 * still convert to a supported format that your code handles even if it does
 * not match the requested format. Simply update the \p format argument and/or
 * \p image argument and return a non-zero value.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param convert the conversion callback to register
 */
void mlt_frame_prepend_convert_image(mlt_frame self, mlt_convert_image convert)
{
    if (!self || !convert)
        return;
    mlt_properties props = MLT_FRAME_PROPERTIES(self);
    mlt_deque list = mlt_properties_get_data(props, CONVERT_IMAGE_CALLBACKS, NULL);
    if (!list) {
        list = mlt_deque_init();
        mlt_properties_set_data(props,
                                CONVERT_IMAGE_CALLBACKS,
                                list,
                                0,
                                (mlt_destructor) mlt_deque_close,
                                NULL);
    }
    // Deduplicate: don't register the same function pointer twice.
    for (int i = 0; i < mlt_deque_count(list); i++)
        if (mlt_deque_peek(list, i) == (void *) convert)
            return;
    mlt_deque_push_front(list, (void *) convert);
}

/** Determine whether any image-conversion callbacks are registered.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \return non-zero if at least one callback has been pushed, zero otherwise
 */
int mlt_frame_has_convert_image(mlt_frame self)
{
    if (!self)
        return 0;
    mlt_deque list = mlt_properties_get_data(MLT_FRAME_PROPERTIES(self),
                                             CONVERT_IMAGE_CALLBACKS,
                                             NULL);
    return list && mlt_deque_count(list) > 0;
}

/** Convert the frame image to the requested format.
 *
 * Dispatches the registered conversion callbacks in order, stopping at the
 * first one that succeeds (returns 0). If no callback succeeds, returns 1.
 * This is also the function stored in the \p convert_image field of every
 * frame; external callers that hold a pointer to that field will therefore
 * invoke this dispatcher transparently.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param[in,out] image the image buffer pointer
 * \param[in,out] format the current image format; updated to \p output on success
 * \param output the desired image format
 * \return 0 on success, 1 if no callback could perform the conversion
 */
int mlt_frame_convert_image(mlt_frame self,
                            uint8_t **image,
                            mlt_image_format *format,
                            mlt_image_format output)
{
    return do_convert_image(self, image, format, output, 0);
}

/** Advance to the next image-conversion callback and continue dispatching.
 *
 * Call this from inside a conversion callback to delegate to the next
 * registered callback in the list (fallback chaining). The current
 * callback's index is read from the frame's internal dispatch state, so
 * this must only be called from within an active \p mlt_frame_convert_image
 * dispatch.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param[in,out] image the image buffer pointer
 * \param[in,out] format the current image format
 * \param output the desired image format
 * \return 0 if a subsequent callback succeeded, 1 if none did
 */
int mlt_frame_next_convert_image(mlt_frame self,
                                 uint8_t **image,
                                 mlt_image_format *format,
                                 mlt_image_format output)
{
    int idx = mlt_properties_get_int(MLT_FRAME_PROPERTIES(self), CONVERT_IMAGE_IDX);
    return do_convert_image(self, image, format, output, idx + 1);
}

/** Copy the image-conversion callback list from one frame to another.
 *
 * Used by the tractor and clone functions to propagate converters attached
 * to track or source frames onto merged or cloned frames. If \p dst already
 * has a callback list the copy is skipped (first-wins semantics).
 *
 * \public \memberof mlt_frame_s
 * \param dst the frame to copy callbacks onto
 * \param src the frame to copy callbacks from
 */
void mlt_frame_copy_convert_image(mlt_frame dst, mlt_frame src)
{
    mlt_deque src_list = mlt_properties_get_data(MLT_FRAME_PROPERTIES(src),
                                                 CONVERT_IMAGE_CALLBACKS,
                                                 NULL);
    if (!src_list || !mlt_deque_count(src_list))
        return;
    mlt_deque dst_list = mlt_properties_get_data(MLT_FRAME_PROPERTIES(dst),
                                                 CONVERT_IMAGE_CALLBACKS,
                                                 NULL);
    if (dst_list)
        return;
    dst_list = mlt_deque_init();
    for (int i = 0; i < mlt_deque_count(src_list); i++)
        mlt_deque_push_back(dst_list, mlt_deque_peek(src_list, i));
    mlt_properties_set_data(MLT_FRAME_PROPERTIES(dst),
                            CONVERT_IMAGE_CALLBACKS,
                            dst_list,
                            0,
                            (mlt_destructor) mlt_deque_close,
                            NULL);
}

/***** convenience functions *****/

void mlt_frame_write_ppm(mlt_frame frame)
{
    int width = 0;
    int height = 0;
    mlt_image_format format = mlt_image_rgb;
    uint8_t *image;

    if (mlt_frame_get_image(frame, &image, &format, &width, &height, 0) == 0) {
        FILE *file;
        char filename[16];

        sprintf(filename, "frame-%05d.ppm", (int) mlt_frame_get_position(frame));
        file = mlt_fopen(filename, "wb");
        if (!file)
            return;
        fprintf(file, "P6\n%d %d\n255\n", width, height);
        fwrite(image, width * height * 3, 1, file);
        fclose(file);
    }
}

/** Get or create a properties object unique to this service instance.
 *
 * Use this function to hold a service's processing parameters for this
 * particular frame. Set the parameters in the service's process function.
 * Then, get the parameters in the function it pushes to the frame's audio
 * or image stack. This makes the service more parallel by reducing race
 * conditions and less sensitive to multiple instances (by not setting a
 * non-unique property on the frame). Creation and destruction of the
 * properties object is handled automatically.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param service a service
 * \return a properties object
 */

mlt_properties mlt_frame_unique_properties(mlt_frame self, mlt_service service)
{
    mlt_properties frame_props = MLT_FRAME_PROPERTIES(self);
    mlt_properties service_props = MLT_SERVICE_PROPERTIES(service);
    char *unique = mlt_properties_get(service_props, "_unique_id");
    mlt_properties instance_props = mlt_properties_get_data(frame_props, unique, NULL);

    if (!instance_props) {
        instance_props = mlt_properties_new();
        mlt_properties_set_data(frame_props,
                                unique,
                                instance_props,
                                0,
                                (mlt_destructor) mlt_properties_close,
                                NULL);
        mlt_properties_set_lcnumeric(instance_props, mlt_properties_get_lcnumeric(service_props));
        mlt_properties_set_data(instance_props,
                                "_profile",
                                mlt_service_profile(service),
                                0,
                                NULL,
                                NULL);
    }

    return instance_props;
}

/** Get a properties object unique to this service instance.
 *
 * Unlike \p mlt_frame_unique_properties, this function does not create the
 * service-unique properties object if it does not exist.
 *
 * \public \memberof mlt_frame_s
 * \param self a frame
 * \param service a service
 * \return a properties object or NULL if it does not exist
 */

mlt_properties mlt_frame_get_unique_properties(mlt_frame self, mlt_service service)
{
    char *unique = mlt_properties_get(MLT_SERVICE_PROPERTIES(service), "_unique_id");
    return mlt_properties_get_data(MLT_FRAME_PROPERTIES(self), unique, NULL);
}

/** Make a copy of a frame.
 *
 * This does not copy the get_image/get_audio processing stacks or any
 * data properties other than the audio and image.
 *
 * \public \memberof mlt_frame_s
 * \param self the frame to clone
 * \param is_deep a boolean to indicate whether to make a deep copy of the audio
 * and video data chunks or to make a shallow copy by pointing to the supplied frame
 * \return a almost-complete copy of the frame
 * \todo copy the processing deques
 */

mlt_frame mlt_frame_clone(mlt_frame self, int is_deep)
{
    mlt_frame new_frame = mlt_frame_init(NULL);
    mlt_properties properties = MLT_FRAME_PROPERTIES(self);
    mlt_properties new_props = MLT_FRAME_PROPERTIES(new_frame);
    void *data, *copy;
    int size = 0;

    mlt_properties_inherit(new_props, properties);

    // Carry over some special data properties for the multi consumer.
    mlt_properties_set_data(new_props,
                            "_producer",
                            mlt_frame_get_original_producer(self),
                            0,
                            NULL,
                            NULL);
    mlt_frame_copy_convert_image(new_frame, self);

    if (is_deep) {
        data = mlt_properties_get_data(properties, "audio", &size);
        if (data) {
            if (!size)
                size = mlt_audio_format_size(mlt_properties_get_int(properties, "audio_format"),
                                             mlt_properties_get_int(properties, "audio_samples"),
                                             mlt_properties_get_int(properties, "audio_channels"));
            copy = mlt_pool_alloc(size);
            memcpy(copy, data, size);
            mlt_properties_set_data(new_props, "audio", copy, size, mlt_pool_release, NULL);
        }
        size = 0;
        data = mlt_properties_get_data(properties, "image", &size);
        mlt_image_format format = mlt_properties_get_int(properties, "format");
        if (data && format != mlt_image_movit && format != mlt_image_private) {
            int width = mlt_properties_get_int(properties, "width");
            int height = mlt_properties_get_int(properties, "height");

            if (!size)
                size = mlt_image_format_size(format, width, height, NULL);
            copy = mlt_pool_alloc(size);
            memcpy(copy, data, size);
            mlt_properties_set_data(new_props, "image", copy, size, mlt_pool_release, NULL);

            size = 0;
            data = mlt_frame_get_alpha_size(self, &size);
            if (data) {
                if (!size)
                    size = width * height;
                copy = mlt_pool_alloc(size);
                memcpy(copy, data, size);
                mlt_properties_set_data(new_props, "alpha", copy, size, mlt_pool_release, NULL);
            };
        }
    } else {
        // This frame takes a reference on the original frame since the data is a shallow copy.
        mlt_properties_inc_ref(properties);
        mlt_properties_set_data(new_props,
                                "_cloned_frame",
                                self,
                                0,
                                (mlt_destructor) mlt_frame_close,
                                NULL);

        // Copy properties
        data = mlt_properties_get_data(properties, "audio", &size);
        mlt_properties_set_data(new_props, "audio", data, size, NULL, NULL);
        size = 0;
        data = mlt_properties_get_data(properties, "image", &size);
        mlt_properties_set_data(new_props, "image", data, size, NULL, NULL);
        size = 0;
        data = mlt_frame_get_alpha_size(self, &size);
        mlt_properties_set_data(new_props, "alpha", data, size, NULL, NULL);
    }

    return new_frame;
}

/** Make a copy of a frame and audio.
 *
 * This does not copy the get_image/get_audio processing stacks or any
 * data properties other than the audio.
 *
 * \public \memberof mlt_frame_s
 * \param self the frame to clone
 * \param is_deep a boolean to indicate whether to make a deep copy of the audio
 * data chunks or to make a shallow copy by pointing to the supplied frame
 * \return a almost-complete copy of the frame
 * \todo copy the processing deques
 */

mlt_frame mlt_frame_clone_audio(mlt_frame self, int is_deep)
{
    mlt_frame new_frame = mlt_frame_init(NULL);
    mlt_properties properties = MLT_FRAME_PROPERTIES(self);
    mlt_properties new_props = MLT_FRAME_PROPERTIES(new_frame);
    void *data, *copy;
    int size = 0;

    mlt_properties_inherit(new_props, properties);

    // Carry over some special data properties for the multi consumer.
    mlt_properties_set_data(new_props,
                            "_producer",
                            mlt_frame_get_original_producer(self),
                            0,
                            NULL,
                            NULL);
    mlt_frame_copy_convert_image(new_frame, self);

    if (is_deep) {
        data = mlt_properties_get_data(properties, "audio", &size);
        if (data) {
            if (!size)
                size = mlt_audio_format_size(mlt_properties_get_int(properties, "audio_format"),
                                             mlt_properties_get_int(properties, "audio_samples"),
                                             mlt_properties_get_int(properties, "audio_channels"));
            copy = mlt_pool_alloc(size);
            memcpy(copy, data, size);
            mlt_properties_set_data(new_props, "audio", copy, size, mlt_pool_release, NULL);
        }
    } else {
        // This frame takes a reference on the original frame since the data is a shallow copy.
        mlt_properties_inc_ref(properties);
        mlt_properties_set_data(new_props,
                                "_cloned_frame",
                                self,
                                0,
                                (mlt_destructor) mlt_frame_close,
                                NULL);

        // Copy properties
        data = mlt_properties_get_data(properties, "audio", &size);
        mlt_properties_set_data(new_props, "audio", data, size, NULL, NULL);
    }

    return new_frame;
}

/** Make a copy of a frame and image.
 *
 * This does not copy the get_image/get_audio processing stacks or any
 * data properties other than the image.
 *
 * \public \memberof mlt_frame_s
 * \param self the frame to clone
 * \param is_deep a boolean to indicate whether to make a deep copy of the
 * video data chunks or to make a shallow copy by pointing to the supplied frame
 * \return a almost-complete copy of the frame
 * \todo copy the processing deques
 */

mlt_frame mlt_frame_clone_image(mlt_frame self, int is_deep)
{
    mlt_frame new_frame = mlt_frame_init(NULL);
    mlt_properties properties = MLT_FRAME_PROPERTIES(self);
    mlt_properties new_props = MLT_FRAME_PROPERTIES(new_frame);
    void *data, *copy;
    int size = 0;

    mlt_properties_inherit(new_props, properties);

    // Carry over some special data properties for the multi consumer.
    mlt_properties_set_data(new_props,
                            "_producer",
                            mlt_frame_get_original_producer(self),
                            0,
                            NULL,
                            NULL);
    mlt_frame_copy_convert_image(new_frame, self);

    if (is_deep) {
        data = mlt_properties_get_data(properties, "image", &size);
        mlt_image_format format = mlt_properties_get_int(properties, "format");
        if (data && format != mlt_image_movit && format != mlt_image_private) {
            int width = mlt_properties_get_int(properties, "width");
            int height = mlt_properties_get_int(properties, "height");

            if (!size)
                size = mlt_image_format_size(format, width, height, NULL);
            copy = mlt_pool_alloc(size);
            memcpy(copy, data, size);
            mlt_properties_set_data(new_props, "image", copy, size, mlt_pool_release, NULL);

            size = 0;
            data = mlt_frame_get_alpha_size(self, &size);
            if (data) {
                if (!size)
                    size = width * height;
                copy = mlt_pool_alloc(size);
                memcpy(copy, data, size);
                mlt_properties_set_data(new_props, "alpha", copy, size, mlt_pool_release, NULL);
            };
        }
    } else {
        // This frame takes a reference on the original frame since the data is a shallow copy.
        mlt_properties_inc_ref(properties);
        mlt_properties_set_data(new_props,
                                "_cloned_frame",
                                self,
                                0,
                                (mlt_destructor) mlt_frame_close,
                                NULL);

        // Copy properties
        size = 0;
        data = mlt_properties_get_data(properties, "image", &size);
        mlt_properties_set_data(new_props, "image", data, size, NULL, NULL);
        size = 0;
        data = mlt_frame_get_alpha_size(self, &size);
        mlt_properties_set_data(new_props, "alpha", data, size, NULL, NULL);
    }

    return new_frame;
}
