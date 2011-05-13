/*
 * JACK Rack
 *
 * Original:
 * Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
 *
 * Modification for MLT:
 * Copyright (C) 2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <ladspa.h>
#include <dlfcn.h>
#include <ctype.h>

#include <glib.h>

#include "plugin.h"
#include "jack_rack.h"
#include "process.h"
#include "framework/mlt_log.h"

#define CONTROL_FIFO_SIZE   128



/* swap over the jack ports in two plugins */
static void
plugin_swap_aux_ports (plugin_t * plugin, plugin_t * other)
{
  guint copy;
  jack_port_t ** aux_ports_tmp;
  
  for (copy = 0; copy < plugin->copies; copy++)
    {
      aux_ports_tmp = other->holders[copy].aux_ports;
      other->holders[copy].aux_ports = plugin->holders[copy].aux_ports;
      plugin->holders[copy].aux_ports = aux_ports_tmp;
    }
}

/** connect up the ladspa instance's input buffers to the previous
    plugin's audio memory.  make sure to check that plugin->prev
    exists. */
void
plugin_connect_input_ports (plugin_t * plugin, LADSPA_Data ** inputs)
{
  gint copy;
  unsigned long channel;
  unsigned long rack_channel;

  if (!plugin || !inputs)
    return;

  rack_channel = 0;
  for (copy = 0; copy < plugin->copies; copy++)
    {
      for (channel = 0; channel < plugin->desc->channels; channel++)
        {
          plugin->descriptor->
            connect_port (plugin->holders[copy].instance,
                          plugin->desc->audio_input_port_indicies[channel],
                          inputs[rack_channel]);
          rack_channel++;
        }
    }
  
  plugin->audio_input_memory = inputs;
}

/** connect up a plugin's output ports to its own audio_output_memory output memory */
void
plugin_connect_output_ports (plugin_t * plugin)
{
  gint copy;
  unsigned long channel;
  unsigned long rack_channel = 0;

  if (!plugin)
    return;


  for (copy = 0; copy < plugin->copies; copy++)
    {
      for (channel = 0; channel < plugin->desc->channels; channel++)
        {
          plugin->descriptor->
            connect_port (plugin->holders[copy].instance,
                          plugin->desc->audio_output_port_indicies[channel],
                          plugin->audio_output_memory[rack_channel]);
          rack_channel++;
        }
    }
}

void
process_add_plugin (process_info_t * procinfo, plugin_t * plugin)
{

  /* sort out list pointers */
  plugin->next = NULL;
  plugin->prev = procinfo->chain_end;

  if (procinfo->chain_end)
    procinfo->chain_end->next = plugin;
  else
    procinfo->chain = plugin;

  procinfo->chain_end = plugin;

}


/** remove a plugin from the chain */
plugin_t *
process_remove_plugin (process_info_t * procinfo, plugin_t *plugin)
{
  /* sort out chain pointers */
  if (plugin->prev)
    plugin->prev->next = plugin->next;
  else
    procinfo->chain = plugin->next;

  if (plugin->next)
    plugin->next->prev = plugin->prev;
  else
    procinfo->chain_end = plugin->prev;
    
  /* sort out the aux ports */
  if (procinfo->jack_client && plugin->desc->aux_channels > 0)
    {
      plugin_t * other;
      
      for (other = plugin->next; other; other = other->next)
        if (other->desc->id == plugin->desc->id)
          plugin_swap_aux_ports (plugin, other);
    }

  return plugin;
}

/** enable/disable a plugin */
void
process_ablise_plugin (process_info_t * procinfo, plugin_t *plugin, gboolean enable)
{
  plugin->enabled = enable;
}

/** enable/disable a plugin */
void
process_ablise_plugin_wet_dry (process_info_t * procinfo, plugin_t *plugin, gboolean enable)
{
  plugin->wet_dry_enabled = enable;
}

