/*
 *   JACK Rack
 *    
 *   Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
 *    
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __JLH_PROCESS_H__
#define __JLH_PROCESS_H__

#include <glib.h>
#include <jack/jack.h>
#include <ladspa.h>

#include "lock_free_fifo.h"

typedef struct _process_info process_info_t;

/** this is what gets passed to the process() callback and contains all
    the data the process callback will need */
struct _process_info {

  /** the plugin instance chain */
  struct _plugin * chain;
  struct _plugin * chain_end;
  
  jack_client_t * jack_client;
  unsigned long port_count;
  jack_port_t ** jack_input_ports;
  jack_port_t ** jack_output_ports;

  unsigned long channels;
  LADSPA_Data ** jack_input_buffers;
  LADSPA_Data ** jack_output_buffers;
  
  lff_t * ui_to_process;
  lff_t * process_to_ui;
};

extern jack_nframes_t sample_rate;
extern jack_nframes_t buffer_size;

struct _ui;

process_info_t * process_info_new (struct _ui * ui, const char * client_name, 
	unsigned long rack_channels, gboolean connect_inputs, gboolean connect_outputs);
void process_info_destroy (process_info_t * procinfo);

void process_info_set_channels (process_info_t * procinfo, struct _ui * ui, 
	unsigned long channels, gboolean connect_inputs, gboolean connect_outputs);


int process (jack_nframes_t frames, void * data);


#endif /* __JLH_PROCESS_H__ */
