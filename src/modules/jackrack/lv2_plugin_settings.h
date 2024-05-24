/*
 * JACK Rack
 *
 * Original:
 * Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
 *
 * Modification for MLT:
 * Copyright (C) 2004-2014 Meltytech, LLC
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

#ifndef __LV2_PLUGIN_SETTINGS_H__
#define __LV2_PLUGIN_SETTINGS_H__

#include <glib.h>
#include <ladspa.h>

#include "plugin_mgr.h"
#include "plugin_desc.h"

typedef struct _lv2_settings lv2_settings_t;

struct _lv2_settings
{
  guint32   sample_rate;
  lv2_plugin_desc_t *  desc;
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

lv2_settings_t * lv2_settings_new     (lv2_plugin_desc_t * desc, unsigned long channels, guint32 sample_rate);
lv2_settings_t * lv2_settings_dup     (lv2_settings_t * settings);
void         lv2_settings_destroy (lv2_settings_t * settings);

void lv2_settings_set_control_value   (lv2_settings_t * settings, guint copy, unsigned long control_index, LADSPA_Data value);
void lv2_settings_set_lock            (lv2_settings_t * settings, unsigned long control_index, gboolean locked);
void lv2_settings_set_lock_all        (lv2_settings_t * settings, gboolean lock_all);
void lv2_settings_set_enabled         (lv2_settings_t * settings, gboolean enabled);
void lv2_settings_set_wet_dry_enabled (lv2_settings_t * settings, gboolean enabled);
void lv2_settings_set_wet_dry_locked  (lv2_settings_t * settings, gboolean locked);
void lv2_settings_set_wet_dry_value   (lv2_settings_t * settings, unsigned long channel, LADSPA_Data value);

LADSPA_Data   lv2_settings_get_control_value   (lv2_settings_t * settings, guint copy, unsigned long control_index);
gboolean      lv2_settings_get_lock            (const lv2_settings_t * settings, unsigned long control_index);
gboolean      lv2_settings_get_lock_all        (const lv2_settings_t * settings);
gboolean      lv2_settings_get_enabled         (const lv2_settings_t * settings);
guint         lv2_settings_get_copies          (const lv2_settings_t * settings);
unsigned long lv2_settings_get_channels        (const lv2_settings_t * settings);
gboolean      lv2_settings_get_wet_dry_enabled (const lv2_settings_t * settings);
gboolean      lv2_settings_get_wet_dry_locked  (const lv2_settings_t * settings);
LADSPA_Data   lv2_settings_get_wet_dry_value   (lv2_settings_t * settings, unsigned long channel);

void lv2_settings_set_sample_rate (lv2_settings_t * settings, guint32 sample_rate);

#endif /* __LV2_PLUGIN_SETTINGS_H__ */
