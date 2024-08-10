/*
 * producer_vst2.c -- VST2 plugin producer
 * Copyright (C) 2024 Meltytech, LLC
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

#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>
#include <framework/mlt_producer.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <string.h>

#include "vst2_context.h"

#define BUFFER_LEN 10000

/** One-time initialization of jack rack.
*/

static vst2_context_t *initialise_vst2_context(mlt_properties properties, int channels)
{
    vst2_context_t *vst2context = NULL;
    unsigned long plugin_id = mlt_properties_get_int64(properties, "_pluginid");

    // Start Vst2context
    if (plugin_id) {
        // Create Vst2context without Jack client name so that it only uses LADSPA
        vst2context = vst2_context_new(NULL, channels);
        mlt_properties_set_data(properties,
                                "_vst2context",
                                vst2context,
                                0,
                                (mlt_destructor) vst2_context_destroy,
                                NULL);

        // Load one LADSPA plugin by its UniqueID
        vst2_plugin_desc_t *desc = vst2_mgr_get_any_desc(vst2context->plugin_mgr, plugin_id);
        vst2_plugin_t *plugin;

        if (desc && (plugin = vst2_context_instantiate_plugin(vst2context, desc))) {
            plugin->enabled = TRUE;
            plugin->wet_dry_enabled = FALSE;
            vst2_process_add_plugin(vst2context->procinfo, plugin);
            mlt_properties_set_int(properties, "instances", plugin->copies);
        } else {
            mlt_log_error(properties, "failed to load plugin %lu\n", plugin_id);
        }
    }

    return vst2context;
}

static int producer_get_audio(mlt_frame frame,
                              void **buffer,
                              mlt_audio_format *format,
                              int *frequency,
                              int *channels,
                              int *samples)
{
    // Get the producer service
    mlt_producer producer = mlt_properties_get_data(MLT_FRAME_PROPERTIES(frame),
                                                    "_producer_vst2",
                                                    NULL);
    mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES(producer);
    int size = 0;
    LADSPA_Data **output_buffers = NULL;
    int i = 0;

    // Initialize VST2 if needed
    vst2_context_t *vst2context = mlt_properties_get_data(producer_properties, "_vst2context", NULL);
    if (!vst2context) {
        vst2_sample_rate = *frequency; // global inside jack_rack
        vst2context = initialise_vst2_context(producer_properties, *channels);
    }

    if (vst2context) {
        // Correct the returns if necessary
        *samples = *samples <= 0 ? 1920 : *samples;
        *channels = *channels <= 0 ? 2 : *channels;
        *frequency = *frequency <= 0 ? 48000 : *frequency;
        *format = mlt_audio_float;

        if (vst2context->procinfo && vst2context->procinfo->chain) {
            vst2_plugin_t *plugin = vst2context->procinfo->chain;
            LADSPA_Data value;
            int index, c;
            mlt_position position = mlt_frame_get_position(frame);
            mlt_position length = mlt_producer_get_length(producer);

            for (index = 0; index < plugin->desc->control_port_count; index++) {
                // Apply the control port values
                char key[20];
                value = vst2_plugin_desc_get_default_control_value(plugin->desc, index, vst2_sample_rate);
                snprintf(key, sizeof(key), "%d", index);
                if (mlt_properties_get(producer_properties, key))
                    value = mlt_properties_anim_get_double(producer_properties,
                                                           key,
                                                           position,
                                                           length);
                for (c = 0; c < plugin->copies; c++)
		  {
		    if (plugin->holders[c].control_memory[index] != value)
		      {
			plugin->holders[c].control_memory[index] = value;
			plugin->holders[c].effect
			  ->setParameter (plugin->holders[c].effect,
					  plugin->desc->control_port_indicies[index]
					  -
					  (plugin->holders[c].effect->numInputs+plugin->holders[c].effect->numOutputs),
					  plugin->holders[c].control_memory[index]);
		      }
		  }
            }
        }

        // Calculate the size of the buffer
        size = *samples * *channels * sizeof(float);

        // Allocate the buffer
        *buffer = mlt_pool_alloc(size);

        // Initialize the LADSPA output buffer.
        output_buffers = mlt_pool_alloc(sizeof(LADSPA_Data *) * *channels);
        for (i = 0; i < *channels; i++) {
            output_buffers[i] = (LADSPA_Data *) *buffer + i * *samples;
        }

        // Do VST2 processing
        process_vst2(vst2context->procinfo, *samples, NULL, output_buffers);
        mlt_pool_release(output_buffers);

        // Set the buffer for destruction
        mlt_frame_set_audio(frame, *buffer, *format, size, mlt_pool_release);

        if (vst2context && vst2context->procinfo && vst2context->procinfo->chain
            && mlt_properties_get_int64(producer_properties, "_pluginid")) {
            vst2_plugin_t *plugin = vst2context->procinfo->chain;
            LADSPA_Data value;
            int i, c;
            for (i = 0; i < plugin->desc->status_port_count; i++) {
                // read the status port values
                char key[20];
                int p = plugin->desc->status_port_indicies[i];
                for (c = 0; c < plugin->copies; c++) {
                    snprintf(key, sizeof(key), "%d[%d]", p, c);
                    value = plugin->holders[c].status_memory[i];
                    mlt_properties_set_double(producer_properties, key, value);
                }
            }
        }
    }

    return 0;
}

static int producer_get_frame(mlt_producer producer, mlt_frame_ptr frame, int index)
{
    // Generate a frame
    *frame = mlt_frame_init(MLT_PRODUCER_SERVICE(producer));

    // Check that we created a frame and initialize it
    if (*frame != NULL) {
        // Obtain properties of frame
        mlt_properties frame_properties = MLT_FRAME_PROPERTIES(*frame);

        // Update timecode on the frame we're creating
        mlt_frame_set_position(*frame, mlt_producer_position(producer));

        // Save the producer to be used in get_audio
        mlt_properties_set_data(frame_properties, "_producer_vst2", producer, 0, NULL, NULL);

        // Push the get_audio method
        mlt_frame_push_audio(*frame, producer_get_audio);
    }

    // Calculate the next time code
    mlt_producer_prepare_next(producer);

    return 0;
}

/** Destructor for the producer.
*/

static void producer_close(mlt_producer producer)
{
    producer->close = NULL;
    mlt_producer_close(producer);
    free(producer);
}

/** Constructor for the producer.
*/

mlt_producer producer_vst2_init(mlt_profile profile,
				mlt_service_type type,
				const char *id,
				char *arg)
{
    // Create a new producer object
    mlt_producer producer = mlt_producer_new(profile);

    if (producer != NULL) {
        mlt_properties properties = MLT_PRODUCER_PROPERTIES(producer);

        producer->get_frame = producer_get_frame;
        producer->close = (mlt_destructor) producer_close;

        // Save the plugin ID.
        if (!strncmp(id, "vst2.", 5)) {
            mlt_properties_set(properties, "_pluginid", id + 5);
        }

        // Make sure the plugin ID is valid.
        unsigned long plugin_id = mlt_properties_get_int64(properties, "_pluginid");
        if (plugin_id < 1000 || plugin_id > 0x00FFFFFF) {
            producer_close(producer);
            producer = NULL;
        }
    }
    return producer;
}
