/*
 * producer_blank.c
 * Copyright (C) 2023 Meltytech, LLC
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

#include <framework/mlt_frame.h>
#include <framework/mlt_producer.h>

#include <stdlib.h>

static int producer_get_frame(mlt_producer self, mlt_frame_ptr frame, int index)
{
    // Generate a frame
    *frame = mlt_frame_init(MLT_PRODUCER_SERVICE(self));

    if (*frame != NULL) {
        mlt_properties frame_properties = MLT_FRAME_PROPERTIES(*frame);

        mlt_frame_set_position(*frame, mlt_producer_position(self));

        // Set producer-specific frame properties
        mlt_properties_set_int(frame_properties, "progressive", 1);
    }

    // Calculate the next timecode
    mlt_producer_prepare_next(self);

    return 0;
}

static void producer_close(mlt_producer producer)
{
    producer->close = NULL;
    mlt_producer_close(producer);
    free(producer);
}

mlt_producer producer_blank_init(mlt_profile profile,
                                 mlt_service_type type,
                                 const char *id,
                                 char *arg)
{
    mlt_producer self = calloc(1, sizeof(struct mlt_producer_s));
    if (self != NULL && mlt_producer_init(self, NULL) == 0) {
        mlt_properties_set(MLT_PRODUCER_PROPERTIES(self), "mlt_service", "blank");
        mlt_properties_set(MLT_PRODUCER_PROPERTIES(self), "resource", "blank");
        // Callback registration
        self->get_frame = producer_get_frame;
        self->close = (mlt_destructor) producer_close;
    } else {
        free(self);
        self = NULL;
    }
    return self;
}
