/*
 *   jack-ladspa-host
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

#include <jack/jack.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>

#include "process.h"
#include "control_message.h"
#include "lock_free_fifo.h"
#include "plugin.h"
#include "jack_rack.h"
#include "ui.h"

#ifndef _
#define _(x) x
#endif

#define USEC_PER_SEC         1000000
#define MSEC_PER_SEC         1000
#define TIME_RUN_SKIP_COUNT  5

jack_nframes_t sample_rate;
jack_nframes_t buffer_size;

static char * jack_client_name;

int process_control_messages (process_info_t * procinfo) {
  static int quitting = 0;
  ctrlmsg_t ctrlmsg;
  int err = 0;
  
  if (quitting) return quitting;
  
  while (lff_read (procinfo->ui_to_process, &ctrlmsg) == 0)
    {
    switch (ctrlmsg.type)
      {
    
      /* add a link to the end of the plugin chain */
      case CTRLMSG_ADD:
        process_add_plugin (procinfo, ctrlmsg.data.add.plugin);
        err = lff_write (procinfo->process_to_ui, &ctrlmsg);
        break;
        
      /* remove a link (plugin will be sent back) */
      case CTRLMSG_REMOVE:
        ctrlmsg.data.remove.plugin =
          process_remove_plugin (procinfo, ctrlmsg.data.remove.plugin);
        err = lff_write (procinfo->process_to_ui, &ctrlmsg);
        break;
        
      case CTRLMSG_QUIT:
        quitting = 1;
        err = lff_write (procinfo->process_to_ui, &ctrlmsg);
        return 1;
      }
    
    if (err)
      {
        fprintf (stderr, "%s: gui fifo is out of space\n", __FUNCTION__);
        return err;
      }
      
    }
    
  return 0;
}

/** process messages for plugins' control ports */
void process_control_port_messages (process_info_t * procinfo) {
  plugin_t * plugin;
  unsigned long control;
  unsigned long channel;
  gint copy;
  
  if (!procinfo->chain) return;
  
  for (plugin = procinfo->chain; plugin; plugin = plugin->next)
    {
      if (plugin->desc->control_port_count > 0)
        for (control = 0; control < plugin->desc->control_port_count; control++)
          for (copy = 0; copy < plugin->copies; copy++)
            {
              while (lff_read (plugin->holders[copy].ui_control_fifos + control,
                               plugin->holders[copy].control_memory + control) == 0);
            }
      
      if (plugin->wet_dry_enabled)
        for (channel = 0; channel < procinfo->channels; channel++)
          {
            while (lff_read (plugin->wet_dry_fifos + channel,
                             plugin->wet_dry_values + channel) == 0);
          }
    }
}

int get_jack_buffers (process_info_t * procinfo, jack_nframes_t frames) {
  unsigned long channel;
  
  for (channel = 0; channel < procinfo->channels; channel++)
    {
      procinfo->jack_input_buffers[channel] = jack_port_get_buffer (procinfo->jack_input_ports[channel], frames);
      if (!procinfo->jack_input_buffers[channel])
        {
          fprintf (stderr, "%s: no jack buffer for input port %ld\n", __FUNCTION__, channel);
          return 1;
        }

      procinfo->jack_output_buffers[channel] = jack_port_get_buffer (procinfo->jack_output_ports[channel], frames);
      if (!procinfo->jack_output_buffers[channel])
        {
          fprintf (stderr, "%s: no jack buffer for output port %ld\n", __FUNCTION__, channel);
          return 1;
        }
    }

  return 0;
}

plugin_t *
get_first_enabled_plugin (process_info_t * procinfo)
{
  plugin_t * first_enabled;
  
  if (!procinfo->chain) return NULL;

  for (first_enabled = procinfo->chain;
       first_enabled;
       first_enabled = first_enabled->next)
    {
      if (first_enabled->enabled) return first_enabled;
    }
 
  return NULL;
}

plugin_t *
get_last_enabled_plugin (process_info_t * procinfo)
{
  plugin_t * last_enabled;
  
  if (!procinfo->chain) return NULL;

  for (last_enabled = procinfo->chain_end;
       last_enabled;
       last_enabled = last_enabled->prev)
    {
      if (last_enabled->enabled) return last_enabled;
    }
  
  return NULL;
}

