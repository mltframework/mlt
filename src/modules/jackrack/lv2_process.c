/*
 * JACK Rack
 *
 * Original:
 * Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
 *
 * Modification for MLT:
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <pthread.h>
/* #ifdef WITH_JACK
   #include <jack/jack.h>
   #endif */
#include <ctype.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
    #include <gettimeofday.h>
#else
    #include <sys/time.h>
#endif
#include <time.h>

#include "framework/mlt_log.h"
#include "lock_free_fifo.h"
#include "lv2_context.h"
#include "lv2_plugin.h"
#include "lv2_process.h"

#ifndef _
#define _(x) x
#endif

extern pthread_mutex_t g_activate_mutex;

#define USEC_PER_SEC 1000000
#define MSEC_PER_SEC 1000
#define TIME_RUN_SKIP_COUNT 5
#define MAX_BUFFER_SIZE 4096

jack_nframes_t lv2_sample_rate;
jack_nframes_t lv2_buffer_size;

/* #ifdef WITH_JACK
   
   static void
   jack_shutdown_cb (void * data)
   {
     lv2_process_info_t * procinfo = data;
     
     procinfo->quit = TRUE;
   }
   
   #endif */

/** process messages for plugins' control ports */
void lv2_process_control_port_messages(lv2_process_info_t *procinfo)
{
    lv2_plugin_t *plugin;
    unsigned long control;
    unsigned long channel;
    gint copy;

    if (!procinfo->chain)
        return;

    for (plugin = procinfo->chain; plugin; plugin = plugin->next) {
        if (plugin->desc->control_port_count > 0)
            for (control = 0; control < plugin->desc->control_port_count; control++)
                for (copy = 0; copy < plugin->copies; copy++) {
                    while (lff_read(plugin->holders[copy].ui_control_fifos + control,
                                    plugin->holders[copy].control_memory + control)
                           == 0)
                        ;
                }

        if (plugin->wet_dry_enabled)
            for (channel = 0; channel < procinfo->channels; channel++) {
                while (lff_read(plugin->wet_dry_fifos + channel, plugin->wet_dry_values + channel)
                       == 0)
                    ;
            }
    }
}

/* #ifdef WITH_JACK
   
   static int get_jack_buffers (lv2_process_info_t * procinfo, jack_nframes_t frames) {
     unsigned long channel;
     
     for (channel = 0; channel < procinfo->channels; channel++)
       {
         procinfo->jack_input_buffers[channel] = jack_port_get_buffer (procinfo->jack_input_ports[channel], frames);
         if (!procinfo->jack_input_buffers[channel])
           {
             mlt_log_verbose( NULL, "%s: no jack buffer for input port %ld\n", __FUNCTION__, channel);
             return 1;
           }
   
         procinfo->jack_output_buffers[channel] = jack_port_get_buffer (procinfo->jack_output_ports[channel], frames);
         if (!procinfo->jack_output_buffers[channel])
           {
             mlt_log_verbose( NULL, "%s: no jack buffer for output port %ld\n", __FUNCTION__, channel);
             return 1;
           }
       }
   
     return 0;
   }
   
   #endif */

lv2_plugin_t *lv2_get_first_enabled_plugin(lv2_process_info_t *procinfo)
{
    lv2_plugin_t *first_enabled;

    if (!procinfo->chain)
        return NULL;

    for (first_enabled = procinfo->chain; first_enabled; first_enabled = first_enabled->next) {
        if (first_enabled->enabled)
            return first_enabled;
    }

    return NULL;
}

lv2_plugin_t *lv2_get_last_enabled_plugin(lv2_process_info_t *procinfo)
{
    lv2_plugin_t *last_enabled;

    if (!procinfo->chain)
        return NULL;

    for (last_enabled = procinfo->chain_end; last_enabled; last_enabled = last_enabled->prev) {
        if (last_enabled->enabled)
            return last_enabled;
    }

    return NULL;
}

