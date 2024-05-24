/*
 * JACK Rack
 *
 * Original:
 * Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
 *
 * Modification for MLT:
 * Copyright (C) 2004-2021 Meltytech, LLC
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
#include <math.h>

#include <glib.h>

#include "lv2_plugin.h"
#include "lv2_rack.h"
#include "lv2_process.h"
#include "framework/mlt_log.h"

#define CONTROL_FIFO_SIZE   128

extern const LV2_Feature* features[];

#ifdef WITH_JACK
/* swap over the jack ports in two plugins */
static void
plugin_swap_aux_ports (lv2_plugin_t * plugin, lv2_plugin_t * other)
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
#endif

/** connect up the LV2 instance's input buffers to the previous
    plugin's audio memory.  make sure to check that plugin->prev
    exists. */
void
lv2_plugin_connect_input_ports (lv2_plugin_t * plugin, LADSPA_Data ** inputs)
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
	  lilv_instance_connect_port(plugin->holders[copy].instance, plugin->desc->audio_input_port_indicies[channel],
                             inputs[rack_channel]);

          rack_channel++;
        }
    }
  
  plugin->audio_input_memory = inputs;
}


/** connect up a plugin's output ports to its own audio_output_memory output memory */
void
lv2_plugin_connect_output_ports (lv2_plugin_t * plugin)
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
	  lilv_instance_connect_port(plugin->holders[copy].instance, plugin->desc->audio_output_port_indicies[channel], plugin->audio_output_memory[rack_channel]);

          rack_channel++;
        }
    }
}


void
lv2_process_add_plugin (lv2_process_info_t * procinfo, lv2_plugin_t * plugin)
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
lv2_plugin_t *
lv2_process_remove_plugin (lv2_process_info_t * procinfo, lv2_plugin_t *plugin)
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
    
#ifdef WITH_JACK
  /* sort out the aux ports */
  if (procinfo->jack_client && plugin->desc->aux_channels > 0)
    {
      lv2_plugin_t * other;
      
      for (other = plugin->next; other; other = other->next)
        if (other->desc->id == plugin->desc->id)
          plugin_swap_aux_ports (plugin, other);
    }
#endif

  return plugin;
}

/** enable/disable a plugin */
void
lv2_process_ablise_plugin (lv2_process_info_t * procinfo, lv2_plugin_t *plugin, gboolean enable)
{
  plugin->enabled = enable;
}

/** enable/disable a plugin */
void
lv2_process_ablise_plugin_wet_dry (lv2_process_info_t * procinfo, lv2_plugin_t *plugin, gboolean enable)
{
  plugin->wet_dry_enabled = enable;
}

/** move a plugin up or down one place in the chain */
void
lv2_process_move_plugin (lv2_process_info_t * procinfo, lv2_plugin_t *plugin, gint up)
{
  /* other plugins in the chain */
  lv2_plugin_t *pp = NULL, *p, *n, *nn = NULL;

  /* note that we should never receive an illogical move request
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

#ifdef WITH_JACK
  if (procinfo->jack_client && plugin->desc->aux_channels > 0)
    {
      lv2_plugin_t * other;
      other = up ? plugin->next : plugin->prev;
      
      /* swap around the jack ports */
      if (other->desc->id == plugin->desc->id)
        plugin_swap_aux_ports (plugin, other);
    }
#endif
}

/** exchange an existing plugin for a newly created one */
lv2_plugin_t *
lv2_process_change_plugin (lv2_process_info_t * procinfo,
	               lv2_plugin_t *plugin, lv2_plugin_t * new_plugin)
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

#ifdef WITH_JACK
  /* sort out the aux ports */
  if (procinfo->jack_client && plugin->desc->aux_channels > 0)
    {
      lv2_plugin_t * other;
      
      for (other = plugin->next; other; other = other->next)
        if (other->desc->id == plugin->desc->id)
          plugin_swap_aux_ports (plugin, other);
    }
#endif

  return plugin;
}


