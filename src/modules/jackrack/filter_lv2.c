/*
 * filter_lv2.c -- filter audio through LV2 plugins
 * Copyright (C) 2024-2025 Meltytech, LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_factory.h>
#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <pthread.h>
#include <string.h>

#include "lv2_context.h"

#define BUFFER_LEN (10000)
#define MAX_SAMPLE_COUNT (4096)

static lv2_context_t *initialise_lv2_context(mlt_properties properties, int channels)
{
    lv2_context_t *lv2context = NULL;
    char *resource = mlt_properties_get(properties, "resource");
    if (!resource && mlt_properties_get(properties, "src"))
        resource = mlt_properties_get(properties, "src");

    char *plugin_id = NULL;
    plugin_id = mlt_properties_get(properties, "_pluginid");

    // Start Lv2context
    if (resource || plugin_id) {
        // Create Lv2context without Jack client name so that it only uses LV2
        lv2context = lv2_context_new(NULL, channels);
        mlt_properties_set_data(properties,
                                "lv2context",
                                lv2context,
                                0,
                                (mlt_destructor) lv2_context_destroy,
                                NULL);

        if (plugin_id) {
            // Load one LV2 plugin by its URI

            char *id = plugin_id;
            lv2_plugin_desc_t *desc = lv2_mgr_get_any_desc(lv2context->plugin_mgr, id);

            lv2_plugin_t *plugin;
            if (desc && (plugin = lv2_context_instantiate_plugin(lv2context, desc))) {
                plugin->enabled = TRUE;
                lv2_process_add_plugin(lv2context->procinfo, plugin);
                mlt_properties_set_int(properties, "instances", plugin->copies);
            } else {
                //mlt_log_error(properties, "failed to load plugin %lu\n", id);
                mlt_log_error(properties, "failed to load plugin `%s`\n", id);
                return lv2context;
            }

            if (plugin && plugin->desc && plugin->copies == 0) {
                // Calculate the number of channels that will work with this plugin
                int request_channels = plugin->desc->channels;
                while (request_channels < channels)
                    request_channels += plugin->desc->channels;

                if (request_channels != channels) {
                    // Try to load again with a compatible number of channels.
                    mlt_log_warning(
                        properties,
                        "Not compatible with %d channels. Requesting %d channels instead.\n",
                        channels,
                        request_channels);
                    lv2context = initialise_lv2_context(properties, request_channels);
                } else {
                    mlt_log_error(properties, "Invalid plugin configuration: `%s`\n", id);
                    return lv2context;
                }
            }

            if (plugin && plugin->desc && plugin->copies)
                mlt_log_debug(properties,
                              "Plugin Initialized. Channels: %lu\tCopies: %d\tTotal: %lu\n",
                              plugin->desc->channels,
                              plugin->copies,
                              lv2context->channels);
        }
    }
    return lv2context;
}

/** Get the audio.
*/