void
connect_chain (process_info_t * procinfo, jack_nframes_t frames)
{
  plugin_t * first_enabled, * last_enabled, * plugin;
  gint copy;
  unsigned long channel;
  if (!procinfo->chain) return;
  
  first_enabled = get_first_enabled_plugin (procinfo);
  if (!first_enabled) return;
  
  last_enabled = get_last_enabled_plugin (procinfo);
  
  /* sort out the aux ports */
  plugin = first_enabled;
  do
    {
      if (plugin->desc->aux_channels > 0)
        {
          if (plugin->enabled)
            for (copy = 0; copy < plugin->copies; copy++)
              for (channel = 0; channel < plugin->desc->aux_channels; channel++)
                plugin->descriptor->
                  connect_port (plugin->holders[copy].instance,
                                plugin->desc->audio_aux_port_indicies[channel],
                                jack_port_get_buffer (plugin->holders[copy].aux_ports[channel], frames));
        }
      
    }
  while ( (plugin != last_enabled) && (plugin = plugin->next) );

  /* ensure that all the of the enabled plugins are connected to their memory */
  plugin_connect_output_ports (first_enabled);
  if (first_enabled != last_enabled)
    {
      plugin_connect_input_ports (last_enabled, last_enabled->prev->audio_output_memory);
      
      for (plugin = first_enabled->next;
           plugin;
           plugin = plugin->next)
        {
          if (plugin->enabled)
            {
              plugin_connect_input_ports (plugin, plugin->prev->audio_output_memory);
              plugin_connect_output_ports (plugin);
            }
        }
    }

  /* input buffers for first plugin */
  plugin_connect_input_ports (first_enabled, procinfo->jack_input_buffers);
}

void
process_chain (process_info_t * procinfo, jack_nframes_t frames)
{
  LADSPA_Data zero_signal[frames];
  plugin_t * first_enabled;
  plugin_t * last_enabled = NULL;
  plugin_t * plugin;
  unsigned long channel;
  unsigned long i;
  guint copy;

  /* set the zero signal to zero */
  for (channel = 0; channel < frames; channel++)
    zero_signal[channel] = 0.0;
    
  /* possibly set aux output channels to zero if they're not enabled */
  for (plugin = procinfo->chain; plugin; plugin = plugin->next)
    if (!plugin->enabled &&
        plugin->desc->aux_channels > 0 &&
        !plugin->desc->aux_are_input)
      for (copy = 0; copy < plugin->copies; copy++)
        for (channel = 0; channel < plugin->desc->aux_channels; channel++)
          memcpy (jack_port_get_buffer (plugin->holders[copy].aux_ports[channel], frames),
                  zero_signal, sizeof (LADSPA_Data) * frames);

  first_enabled = get_first_enabled_plugin (procinfo);
  
  /* no chain; just copy input to output */
  if (!procinfo->chain || !first_enabled)
    {
      unsigned long channel;
      for (channel = 0; channel < procinfo->channels; channel++)
        {
          memcpy (procinfo->jack_output_buffers[channel],
                  procinfo->jack_input_buffers[channel],
                  sizeof(LADSPA_Data) * frames);
        }
      return;
    }
  
  /* all past here is guaranteed to have at least 1 enabled plugin */

  last_enabled = get_last_enabled_plugin (procinfo);
  
  for (plugin = first_enabled;
       plugin;
       plugin = plugin->next)
    {
      if (plugin->enabled)
        {
          for (i = 0; i < plugin->copies; i++)
            plugin->descriptor->run (plugin->holders[i].instance, frames);
          
          if (plugin->wet_dry_enabled)
            for (channel = 0; channel < procinfo->channels; channel++)
              for (i = 0; i < frames; i++)
                {
                  plugin->audio_output_memory[channel][i] *= plugin->wet_dry_values[channel];
                  plugin->audio_output_memory[channel][i] += plugin->audio_input_memory[channel][i] * (1.0 - plugin->wet_dry_values[channel]);
                }
          
          if (plugin == last_enabled)
            break;
        }
      else
        {
    
          /* copy the data through */
          for (i = 0; i < procinfo->channels; i++)
            memcpy (plugin->audio_output_memory[i],
                    plugin->prev->audio_output_memory[i],
                    sizeof(LADSPA_Data) * frames);
        }
    }
  
  /* copy the last enabled data to the jack ports */
  for (i = 0; i < procinfo->channels; i++)
    memcpy (procinfo->jack_output_buffers[i],
            last_enabled->audio_output_memory[i],
            sizeof(LADSPA_Data) * frames);
  
}