/** move a plugin up or down one place in the chain */
void
process_move_plugin (process_info_t * procinfo, plugin_t *plugin, gint up)
{
  /* other plugins in the chain */
  plugin_t *pp = NULL, *p, *n, *nn = NULL;

  /* note that we should never recieve an illogical move request
     ie, there will always be at least 1 plugin before for an up
     request or 1 plugin after for a down request */

  /* these are pointers to the plugins surrounding the specified one:
     { pp, p, plugin, n, nn } which makes things much clearer than
     tptr, tptr2 etc */
  p = plugin->prev;
  if (p) pp = p->prev;
  n = plugin->next;
  if (n) nn = n->next;

  if (up)
    {
      if (!p)
	return;

      if (pp)
        pp->next = plugin;
      else
        procinfo->chain = plugin;

      p->next = n;
      p->prev = plugin;

      plugin->prev = pp;
      plugin->next = p;

      if (n)
	n->prev = p;
      else
	procinfo->chain_end = p;

    }
  else
    {
      if (!n)
	return;

      if (p)
	p->next = n;
      else
	procinfo->chain = n;

      n->prev = p;
      n->next = plugin;

      plugin->prev = n;
      plugin->next = nn;

      if (nn)
	nn->prev = plugin;
      else
	procinfo->chain_end = plugin;
    }
  
  if (procinfo->jack_client && plugin->desc->aux_channels > 0)
    {
      plugin_t * other;
      other = up ? plugin->next : plugin->prev;
      
      /* swap around the jack ports */
      if (other->desc->id == plugin->desc->id)
        plugin_swap_aux_ports (plugin, other);
    }
}

/** exchange an existing plugin for a newly created one */
plugin_t *
process_change_plugin (process_info_t * procinfo,
	               plugin_t *plugin, plugin_t * new_plugin)
{
  new_plugin->next = plugin->next;
  new_plugin->prev = plugin->prev;

  if (plugin->prev)
    plugin->prev->next = new_plugin;
  else
    procinfo->chain = new_plugin;

  if (plugin->next)
    plugin->next->prev = new_plugin;
  else
    procinfo->chain_end = new_plugin;

  /* sort out the aux ports */
  if (procinfo->jack_client && plugin->desc->aux_channels > 0)
    {
      plugin_t * other;
      
      for (other = plugin->next; other; other = other->next)
        if (other->desc->id == plugin->desc->id)
          plugin_swap_aux_ports (plugin, other);
    }

  return plugin;
}


/******************************************
 ************* non RT stuff ***************
 ******************************************/


static int
plugin_open_plugin (plugin_desc_t * desc,
                    void ** dl_handle_ptr,
                    const LADSPA_Descriptor ** descriptor_ptr)
{
  void * dl_handle;
  const char * dlerr;
  LADSPA_Descriptor_Function get_descriptor;
    
  /* open the object file */
  dl_handle = dlopen (desc->object_file, RTLD_NOW|RTLD_GLOBAL);
  if (!dl_handle)
    {
      mlt_log_warning( NULL, "%s: error opening shared object file '%s': %s\n",
               __FUNCTION__, desc->object_file, dlerror());
      return 1;
    }
  
  
  /* get the get_descriptor function */
  dlerror (); /* clear the error report */
  
  get_descriptor = (LADSPA_Descriptor_Function)
    dlsym (dl_handle, "ladspa_descriptor");
  
  dlerr = dlerror();
  if (dlerr)
    {
      mlt_log_warning( NULL, "%s: error finding descriptor symbol in object file '%s': %s\n",
               __FUNCTION__, desc->object_file, dlerr);
      dlclose (dl_handle);
      return 1;
    }
  
  *descriptor_ptr = get_descriptor (desc->index);
  *dl_handle_ptr = dl_handle;
  
  return 0;
}

static int
plugin_instantiate (const LADSPA_Descriptor * descriptor,
                    unsigned long plugin_index,
                    gint copies,
                    LADSPA_Handle * instances)
{
  gint i;
  
  for (i = 0; i < copies; i++)
    {
      instances[i] = descriptor->instantiate (descriptor, sample_rate);
      
      if (!instances[i])
        {
          unsigned long d;
 
          for (d = 0; d < i; d++)
            descriptor->cleanup (instances[d]);
          
          return 1;
        }
    }
  
  return 0;
}

