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

#ifndef __JR_PLUGIN_H__
#define __JR_PLUGIN_H__

#include <glib.h>
#include <ladspa.h>
#include <pthread.h>
#include <jack/jack.h>

#include "process.h"
#include "plugin_desc.h"

typedef struct _ladspa_holder ladspa_holder_t;
typedef struct _plugin plugin_t;

struct _ladspa_holder
{
  LADSPA_Handle instance;
  lff_t * ui_control_fifos;
  LADSPA_Data * control_memory;

  jack_port_t **             aux_ports;
};

struct _plugin
{
  plugin_desc_t *            desc;
  gint                       enabled;

  gint                       copies;
  ladspa_holder_t *          holders;
  LADSPA_Data **             audio_input_memory;
  LADSPA_Data **             audio_output_memory;
  
  gboolean                   wet_dry_enabled;
  /* 1.0 = all wet, 0.0 = all dry, 0.5 = 50% wet/50% dry */
  LADSPA_Data *              wet_dry_values;
  lff_t *                    wet_dry_fifos;
  
  plugin_t *                 next;
  plugin_t *                 prev;

  const LADSPA_Descriptor *  descriptor;
  void *                     dl_handle;
  struct _jack_rack *        jack_rack;
  
};

void       process_add_plugin            (process_info_t *, plugin_t *plugin);
plugin_t * process_remove_plugin         (process_info_t *, plugin_t *plugin);
void       process_ablise_plugin         (process_info_t *, plugin_t *plugin, gboolean able);
void       process_ablise_plugin_wet_dry (process_info_t *, plugin_t *plugin, gboolean enable);
void       process_move_plugin           (process_info_t *, plugin_t *plugin, gint up);
plugin_t * process_change_plugin         (process_info_t *, plugin_t *plugin, plugin_t * new_plugin);

struct _jack_rack;
struct _ui;

plugin_t * plugin_new (plugin_desc_t * plugin_desc, struct _jack_rack * jack_rack);
void       plugin_destroy (plugin_t * plugin);

void plugin_connect_input_ports (plugin_t * plugin, LADSPA_Data ** inputs);
void plugin_connect_output_ports (plugin_t * plugin);

#endif /* __JR_PLUGIN_H__ */