int process (jack_nframes_t frames, void * data) {
  int err, quitting;
  process_info_t * procinfo;
  
  procinfo = (process_info_t *) data;
  
  if (!procinfo)
    {
      fprintf (stderr, "%s: no process_info from jack!\n", __FUNCTION__);
      return 1;
    }
  
  if (procinfo->port_count == 0)
    return 0;
  
  quitting = process_control_messages (procinfo);
  
  if (quitting)
    return 1;
  
  process_control_port_messages (procinfo);
  
  err = get_jack_buffers (procinfo, frames);
  if (err)
    {
      fprintf(stderr, "%s: failed to get jack ports, not processing\n", __FUNCTION__);
      return 0;
    }
  
  connect_chain (procinfo, frames);
  
  process_chain (procinfo, frames);
  
  return 0;
}



/*******************************************
 ************** non RT stuff ***************
 *******************************************/
 
static int
process_info_connect_jack (process_info_t * procinfo, ui_t * ui)
{
  printf (_("Connecting to JACK server with client name '%s'\n"), jack_client_name);

  procinfo->jack_client = jack_client_new (jack_client_name);

  if (!procinfo->jack_client)
    {
      fprintf (stderr, "%s: could not create jack client; is the jackd server running?\n", __FUNCTION__);
      return 1;
    }

  printf (_("Connected to JACK server\n"));

  jack_set_process_callback (procinfo->jack_client, process, procinfo);
  jack_on_shutdown (procinfo->jack_client, jack_shutdown_cb, ui);
                                            
  return 0;
}

static void
process_info_connect_port (process_info_t * procinfo,
                           ui_t * ui,
                           gshort in,
                           unsigned long port_index,
                           const char * port_name)
{
  const char ** jack_ports;
  unsigned long jack_port_index;
  int err;
  char * full_port_name;
  
  jack_ports = jack_get_ports (procinfo->jack_client, NULL, NULL,
                               JackPortIsPhysical | (in ? JackPortIsOutput : JackPortIsInput));
  
  if (!jack_ports)
    return;
  
  for (jack_port_index = 0;
       jack_ports[jack_port_index] && jack_port_index <= port_index;
       jack_port_index++)
    {
      if (jack_port_index != port_index)
        continue;
        
      full_port_name = g_strdup_printf ("%s:%s", jack_client_name, port_name);

      printf (_("Connecting ports '%s' and '%s'\n"), full_port_name, jack_ports[jack_port_index]);

      err = jack_connect (procinfo->jack_client,
                          in ? jack_ports[jack_port_index] : full_port_name,
                          in ? full_port_name : jack_ports[jack_port_index]);

      if (err)
        fprintf (stderr, "%s: error connecting ports '%s' and '%s'\n",
                 __FUNCTION__, full_port_name, jack_ports[jack_port_index]);
      else
        printf (_("Connected ports '%s' and '%s'\n"), full_port_name, jack_ports[jack_port_index]);
      
      free (full_port_name);
    }
  
  free (jack_ports);
}