void lv2_connect_chain(lv2_process_info_t *procinfo, jack_nframes_t frames)
{
    lv2_plugin_t *first_enabled, *last_enabled, *plugin;
    gint copy;
    unsigned long channel;
    if (!procinfo->chain)
        return;

    first_enabled = lv2_get_first_enabled_plugin(procinfo);
    if (!first_enabled)
        return;

    last_enabled = lv2_get_last_enabled_plugin(procinfo);

    /* sort out the aux ports */
    plugin = first_enabled;
    do {
        if (plugin->desc->aux_channels > 0 && plugin->enabled) {
#ifdef WITH_JACK
            if (procinfo->jack_client) {
                for (copy = 0; copy < plugin->copies; copy++)
                    for (channel = 0; channel < plugin->desc->aux_channels; channel++)
                        lilv_instance_connect_port(plugin->holders[copy].instance,
                                                   plugin->desc->audio_aux_port_indicies[channel],
                                                   jack_port_get_buffer(plugin->holders[copy]
                                                                            .aux_ports[channel],
                                                                        frames));
            } else
#endif
            {
                for (copy = 0; copy < frames; copy++)
                    procinfo->silent_buffer[copy] = 0.0;

                for (copy = 0; copy < plugin->copies; copy++)
                    for (channel = 0; channel < plugin->desc->aux_channels; channel++)
                        lilv_instance_connect_port(plugin->holders[copy].instance,
                                                   plugin->desc->audio_aux_port_indicies[channel],
                                                   procinfo->silent_buffer);
            }
        }
    } while ((plugin != last_enabled) && (plugin = plugin->next));

    /* ensure that all the of the enabled plugins are connected to their memory */
    lv2_plugin_connect_output_ports(first_enabled);
    if (first_enabled != last_enabled) {
        lv2_plugin_connect_input_ports(last_enabled, last_enabled->prev->audio_output_memory);
        for (plugin = first_enabled->next; plugin; plugin = plugin->next) {
            if (plugin->enabled) {
                lv2_plugin_connect_input_ports(plugin, plugin->prev->audio_output_memory);
                lv2_plugin_connect_output_ports(plugin);
            }
        }
    }

    /* input buffers for first plugin */
    if (plugin->desc->has_input)
        lv2_plugin_connect_input_ports(first_enabled, procinfo->jack_input_buffers);
}

void lv2_process_chain(lv2_process_info_t *procinfo, jack_nframes_t frames)
{
    lv2_plugin_t *first_enabled;
    lv2_plugin_t *last_enabled = NULL;
    lv2_plugin_t *plugin;
    unsigned long channel;
    unsigned long i;

#ifdef WITH_JACK
    if (procinfo->jack_client) {
        LADSPA_Data zero_signal[frames];
        guint copy;

        /* set the zero signal to zero */
        for (channel = 0; channel < frames; channel++)
            zero_signal[channel] = 0.0;

        /* possibly set aux output channels to zero if they're not enabled */
        for (plugin = procinfo->chain; plugin; plugin = plugin->next)
            if (!plugin->enabled && plugin->desc->aux_channels > 0 && !plugin->desc->aux_are_input)
                for (copy = 0; copy < plugin->copies; copy++)
                    for (channel = 0; channel < plugin->desc->aux_channels; channel++)
                        memcpy(jack_port_get_buffer(plugin->holders[copy].aux_ports[channel],
                                                    frames),
                               zero_signal,
                               sizeof(LADSPA_Data) * frames);
    }
#endif

    first_enabled = lv2_get_first_enabled_plugin(procinfo);

    /* no chain; just copy input to output */
    if (!procinfo->chain || !first_enabled) {
        unsigned long channel;
        for (channel = 0; channel < procinfo->channels; channel++) {
            memcpy(procinfo->jack_output_buffers[channel],
                   procinfo->jack_input_buffers[channel],
                   sizeof(LADSPA_Data) * frames);
        }
        return;
    }

    /* all past here is guaranteed to have at least 1 enabled plugin */

    last_enabled = lv2_get_last_enabled_plugin(procinfo);

    for (plugin = first_enabled; plugin; plugin = plugin->next) {
        if (plugin->enabled) {
            for (i = 0; i < plugin->copies; i++)
                lilv_instance_run(plugin->holders[i].instance, frames);

            if (plugin->wet_dry_enabled)
                for (channel = 0; channel < procinfo->channels; channel++)
                    for (i = 0; i < frames; i++) {
                        plugin->audio_output_memory[channel][i] *= plugin->wet_dry_values[channel];
                        plugin->audio_output_memory[channel][i]
                            += plugin->audio_input_memory[channel][i]
                               * (1.0 - plugin->wet_dry_values[channel]);
                    }

            if (plugin == last_enabled)
                break;
        } else {
            /* copy the data through */
            for (i = 0; i < procinfo->channels; i++)
                memcpy(plugin->audio_output_memory[i],
                       plugin->prev->audio_output_memory[i],
                       sizeof(LADSPA_Data) * frames);
        }
    }

    /* copy the last enabled data to the jack ports */
    for (i = 0; i < procinfo->channels; i++)
        memcpy(procinfo->jack_output_buffers[i],
               last_enabled->audio_output_memory[i],
               sizeof(LADSPA_Data) * frames);
}