static void
plugin_create_aux_ports (plugin_t * plugin, guint copy, jack_rack_t * jack_rack)
{
  plugin_desc_t * desc;
//  plugin_slot_t * slot;
  unsigned long aux_channel = 1;
  unsigned long plugin_index = 1;
  unsigned long i;
  char port_name[64];
  char * plugin_name;
  char * ptr;
//  GList * list;
  ladspa_holder_t * holder;
  
  desc = plugin->desc;
  holder = plugin->holders + copy;
  
  holder->aux_ports = g_malloc (sizeof (jack_port_t *) * desc->aux_channels);
  
  /* make the plugin name jack worthy */
  ptr = plugin_name = g_strndup (plugin->desc->name, 7);
  while (*ptr != '\0')
    {
      if (*ptr == ' ')
        *ptr = '_';
      else
        *ptr = tolower (*ptr);
      
      ptr++;
    }

/*	
  for (list = jack_rack->slots; list; list = g_list_next (list))
    {
      slot = (plugin_slot_t *) list->data;
      
      if (slot->plugin->desc->id == plugin->desc->id)
        plugin_index++;
    }
*/
      
  for (i = 0; i < desc->aux_channels; i++, aux_channel++)
    {
      sprintf (port_name, "%s_%ld-%d_%c%ld",
               plugin_name,
               plugin_index,
               copy + 1,
               desc->aux_are_input ? 'i' : 'o',
               aux_channel);
      
      holder->aux_ports[i] =
        jack_port_register (jack_rack->procinfo->jack_client,
                            port_name,
                            JACK_DEFAULT_AUDIO_TYPE,
                            desc->aux_are_input ? JackPortIsInput : JackPortIsOutput,
                            0);
      
      if (!holder->aux_ports[i])
        {
          mlt_log_panic( NULL, "Could not register jack port '%s'; aborting\n", port_name);
          abort ();
        }
    }
  
  g_free (plugin_name);
}

static LADSPA_Data unused_control_port_output;

static void
plugin_init_holder (plugin_t * plugin,
                    guint copy,
                    LADSPA_Handle instance,
                    jack_rack_t * jack_rack)
{
  unsigned long i;
  plugin_desc_t * desc;
  ladspa_holder_t * holder;
  
  desc = plugin->desc;
  holder = plugin->holders + copy;
  
  holder->instance = instance;
  
  if (desc->control_port_count > 0)
    {
      holder->ui_control_fifos    = g_malloc (sizeof (lff_t) * desc->control_port_count);
      holder->control_memory = g_malloc (sizeof (LADSPA_Data) * desc->control_port_count);
    }
  else
    {
      holder->ui_control_fifos  = NULL;
      holder->control_memory = NULL;
    }
  
  for (i = 0; i < desc->control_port_count; i++)
    {
      lff_init (holder->ui_control_fifos + i, CONTROL_FIFO_SIZE, sizeof (LADSPA_Data));
      holder->control_memory[i] =
        plugin_desc_get_default_control_value (desc, desc->control_port_indicies[i], sample_rate);        
      
      plugin->descriptor->
        connect_port (instance, desc->control_port_indicies[i], holder->control_memory + i);
    }
  
  for (i = 0; i < desc->port_count; i++)
    {
      if (!LADSPA_IS_PORT_CONTROL (desc->port_descriptors[i]))
        continue;
      
      if (LADSPA_IS_PORT_OUTPUT (desc->port_descriptors[i]))
        plugin->descriptor-> connect_port (instance, i, &unused_control_port_output);
    }
  
  if (jack_rack->procinfo->jack_client && plugin->desc->aux_channels > 0)
    plugin_create_aux_ports (plugin, copy, jack_rack);
  
  if (plugin->descriptor->activate)
    plugin->descriptor->activate (instance);
}