/******************************************
 ************* non RT stuff ***************
 ******************************************/

static int
lv2_plugin_instantiate (const LilvPlugin *plugin,
		     unsigned long plugin_index,
		     gint copies,
		     LilvInstance **instances)
{
  gint i;
  
  for (i = 0; i < copies; i++)
    {
      instances[i] = lilv_plugin_instantiate(plugin, lv2_sample_rate, features);
      
      if (!instances[i])
        {
          unsigned long d;
          for (d = 0; d < i; d++)
	    lilv_instance_free(instances[d]);
          return 1;
        }
    }
  
  return 0;
}

#ifdef WITH_JACK

static void
lv2_plugin_create_aux_ports (lv2_plugin_t * plugin, guint copy, lv2_rack_t * lv2_rack)
{
  lv2_plugin_desc_t * desc;
  unsigned long aux_channel = 1;
  unsigned long plugin_index = 1;
  unsigned long i;
  char port_name[64];
  char * plugin_name;
  char * ptr;
  lv2_holder_t * holder;
     
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
	for (list = lv2_rack->slots; list; list = g_list_next (list))
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
	jack_port_register (lv2_rack->procinfo->jack_client,
			    port_name,
			    JACK_DEFAULT_AUDIO_TYPE,
			    desc->aux_are_input ? JackPortIsInput : JackPortIsOutput,
			    0);
         
      if (!holder->aux_ports[i])
	{
	  mlt_log_panic( NULL, "Could not register jack port '%s'; aborting\n", port_name);
	}
    }
     
  g_free (plugin_name);
}

#endif

static void
lv2_plugin_init_holder (const lv2_plugin_t   *plugin,
                    guint         copy,
                    const LilvInstance *instance,
                    lv2_rack_t  *lv2_rack)
{
  unsigned long i;
  lv2_plugin_desc_t * desc;
  lv2_holder_t * holder;
  
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

      if (!isnan (plugin->desc->def_values[desc->control_port_indicies[i]]))
	{
	  holder->control_memory[i] = plugin->desc->def_values[desc->control_port_indicies[i]];
	}
      else if (!isnan (plugin->desc->min_values[desc->control_port_indicies[i]]))
	{
	  holder->control_memory[i] = plugin->desc->min_values[desc->control_port_indicies[i]];
	}
      else if (!isnan (plugin->desc->max_values[desc->control_port_indicies[i]]))
	{
	  holder->control_memory[i] = plugin->desc->max_values[desc->control_port_indicies[i]];
	}
      else
	{
	  holder->control_memory[i] = 0.0;
	}

      lilv_instance_connect_port(instance, desc->control_port_indicies[i], &holder->control_memory[i]);
    }

  if (desc->status_port_count > 0)
    {
      holder->status_memory = g_malloc (sizeof (LADSPA_Data) * desc->status_port_count);
    }
  else
    {
      holder->status_memory = NULL;
    }

  for (i = 0; i < desc->status_port_count; i++)
    {
      lilv_instance_connect_port(instance, desc->status_port_indicies[i], holder->status_memory + i);
    }
  
#ifdef WITH_JACK
  if (lv2_rack->procinfo->jack_client && plugin->desc->aux_channels > 0)
    lv2_plugin_create_aux_ports (plugin, copy, lv2_rack);
#endif

  lilv_instance_activate(instance);
}