int process_lv2(lv2_process_info_t *procinfo,
                jack_nframes_t frames,
                LADSPA_Data **inputs,
                LADSPA_Data **outputs)
{
    unsigned long channel;

    if (!procinfo) {
        mlt_log_error(NULL, "%s: no process_info from jack!\n", __FUNCTION__);
        return 1;
    }

    if (procinfo->quit == TRUE)
        return 1;

    lv2_process_control_port_messages(procinfo);

    for (channel = 0; channel < procinfo->channels; channel++) {
        if (lv2_get_first_enabled_plugin(procinfo)->desc->has_input) {
            procinfo->jack_input_buffers[channel] = inputs[channel];
            if (!procinfo->jack_input_buffers[channel]) {
                mlt_log_verbose(NULL,
                                "%s: no jack buffer for input port %ld\n",
                                __FUNCTION__,
                                channel);
                return 1;
            }
        }
        procinfo->jack_output_buffers[channel] = outputs[channel];
        if (!procinfo->jack_output_buffers[channel]) {
            mlt_log_verbose(NULL, "%s: no jack buffer for output port %ld\n", __FUNCTION__, channel);
            return 1;
        }
    }

    lv2_connect_chain(procinfo, frames);

    lv2_process_chain(procinfo, frames);

    return 0;
}

#ifdef WITH_JACK

/* int process_jack (jack_nframes_t frames, void * data) {
     int err;
     lv2_process_info_t * procinfo;
     
     procinfo = (lv2_process_info_t *) data;
     
     if (!procinfo)
       {
         mlt_log_error( NULL, "%s: no process_info from jack!\n", __FUNCTION__);
         return 1;
       }
     
     if (procinfo->port_count == 0)
       return 0;
     
     if (procinfo->quit == TRUE)
       return 1;
     
      lv2_process_control_port_messages (procinfo);
     
     err = get_jack_buffers (procinfo, frames);
     if (err)
       {
         mlt_log_warning( NULL, "%s: failed to get jack ports, not processing\n", __FUNCTION__);
         return 0;
       }
     
     lv2_connect_chain (procinfo, frames);
     
     lv2_process_chain (procinfo, frames);
     
     return 0;
   } */

/*******************************************
 ************** non RT stuff ***************
 *******************************************/

/* static int
   lv2_process_info_connect_jack (lv2_process_info_t * procinfo)
   {
     mlt_log_info( NULL, _("Connecting to JACK server with client name '%s'\n"), procinfo->jack_client_name);
   
     procinfo->jack_client = jack_client_open (procinfo->jack_client_name, JackNullOption, NULL);
   
     if (!procinfo->jack_client)
       {
         mlt_log_warning( NULL, "%s: could not create jack client; is the jackd server running?\n", __FUNCTION__);
         return 1;
       }
   
     mlt_log_verbose( NULL, _("Connected to JACK server\n"));
   
     jack_set_process_callback (procinfo->jack_client, process_jack, procinfo);
     jack_on_shutdown (procinfo->jack_client, jack_shutdown_cb, procinfo);
                                               
     return 0;
   } */

