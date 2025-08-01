/*
 * JACK Rack
 *
 * Original:
 * Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
 *
 * Modification for MLT:
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __JLH_PROCESS_H__
#define __JLH_PROCESS_H__

#include <glib.h>
#ifdef WITH_JACK
#include <jack/jack.h>
#endif
#include <ladspa.h>

#include "lock_free_fifo.h"

typedef struct _lv2_process_info lv2_process_info_t;

/** this is what gets passed to the process() callback and contains all
    the data the process callback will need */
struct _lv2_process_info
{
    /** the plugin instance chain */
    struct _lv2_plugin *chain;
    struct _lv2_plugin *chain_end;

#ifdef WITH_JACK
    jack_client_t *jack_client;
    unsigned long port_count;
    jack_port_t **jack_input_ports;
    jack_port_t **jack_output_ports;
#endif

    unsigned long channels;
    unsigned long channel_mask;
    LADSPA_Data **jack_input_buffers;
    LADSPA_Data **jack_output_buffers;
    LADSPA_Data *silent_buffer;

    char *jack_client_name;
    int quit;
};

#ifndef WITH_JACK
typedef guint32 jack_nframes_t;
#endif
extern jack_nframes_t lv2_sample_rate;
extern jack_nframes_t lv2_buffer_size;

lv2_process_info_t *lv2_process_info_new(const char *client_name,
                                         unsigned long rack_channels,
                                         gboolean connect_inputs,
                                         gboolean connect_outputs);
void lv2_process_info_destroy(lv2_process_info_t *procinfo);

void lv2_process_info_set_channels(lv2_process_info_t *procinfo,
                                   unsigned long channels,
                                   gboolean connect_inputs,
                                   gboolean connect_outputs);

int process_lv2(lv2_process_info_t *procinfo,
                jack_nframes_t frames,
                LADSPA_Data **inputs,
                LADSPA_Data **outputs);

/* #ifdef WITH_JACK
   int process_jack (jack_nframes_t frames, void * data);
   #endif */

void lv2_process_quit(lv2_process_info_t *procinfo);

#endif /* __JLH_PROCESS_H__ */
