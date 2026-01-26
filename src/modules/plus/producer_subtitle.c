/*
 * producer_subtitle.c
 * Copyright (C) 2024-2026 Meltytech, LLC
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

#include <stdlib.h>

static int producer_get_frame(mlt_producer producer, mlt_frame_ptr frame, int index)
{
    mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);
    mlt_producer color_producer = mlt_properties_get_data(producer_properties, "_c", NULL);
    mlt_producer_seek(color_producer, mlt_producer_position(producer));
    mlt_service_get_frame(MLT_PRODUCER_SERVICE(color_producer), frame, index);

    if (*frame != NULL) {
        mlt_filter sub_filter = mlt_properties_get_data(producer_properties, "_s", NULL);
        if (!sub_filter) {
            sub_filter = mlt_factory_filter(mlt_service_profile(MLT_PRODUCER_SERVICE(producer)),
                                            "subtitle",
                                            NULL);
            if (!sub_filter) {
                mlt_log_error(MLT_PRODUCER_SERVICE(producer), "Unable to create subtitle filter.\n");
                return 0;
            }
            // Register the subtitle filter for reuse/destruction
            mlt_properties_set_data(producer_properties,
                                    "_s",
                                    sub_filter,
                                    0,
                                    (mlt_destructor) mlt_filter_close,
                                    NULL);
        }
        mlt_properties_pass_list(
            MLT_FILTER_PROPERTIES(sub_filter),
            producer_properties,
            "resource geometry family size weight style fgcolour bgcolour "
            "olcolour pad halign valign outline underline strikethrough opacity");
        mlt_filter_process(sub_filter, *frame);
    }

    // Calculate the next time code
    mlt_producer_prepare_next(producer);

    return 0;
}

static void producer_close(mlt_producer this)
{
    this->close = NULL;
    mlt_producer_close(this);
    free(this);
}

mlt_producer producer_subtitle_init(mlt_profile profile,
                                    mlt_service_type type,
                                    const char *id,
                                    char *arg)
{
    // Create a new producer object
    mlt_producer producer = mlt_producer_new(profile);
    mlt_producer color_producer = mlt_factory_producer(profile, "loader-nogl", "color");

    // Initialize the producer
    if (producer && color_producer) {
        mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);
        if (arg) {
            mlt_properties_set_string(producer_properties, "resource", arg);
        }
        mlt_properties_set_string(producer_properties, "geometry", "0%/0%:100%x100%:100%");
        mlt_properties_set_string(producer_properties, "family", "Sans");
        mlt_properties_set_string(producer_properties, "size", "48");
        mlt_properties_set_string(producer_properties, "weight", "400");
        mlt_properties_set_string(producer_properties, "style", "normal");
        mlt_properties_set_string(producer_properties, "fgcolour", "0xffffffff");
        mlt_properties_set_string(producer_properties, "bgcolour", "0x00000020");
        mlt_properties_set_string(producer_properties, "olcolour", "0x00000000");
        mlt_properties_set_string(producer_properties, "pad", "0");
        mlt_properties_set_string(producer_properties, "halign", "left");
        mlt_properties_set_string(producer_properties, "valign", "top");
        mlt_properties_set_string(producer_properties, "outline", "0");
        mlt_properties_set_string(producer_properties, "underline", "0");
        mlt_properties_set_string(producer_properties, "strikethrough", "0");
        mlt_properties_set_string(producer_properties, "opacity", "1.0");

        // Register the color producer for reuse/destruction
        mlt_properties_set(MLT_PRODUCER_PROPERTIES(color_producer), "resource", "0x00000000");
        mlt_properties_set_data(producer_properties,
                                "_c",
                                color_producer,
                                0,
                                (mlt_destructor) mlt_producer_close,
                                NULL);

        // Callback registration
        producer->get_frame = producer_get_frame;
        producer->close = (mlt_destructor) producer_close;
    } else {
        if (!color_producer)
            mlt_log_error(MLT_PRODUCER_SERVICE(producer), "Unable to create color producer.\n");
        mlt_producer_close(producer);
        mlt_producer_close(color_producer);
        producer = NULL;
    }

    return producer;
}