plugin_t *
plugin_new (plugin_desc_t * desc, jack_rack_t * jack_rack)
{
  void * dl_handle;
  const LADSPA_Descriptor * descriptor;
  LADSPA_Handle * instances;
  gint copies;
  unsigned long i;
  int err;
  plugin_t * plugin;
  
  /* open the plugin */
  err = plugin_open_plugin (desc, &dl_handle, &descriptor);
  if (err)
    return NULL;

  /* create the instances */
  copies = plugin_desc_get_copies (desc, jack_rack->channels);
  instances = g_malloc (sizeof (LADSPA_Handle) * copies);

  err = plugin_instantiate (descriptor, desc->index, copies, instances);
  if (err)
    {
      g_free (instances);
      dlclose (dl_handle);
      return NULL;
    }
  

  plugin = g_malloc (sizeof (plugin_t));
  
  plugin->descriptor = descriptor;
  plugin->dl_handle = dl_handle;
  plugin->desc = desc;
  plugin->copies = copies;
  plugin->enabled = FALSE;
  plugin->next = NULL;
  plugin->prev = NULL;
  plugin->wet_dry_enabled = FALSE;
  plugin->jack_rack = jack_rack;
  
  /* create audio memory and wet/dry stuff */
  plugin->audio_output_memory   = g_malloc (sizeof (LADSPA_Data *) * jack_rack->channels);
  plugin->wet_dry_fifos  = g_malloc (sizeof (lff_t) * jack_rack->channels);
  plugin->wet_dry_values = g_malloc (sizeof (LADSPA_Data) * jack_rack->channels);
  
  for (i = 0; i < jack_rack->channels; i++)
    {
      plugin->audio_output_memory[i] = g_malloc (sizeof (LADSPA_Data) * buffer_size);
      lff_init (plugin->wet_dry_fifos + i, CONTROL_FIFO_SIZE, sizeof (LADSPA_Data));
      plugin->wet_dry_values[i] = 1.0;
    }
  
  /* create holders and fill them out */
  plugin->holders = g_malloc (sizeof (ladspa_holder_t) * copies);
  for (i = 0; i < copies; i++)
    plugin_init_holder (plugin, i, instances[i], jack_rack);
  
  return plugin;
}


void
plugin_destroy (plugin_t * plugin)
{
  unsigned long i, j;
  int err;

  /* destroy holders */
  for (i = 0; i < plugin->copies; i++)
    {
      if (plugin->descriptor->deactivate)
        plugin->descriptor->deactivate (plugin->holders[i].instance);
      
/*      if (plugin->descriptor->cleanup)
        plugin->descriptor->cleanup (plugin->holders[i].instance); */
      
      if (plugin->desc->control_port_count > 0)
        {
          for (j = 0; j < plugin->desc->control_port_count; j++)
            {
              lff_free (plugin->holders[i].ui_control_fifos + j);
            }
          g_free (plugin->holders[i].ui_control_fifos);
          g_free (plugin->holders[i].control_memory);
        }
      
      /* aux ports */
      if (plugin->jack_rack->procinfo->jack_client && plugin->desc->aux_channels > 0)
        {
          for (j = 0; j < plugin->desc->aux_channels; j++)
            {
              err = jack_port_unregister (plugin->jack_rack->procinfo->jack_client,
                                          plugin->holders[i].aux_ports[j]);
          
              if (err)
                mlt_log_warning( NULL, "%s: could not unregister jack port\n", __FUNCTION__);
            }
       
          g_free (plugin->holders[i].aux_ports);
        }
    }
    
  g_free (plugin->holders);
  
  for (i = 0; i < plugin->jack_rack->channels; i++)
    {
      g_free (plugin->audio_output_memory[i]);
      lff_free (plugin->wet_dry_fifos + i);
    }
    
  g_free (plugin->audio_output_memory);
  g_free (plugin->wet_dry_fifos);
  g_free (plugin->wet_dry_values);
  
  err = dlclose (plugin->dl_handle);
  if (err)
    {
      mlt_log_warning( NULL, "%s: error closing shared object '%s': %s\n",
               __FUNCTION__, plugin->desc->object_file, dlerror ());
    }
   
  g_free (plugin);
}


/* EOF */