static void lv2_process_info_connect_port(lv2_process_info_t *procinfo,
                                          gshort in,
                                          unsigned long port_index,
                                          const char *port_name)
{
    const char **jack_ports;
    unsigned long jack_port_index;
    int err;
    char *full_port_name;

    jack_ports = jack_get_ports(procinfo->jack_client,
                                NULL,
                                NULL,
                                JackPortIsPhysical | (in ? JackPortIsOutput : JackPortIsInput));

    if (!jack_ports)
        return;

    for (jack_port_index = 0; jack_ports[jack_port_index] && jack_port_index <= port_index;
         jack_port_index++) {
        if (jack_port_index != port_index)
            continue;

        full_port_name = g_strdup_printf("%s:%s", procinfo->jack_client_name, port_name);

        mlt_log_debug(NULL,
                      _("Connecting ports '%s' and '%s'\n"),
                      full_port_name,
                      jack_ports[jack_port_index]);

        err = jack_connect(procinfo->jack_client,
                           in ? jack_ports[jack_port_index] : full_port_name,
                           in ? full_port_name : jack_ports[jack_port_index]);

        if (err)
            mlt_log_warning(NULL,
                            "%s: error connecting ports '%s' and '%s'\n",
                            __FUNCTION__,
                            full_port_name,
                            jack_ports[jack_port_index]);
        else
            mlt_log_info(NULL,
                         _("Connected ports '%s' and '%s'\n"),
                         full_port_name,
                         jack_ports[jack_port_index]);

        free(full_port_name);
    }

    free(jack_ports);
}

static int lv2_process_info_set_port_count(lv2_process_info_t *procinfo,
                                           unsigned long port_count,
                                           gboolean connect_inputs,
                                           gboolean connect_outputs)
{
    unsigned long i;
    char *port_name;
    jack_port_t **port_ptr;
    gshort in;

    if (procinfo->port_count >= port_count)
        return -1;

    if (procinfo->port_count == 0) {
        procinfo->jack_input_ports = g_malloc(sizeof(jack_port_t *) * port_count);
        procinfo->jack_output_ports = g_malloc(sizeof(jack_port_t *) * port_count);

        procinfo->jack_input_buffers = g_malloc(sizeof(LADSPA_Data *) * port_count);
        procinfo->jack_output_buffers = g_malloc(sizeof(LADSPA_Data *) * port_count);
    } else {
        procinfo->jack_input_ports = g_realloc(procinfo->jack_input_ports,
                                               sizeof(jack_port_t *) * port_count);
        procinfo->jack_output_ports = g_realloc(procinfo->jack_output_ports,
                                                sizeof(jack_port_t *) * port_count);

        procinfo->jack_input_buffers = g_realloc(procinfo->jack_input_buffers,
                                                 sizeof(LADSPA_Data *) * port_count);
        procinfo->jack_output_buffers = g_realloc(procinfo->jack_output_buffers,
                                                  sizeof(LADSPA_Data *) * port_count);
    }

    for (i = procinfo->port_count; i < port_count; i++) {
        for (in = 0; in < 2; in++) {
            port_name = g_strdup_printf("%s_%ld", in ? "in" : "out", i + 1);

            //mlt_log_debug( NULL, _("Creating %s port %s\n"), in ? "input" : "output", port_name);

            port_ptr = (in ? &procinfo->jack_input_ports[i] : &procinfo->jack_output_ports[i]);

            *port_ptr = jack_port_register(procinfo->jack_client,
                                           port_name,
                                           JACK_DEFAULT_AUDIO_TYPE,
                                           in ? JackPortIsInput : JackPortIsOutput,
                                           0);

            if (!*port_ptr) {
                mlt_log_error(NULL,
                              "%s: could not register port '%s'; aborting\n",
                              __FUNCTION__,
                              port_name);
                return 1;
            }

            //mlt_log_debug( NULL, _("Created %s port %s\n"), in ? "input" : "output", port_name);

            if ((in && connect_inputs) || (!in && connect_outputs))
                lv2_process_info_connect_port(procinfo, in, i, port_name);

            g_free(port_name);
        }
    }

    procinfo->port_count = port_count;

    return 0;
}