int
process_info_set_port_count (process_info_t * procinfo, ui_t * ui, 
	unsigned long port_count, gboolean connect_inputs, gboolean connect_outputs)
{
  unsigned long i;
  char * port_name;
  jack_port_t ** port_ptr;
  gshort in;
  
  if (procinfo->port_count >= port_count)
      return;
  
  if (procinfo->port_count == 0)
    {
      procinfo->jack_input_ports = g_malloc (sizeof (jack_port_t *) * port_count);
      procinfo->jack_output_ports = g_malloc (sizeof (jack_port_t *) * port_count);
      
      procinfo->jack_input_buffers = g_malloc (sizeof (LADSPA_Data *) * port_count);
      procinfo->jack_output_buffers = g_malloc (sizeof (LADSPA_Data *) * port_count);
    }
  else
    {
      procinfo->jack_input_ports = g_realloc (procinfo->jack_input_ports, sizeof (jack_port_t *) * port_count);
      procinfo->jack_output_ports = g_realloc (procinfo->jack_output_ports, sizeof (jack_port_t *) * port_count);

      procinfo->jack_input_buffers = g_realloc (procinfo->jack_input_buffers, sizeof (LADSPA_Data *) * port_count);
      procinfo->jack_output_buffers = g_realloc (procinfo->jack_output_buffers, sizeof (LADSPA_Data *) * port_count);
    }
  
  for (i = procinfo->port_count; i < port_count; i++)
    {
    for (in = 0; in < 2; in++)
      {
        port_name = g_strdup_printf ("%s_%ld", in ? "in" : "out", i + 1);
       
        //printf (_("Creating %s port %s\n"), in ? "input" : "output", port_name);
        
        port_ptr = (in ? &procinfo->jack_input_ports[i]
                       : &procinfo->jack_output_ports[i]);
        
        *port_ptr =  jack_port_register (procinfo->jack_client,
                                         port_name,
                                         JACK_DEFAULT_AUDIO_TYPE,
                                         in ? JackPortIsInput : JackPortIsOutput,
                                         0);
        
        if (!*port_ptr)
          {
            fprintf (stderr, "%s: could not register port '%s'; aborting\n",
                     __FUNCTION__, port_name);
            return 1;
          }

        //printf (_("Created %s port %s\n"), in ? "input" : "output", port_name);
        
        if ((in && connect_inputs) || (!in && connect_outputs))
          process_info_connect_port (procinfo, ui, in, i, port_name);
        
        g_free (port_name);
      }
    }
  
  procinfo->port_count = port_count;

  return 0;
}

void
process_info_set_channels (process_info_t * procinfo, ui_t * ui, 
	unsigned long channels, gboolean connect_inputs, gboolean connect_outputs)
{
  process_info_set_port_count (procinfo, ui, channels, connect_inputs, connect_outputs);
  procinfo->channels = channels; 
}

process_info_t *
process_info_new (ui_t * ui, const char * client_name, unsigned long rack_channels, 
	gboolean connect_inputs, gboolean connect_outputs)
{
  process_info_t * procinfo;
  int err;

  procinfo = g_malloc (sizeof (process_info_t));
  
  procinfo->chain = NULL;
  procinfo->chain_end = NULL;
  procinfo->jack_client = NULL;
  procinfo->port_count = 0;
  procinfo->jack_input_ports = NULL;
  procinfo->jack_output_ports = NULL;
  procinfo->channels = rack_channels;
	
  /* sort out the client name */
  jack_client_name = strdup (client_name);
  for (err = 0; jack_client_name[err] != '\0'; err++)
    {
      if (jack_client_name[err] == ' ')
        jack_client_name[err] = '_';
      else if (!isalnum (jack_client_name[err]))
        { /* shift all the chars up one (to remove the non-alphanumeric char) */
          int i;
          for (i = err; jack_client_name[i] != '\0'; i++)
            jack_client_name[i] = jack_client_name[i + 1];
        }
      else if (isupper (jack_client_name[err]))
        jack_client_name[err] = tolower (jack_client_name[err]);
    }
  
  err = process_info_connect_jack (procinfo, ui);
  if (err)
    {
/*      g_free (procinfo); */
      return NULL;
/*      abort (); */
    }
  
  sample_rate = jack_get_sample_rate (procinfo->jack_client);
  buffer_size = jack_get_sample_rate (procinfo->jack_client);
  
  jack_set_process_callback (procinfo->jack_client, process, procinfo);
  jack_on_shutdown (procinfo->jack_client, jack_shutdown_cb, ui);
  
  procinfo->ui_to_process = ui->ui_to_process; 
  procinfo->process_to_ui = ui->process_to_ui; 
  
  jack_activate (procinfo->jack_client);

  err = process_info_set_port_count (procinfo, ui, rack_channels, connect_inputs, connect_outputs);
  if (err)
    return NULL;

  return procinfo;
}

void
process_info_destroy (process_info_t * procinfo) {
  jack_deactivate (procinfo->jack_client);
  jack_client_close (procinfo->jack_client);
  g_free (procinfo);
}
