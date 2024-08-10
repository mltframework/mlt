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

#ifndef __JR_VST2_PLUGIN_H__
#define __JR_VST2_PLUGIN_H__

#include <glib.h>
#include <ladspa.h>
#include <pthread.h>
#ifdef WITH_JACK
#include <jack/jack.h>
#endif

#include "vst2_process.h"
#include "plugin_desc.h"

typedef struct _vst2_holder vst2_holder_t;
typedef struct _vst2_plugin vst2_plugin_t;

struct _vst2_holder
{
  //LADSPA_Handle instance;
  AEffect * effect;
  lff_t * ui_control_fifos;
  LADSPA_Data * control_memory;
  LADSPA_Data * status_memory;

#ifdef WITH_JACK
  jack_port_t **             aux_ports;
#endif
};

struct _vst2_plugin
{
  vst2_plugin_desc_t *            desc;
  gint                       enabled;

  gint                       copies;
  vst2_holder_t *          holders;
  LADSPA_Data **             audio_input_memory;
  LADSPA_Data **             audio_output_memory;
  
  gboolean                   wet_dry_enabled;
  /* 1.0 = all wet, 0.0 = all dry, 0.5 = 50% wet/50% dry */
  LADSPA_Data *              wet_dry_values;
  lff_t *                    wet_dry_fifos;
  
  vst2_plugin_t *                 next;
  vst2_plugin_t *                 prev;

  //const LADSPA_Descriptor *  descriptor;
  //const AEffect *  effect;
  void *                     dl_handle;
  struct _vst2_context *        vst2_context;
  
};

void       vst2_process_add_plugin            (vst2_process_info_t *, vst2_plugin_t *plugin);
vst2_plugin_t * vst2_process_remove_plugin         (vst2_process_info_t *, vst2_plugin_t *plugin);
void       vst2_process_ablise_plugin         (vst2_process_info_t *, vst2_plugin_t *plugin, gboolean able);
void       vst2_process_ablise_plugin_wet_dry (vst2_process_info_t *, vst2_plugin_t *plugin, gboolean enable);
void       vst2_process_move_plugin           (vst2_process_info_t *, vst2_plugin_t *plugin, gint up);
vst2_plugin_t * vst2_process_change_plugin         (vst2_process_info_t *, vst2_plugin_t *plugin, vst2_plugin_t * new_plugin);

struct _vst2_context;
struct _ui;

vst2_plugin_t * vst2_plugin_new (vst2_plugin_desc_t * plugin_desc, struct _vst2_context * vst2_context);
void       vst2_plugin_destroy (vst2_plugin_t * plugin);

void vst2_plugin_connect_input_ports (vst2_plugin_t * plugin, LADSPA_Data ** inputs);
void vst2_plugin_connect_output_ports (vst2_plugin_t * plugin);

#endif /* __JR_VST2_PLUGIN_H__ */