#endif

void lv2_process_info_set_channels(lv2_process_info_t *procinfo,
                                   unsigned long channels,
                                   gboolean connect_inputs,
                                   gboolean connect_outputs)
{
#ifdef WITH_JACK
    lv2_process_info_set_port_count(procinfo, channels, connect_inputs, connect_outputs);
#endif
    procinfo->channels = channels;
}

lv2_process_info_t *lv2_process_info_new(const char *client_name,
                                         unsigned long rack_channels,
                                         gboolean connect_inputs,
                                         gboolean connect_outputs)
{
    lv2_process_info_t *procinfo;
    char *jack_client_name;
    int err;

    procinfo = g_malloc(sizeof(lv2_process_info_t));

    procinfo->chain = NULL;
    procinfo->chain_end = NULL;
#ifdef WITH_JACK
    procinfo->jack_client = NULL;
    procinfo->port_count = 0;
    procinfo->jack_input_ports = NULL;
    procinfo->jack_output_ports = NULL;
#endif
    procinfo->channels = rack_channels;
    procinfo->quit = FALSE;

    if (client_name == NULL) {
        lv2_sample_rate = 48000; // should be set externally before calling process_lv2
        lv2_buffer_size = MAX_BUFFER_SIZE;
        procinfo->silent_buffer = g_malloc(sizeof(LADSPA_Data) * lv2_buffer_size);
        procinfo->jack_input_buffers = g_malloc(sizeof(LADSPA_Data *) * rack_channels);
        procinfo->jack_output_buffers = g_malloc(sizeof(LADSPA_Data *) * rack_channels);

        return procinfo;
    }

    /* sort out the client name */
    procinfo->jack_client_name = jack_client_name = strdup(client_name);
    for (err = 0; jack_client_name[err] != '\0'; err++) {
        if (jack_client_name[err] == ' ')
            jack_client_name[err] = '_';
        else if (!isalnum(
                     jack_client_name
                         [err])) { /* shift all the chars up one (to remove the non-alphanumeric char) */
            int i;
            for (i = err; jack_client_name[i] != '\0'; i++)
                jack_client_name[i] = jack_client_name[i + 1];
        } else if (isupper(jack_client_name[err]))
            jack_client_name[err] = tolower(jack_client_name[err]);
    }

    /* #ifdef WITH_JACK
     err = lv2_process_info_connect_jack (procinfo);
     if (err)
       {
   /\*      g_free (procinfo); *\/
         return NULL;
   /\*      abort (); *\/
       }
     
     lv2_sample_rate = jack_get_sample_rate (procinfo->jack_client);
     lv2_buffer_size = jack_get_sample_rate (procinfo->jack_client);
     
     jack_set_process_callback (procinfo->jack_client, process_jack, procinfo);
     pthread_mutex_lock( &g_activate_mutex );
     jack_on_shutdown (procinfo->jack_client, jack_shutdown_cb, procinfo);
     pthread_mutex_unlock( &g_activate_mutex );
     
     jack_activate (procinfo->jack_client);
   
     err = lv2_process_info_set_port_count (procinfo, rack_channels, connect_inputs, connect_outputs);
     if (err)
       return NULL;
   #endif */

    return procinfo;
}

void lv2_process_info_destroy(lv2_process_info_t *procinfo)
{
#ifdef WITH_JACK
    if (procinfo->jack_client) {
        jack_deactivate(procinfo->jack_client);
        jack_client_close(procinfo->jack_client);
    }
    g_free(procinfo->jack_input_ports);
    g_free(procinfo->jack_output_ports);
#endif
    g_free(procinfo->jack_input_buffers);
    g_free(procinfo->jack_output_buffers);
    g_free(procinfo->silent_buffer);
    g_free(procinfo);
}

void lv2_process_quit(lv2_process_info_t *procinfo)
{
    procinfo->quit = TRUE;
}