static int lv2_get_audio(mlt_frame frame,
                         void **buffer,
                         mlt_audio_format *format,
                         int *frequency,
                         int *channels,
                         int *samples)
{
    int error = 0;

    // Get the filter service
    mlt_filter filter = mlt_frame_pop_audio(frame);

    // Get the filter properties
    mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);

    // Check if the channel configuration has changed
    int prev_channels = mlt_properties_get_int(filter_properties, "_prev_channels");
    if (prev_channels != *channels) {
        if (prev_channels) {
            mlt_log_info(MLT_FILTER_SERVICE(filter),
                         "Channel configuration changed. Old: %d New: %d.\n",
                         prev_channels,
                         *channels);
            mlt_properties_set_data(filter_properties,
                                    "lv2context",
                                    NULL,
                                    0,
                                    (mlt_destructor) NULL,
                                    NULL);
        }
        mlt_properties_set_int(filter_properties, "_prev_channels", *channels);
    }

    // Initialise LV2 if needed
    lv2_context_t *lv2context = mlt_properties_get_data(filter_properties, "lv2context", NULL);
    if (lv2context == NULL) {
        lv2_sample_rate = *frequency; // global inside lv2_context

        lv2context = initialise_lv2_context(filter_properties, *channels);
    }

    char *plugin_id = NULL;
    plugin_id = mlt_properties_get(filter_properties, "_pluginid");

    if (lv2context && lv2context->procinfo && lv2context->procinfo->chain && plugin_id) {
        lv2_plugin_t *plugin = lv2context->procinfo->chain;
        LADSPA_Data value;
        int i, c;
        mlt_position position = mlt_filter_get_position(filter, frame);
        mlt_position length = mlt_filter_get_length2(filter, frame);

        lv2context->procinfo->channel_mask
            = (unsigned long) mlt_properties_get_int64(filter_properties, "channel_mask");

        // Get the producer's audio
        *format = mlt_audio_float;
        mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);

        // Resize the buffer if necessary.
        if (*channels < lv2context->channels) {
            // Add extra channels to satisfy the plugin.
            // Extra channels in the buffer will be ignored by downstream services.
            int old_size = mlt_audio_format_size(*format, *samples, *channels);
            int new_size = mlt_audio_format_size(*format, *samples, lv2context->channels);
            uint8_t *new_buffer = mlt_pool_alloc(new_size);
            memcpy(new_buffer, *buffer, old_size);
            // Put silence in extra channels.
            memset(new_buffer + old_size, 0, new_size - old_size);
            mlt_frame_set_audio(frame, new_buffer, *format, new_size, mlt_pool_release);
            *buffer = new_buffer;
        }

        for (i = 0; i < plugin->desc->control_port_count; i++) {
            // Apply the control port values
            char key[20];
            value = plugin->desc->def_values[plugin->desc->control_port_indicies[i]];
            snprintf(key, sizeof(key), "%d", (int) plugin->desc->control_port_indicies[i]);

            if (mlt_properties_get(filter_properties, key))
                value = mlt_properties_anim_get_double(filter_properties, key, position, length);
            for (c = 0; c < plugin->copies; c++) {
                plugin->holders[c].control_memory[i] = value;
            }
        }

        plugin->wet_dry_enabled = mlt_properties_get(filter_properties, "wetness") != NULL;
        if (plugin->wet_dry_enabled) {
            value = mlt_properties_anim_get_double(filter_properties, "wetness", position, length);
            for (c = 0; c < lv2context->channels; c++)
                plugin->wet_dry_values[c] = value;
        }

        // Configure the buffers
        LADSPA_Data **input_buffers = mlt_pool_alloc(sizeof(LADSPA_Data *) * lv2context->channels);
        LADSPA_Data **output_buffers = mlt_pool_alloc(sizeof(LADSPA_Data *) * lv2context->channels);

        // Some plugins crash with too many frames (samples).
        // So, feed the plugin with N samples per loop iteration.
        int samples_offset = 0;
        int sample_count = MIN(*samples, MAX_SAMPLE_COUNT);
        for (i = 0; samples_offset < *samples; i++) {
            int j = 0;
            for (; j < lv2context->channels; j++)
                output_buffers[j] = input_buffers[j] = (LADSPA_Data *) *buffer + j * (*samples)
                                                       + samples_offset;
            sample_count = MIN(*samples - samples_offset, MAX_SAMPLE_COUNT);
            // Do LV2 processing
            error = process_lv2(lv2context->procinfo, sample_count, input_buffers, output_buffers);

            samples_offset += MAX_SAMPLE_COUNT;
        }

        mlt_pool_release(input_buffers);
        mlt_pool_release(output_buffers);

        // read the status port values
        for (i = 0; i < plugin->desc->status_port_count; i++) {
            char key[20];
            int p = plugin->desc->status_port_indicies[i];
            for (c = 0; c < plugin->copies; c++) {
                snprintf(key, sizeof(key), "%d[%d]", p, c);
                value = plugin->holders[c].status_memory[i];
                mlt_properties_set_double(filter_properties, key, value);
            }
        }
    } else {
        // Nothing to do.
        error = mlt_frame_get_audio(frame, buffer, format, frequency, channels, samples);
    }

    return error;
}

/** Filter processing.
*/

static mlt_frame filter_process(mlt_filter this, mlt_frame frame)
{
    if (mlt_frame_is_test_audio(frame) == 0) {
        mlt_frame_push_audio(frame, this);
        mlt_frame_push_audio(frame, lv2_get_audio);
    }

    return frame;
}

/** Constructor for the filter.
*/
mlt_filter filter_lv2_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg)
{
    mlt_filter this = mlt_filter_new();
    /* mlt_filter this = mlt_factory_filter(profile, id, arga); */

    if (this != NULL) {
        mlt_properties properties = MLT_FILTER_PROPERTIES(this);
        this->process = filter_process;
        mlt_properties_set(properties, "resource", arg);
        if (!strncmp(id, "lv2.", 4)) {
            mlt_properties_set(properties, "_pluginid", id + 4);
        }
        mlt_properties_set_int(properties, "channel_mask", -1);
    }

    return this;
}
