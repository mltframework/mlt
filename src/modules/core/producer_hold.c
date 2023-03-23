/*
 * producer_hold.c -- frame holding producer
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <framework/mlt.h>

// Forward references
static int producer_get_frame(mlt_producer producer, mlt_frame_ptr frame, int index);
static void producer_close(mlt_producer producer);

/** Constructor for the frame holding producer. Basically, all this producer does is
	provide a producer wrapper for the requested producer, allows the specification of
	the frame required and will then repeatedly obtain that frame for each get_frame
	and get_image requested.
*/

mlt_producer producer_hold_init(mlt_profile profile,
                                mlt_service_type type,
                                const char *id,
                                char *arg)
{
    // Construct a new holding producer
    mlt_producer self = mlt_producer_new(profile);

    // Construct the requested producer via loader
    mlt_producer producer = mlt_factory_producer(profile, NULL, arg);

    // Initialise the frame holding capabilities
    if (self != NULL && producer != NULL) {
        // Get the properties of this producer
        mlt_properties properties = MLT_PRODUCER_PROPERTIES(self);

        // Store the producer
        mlt_properties_set_data(properties,
                                "producer",
                                producer,
                                0,
                                (mlt_destructor) mlt_producer_close,
                                NULL);

        // Set frame, in, out and length for this producer
        mlt_properties_set_position(properties, "frame", 0);
        mlt_properties_set_position(properties, "out", 25);
        mlt_properties_set(properties, "resource", arg);
        mlt_properties_set(properties, "method", "onefield");

        // Override the get_frame method
        self->get_frame = producer_get_frame;
        self->close = (mlt_destructor) producer_close;
    } else {
        // Clean up (not sure which one failed, can't be bothered to find out, so close both)
        if (self)
            mlt_producer_close(self);
        if (producer)
            mlt_producer_close(producer);

        // Make sure we return NULL
        self = NULL;
    }

    // Return this producer
    return self;
}

static int producer_get_image(mlt_frame frame,
                              uint8_t **buffer,
                              mlt_image_format *format,
                              int *width,
                              int *height,
                              int writable)
{
    // Get the properties of the frame
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);

    // Obtain the real frame
    mlt_frame real_frame = mlt_frame_pop_service(frame);

    // Get the image from the real frame
    int size = 0;
    *buffer = mlt_properties_get_data(MLT_FRAME_PROPERTIES(real_frame), "image", &size);
    *width = mlt_properties_get_int(MLT_FRAME_PROPERTIES(real_frame), "width");
    *height = mlt_properties_get_int(MLT_FRAME_PROPERTIES(real_frame), "height");

    // If this is the first time, get it from the producer
    if (*buffer == NULL) {
        mlt_properties_pass(MLT_FRAME_PROPERTIES(real_frame), properties, "");

        // We'll deinterlace on the downstream deinterlacer
        mlt_properties_set_int(MLT_FRAME_PROPERTIES(real_frame), "consumer.progressive", 1);

        // We want distorted to ensure we don't hit the resize filter twice
        mlt_properties_set_int(MLT_FRAME_PROPERTIES(real_frame), "distort", 1);

        // Get the image
        mlt_frame_get_image(real_frame, buffer, format, width, height, writable);

        // Make sure we get the size
        *buffer = mlt_properties_get_data(MLT_FRAME_PROPERTIES(real_frame), "image", &size);
    }

    mlt_properties_pass(properties, MLT_FRAME_PROPERTIES(real_frame), "");

    // Set the values obtained on the frame
    if (*buffer != NULL) {
        uint8_t *image = mlt_pool_alloc(size);
        memcpy(image, *buffer, size);
        *buffer = image;
        mlt_frame_set_image(frame, *buffer, size, mlt_pool_release);
    } else {
        // Pass the current image as is
        mlt_frame_set_image(frame, *buffer, size, NULL);
    }

    // Make sure that no further scaling is done
    mlt_properties_set(properties, "consumer.rescale", "none");
    mlt_properties_set(properties, "scale", "off");

    // All done
    return 0;
}

static int producer_get_frame(mlt_producer producer, mlt_frame_ptr frame, int index)
{
    // Get the properties of this producer
    mlt_properties properties = MLT_PRODUCER_PROPERTIES(producer);

    // Construct a new frame
    *frame = mlt_frame_init(MLT_PRODUCER_SERVICE(producer));

    // If we have a frame, then stack the producer itself and the get_image method
    if (*frame) {
        // Define the real frame
        mlt_frame real_frame = mlt_properties_get_data(properties, "real_frame", NULL);

        // Obtain real frame if we don't have it
        if (real_frame == NULL) {
            // Get the producer
            mlt_producer producer = mlt_properties_get_data(properties, "producer", NULL);

            // Get the frame position requested
            mlt_position position = mlt_properties_get_position(properties, "frame");

            // Seek the producer to the correct place
            mlt_producer_seek(producer, position);

            // Get the real frame
            mlt_service_get_frame(MLT_PRODUCER_SERVICE(producer), &real_frame, index);

            // Ensure that the real frame gets wiped eventually
            mlt_properties_set_data(properties,
                                    "real_frame",
                                    real_frame,
                                    0,
                                    (mlt_destructor) mlt_frame_close,
                                    NULL);
        } else {
            // Temporary fix - ensure that we aren't seen as a test frame
            uint8_t *image = mlt_properties_get_data(MLT_FRAME_PROPERTIES(real_frame),
                                                     "image",
                                                     NULL);
            mlt_frame_set_image(*frame, image, 0, NULL);
            mlt_properties_set_int(MLT_FRAME_PROPERTIES(*frame), "test_image", 0);
        }

        // Stack the real frame and method
        mlt_frame_push_service(*frame, real_frame);
        mlt_frame_push_service(*frame, producer_get_image);

        // Ensure that the consumer sees what the real frame has
        mlt_properties_pass(MLT_FRAME_PROPERTIES(*frame), MLT_FRAME_PROPERTIES(real_frame), "");

        mlt_properties_set(MLT_FRAME_PROPERTIES(real_frame),
                           "consumer.deinterlacer",
                           mlt_properties_get(properties, "method"));
    }

    // Move to the next position
    mlt_producer_prepare_next(producer);

    return 0;
}

static void producer_close(mlt_producer producer)
{
    producer->close = NULL;
    mlt_producer_close(producer);
    free(producer);
}
