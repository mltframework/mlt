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

#ifndef __JR_VST2_PLUGIN_SETTINGS_H__
#define __JR_VST2_PLUGIN_SETTINGS_H__

#include <glib.h>
#include <ladspa.h>

#include "plugin_mgr.h"
#include "plugin_desc.h"

typedef struct _vst2_settings vst2_settings_t;

struct _vst2_settings
{
  guint32   sample_rate;
  vst2_plugin_desc_t *  desc;
  guint            copies;
  LADSPA_Data **   control_values;
  gboolean *       locks;
  gboolean         lock_all;
  gboolean         enabled;
  unsigned long    channels;
  gboolean         wet_dry_enabled;
  gboolean         wet_dry_locked;
  LADSPA_Data *    wet_dry_values;
};

vst2_settings_t * vst2_settings_new     (vst2_plugin_desc_t * desc, unsigned long channels, guint32 sample_rate);
vst2_settings_t * vst2_settings_dup     (vst2_settings_t * settings);
void         vst2_settings_destroy (vst2_settings_t * settings);

void vst2_settings_set_control_value   (vst2_settings_t * settings, guint copy, unsigned long control_index, LADSPA_Data value);
void vst2_settings_set_lock            (vst2_settings_t * settings, unsigned long control_index, gboolean locked);
void vst2_settings_set_lock_all        (vst2_settings_t * settings, gboolean lock_all);
void vst2_settings_set_enabled         (vst2_settings_t * settings, gboolean enabled);
void vst2_settings_set_wet_dry_enabled (vst2_settings_t * settings, gboolean enabled);
void vst2_settings_set_wet_dry_locked  (vst2_settings_t * settings, gboolean locked);
void vst2_settings_set_wet_dry_value   (vst2_settings_t * settings, unsigned long channel, LADSPA_Data value);

LADSPA_Data   vst2_settings_get_control_value   (vst2_settings_t * settings, guint copy, unsigned long control_index);
gboolean      vst2_settings_get_lock            (const vst2_settings_t * settings, unsigned long control_index);
gboolean      vst2_settings_get_lock_all        (const vst2_settings_t * settings);
gboolean      vst2_settings_get_enabled         (const vst2_settings_t * settings);
guint         vst2_settings_get_copies          (const vst2_settings_t * settings);
unsigned long vst2_settings_get_channels        (const vst2_settings_t * settings);
gboolean      vst2_settings_get_wet_dry_enabled (const vst2_settings_t * settings);
gboolean      vst2_settings_get_wet_dry_locked  (const vst2_settings_t * settings);
LADSPA_Data   vst2_settings_get_wet_dry_value   (vst2_settings_t * settings, unsigned long channel);

void vst2_settings_set_sample_rate (vst2_settings_t * settings, guint32 sample_rate);

#endif /* __JR_VST2_PLUGIN_SETTINGS_H__ */