lv2_plugin_t *
lv2_plugin_new (lv2_plugin_desc_t * desc, lv2_rack_t * lv2_rack)
{
  LilvInstance **instances;
  gint copies;
  unsigned long i;
  int err;
  lv2_plugin_t * plugin;
  
  /* open the plugin */
  plugin = g_malloc (sizeof (lv2_plugin_t));

  char *str_ptr = strchr(desc->uri, '<');
  while (str_ptr != NULL)
    {
      *str_ptr++ = ':';
      str_ptr = strchr(str_ptr, '<');
    }

  plugin->lv2_plugin_uri = lilv_new_uri(lv2_rack->plugin_mgr->lv2_world, desc->uri);
  plugin->lv2_plugin = lilv_plugins_get_by_uri(lv2_rack->plugin_mgr->plugin_list, plugin->lv2_plugin_uri);

  str_ptr = strchr(desc->uri, ':');
  while (str_ptr != NULL)
    {
      *str_ptr++ = '<';
      str_ptr = strchr(str_ptr, ':');
    }

  /* create the instances */
  copies = lv2_plugin_desc_get_copies (desc, lv2_rack->channels);
  instances = g_malloc (sizeof (LADSPA_Handle) * copies);

  err = lv2_plugin_instantiate (plugin->lv2_plugin, desc->index, copies, instances);

  if (err)
    {
      g_free (instances);
      return NULL;
    }

  plugin->desc = desc;
  plugin->copies = copies;
  plugin->enabled = FALSE;
  plugin->next = NULL;
  plugin->prev = NULL;
  plugin->wet_dry_enabled = FALSE;
  plugin->lv2_rack = lv2_rack;

  /* create audio memory and wet/dry stuff */
  plugin->audio_output_memory   = g_malloc (sizeof (LADSPA_Data *) * lv2_rack->channels);
  plugin->wet_dry_fifos  = g_malloc (sizeof (lff_t) * lv2_rack->channels);
  plugin->wet_dry_values = g_malloc (sizeof (LADSPA_Data) * lv2_rack->channels);

  for (i = 0; i < lv2_rack->channels; i++)
    {
      plugin->audio_output_memory[i] = g_malloc (sizeof (LADSPA_Data) * lv2_buffer_size);
      lff_init (plugin->wet_dry_fifos + i, CONTROL_FIFO_SIZE, sizeof (LADSPA_Data));
      plugin->wet_dry_values[i] = 1.0;
    }

  /* create holders and fill them out */
  plugin->holders = g_malloc (sizeof (lv2_holder_t) * copies);
  for (i = 0; i < copies; i++)
    lv2_plugin_init_holder (plugin, i, instances[i], lv2_rack);

  return plugin;
}


void
lv2_plugin_destroy (lv2_plugin_t * plugin)
{
  unsigned long i, j;
  int err = 0;

  /* destroy holders */
  for (i = 0; i < plugin->copies; i++)
    {
      lilv_instance_deactivate(plugin->holders[i].instance);
      
      if (plugin->desc->control_port_count > 0)
        {
          for (j = 0; j < plugin->desc->control_port_count; j++)
            {
              lff_free (plugin->holders[i].ui_control_fifos + j);
            }
          g_free (plugin->holders[i].ui_control_fifos);
          g_free (plugin->holders[i].control_memory);
        }
      
      if (plugin->desc->status_port_count > 0)
        {
          g_free (plugin->holders[i].status_memory);
        }

#ifdef WITH_JACK
      /* aux ports */
      if (plugin->lv2_rack->procinfo->jack_client && plugin->desc->aux_channels > 0)
        {
          for (j = 0; j < plugin->desc->aux_channels; j++)
            {
              err = jack_port_unregister (plugin->lv2_rack->procinfo->jack_client,
                                          plugin->holders[i].aux_ports[j]);
          
              if (err)
                mlt_log_warning( NULL, "%s: could not unregister jack port\n", __FUNCTION__);
            }
       
          g_free (plugin->holders[i].aux_ports);
        }
#endif
    }
    
  g_free (plugin->holders);
  
  for (i = 0; i < plugin->lv2_rack->channels; i++)
    {
      g_free (plugin->audio_output_memory[i]);
      lff_free (plugin->wet_dry_fifos + i);
    }
    
  g_free (plugin->audio_output_memory);
  g_free (plugin->wet_dry_fifos);
  g_free (plugin->wet_dry_values);

  if (err)
    {
      mlt_log_warning( NULL, "%s: error closing shared object '%s': %s\n",
               __FUNCTION__, plugin->desc->uri, dlerror ());
    }
   
  g_free (plugin);
}


/* EOF */
