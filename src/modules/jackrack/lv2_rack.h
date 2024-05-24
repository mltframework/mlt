/*
 * LV2 Rack
 *
 * Based on the Jack Rack module
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

#ifndef __LV2_RACK_H__
#define __LV2_RACK_H__

#include <glib.h>
#include <ladspa.h>

#include "lv2_plugin.h"
#include "plugin_mgr.h"
#include "lv2_plugin_settings.h"
#include "lv2_process.h"

typedef struct _saved_plugin saved_plugin_t;

struct _saved_plugin
{
  lv2_settings_t     *settings;
};

typedef struct _saved_rack   saved_rack_t;

struct _saved_rack
{
  unsigned long  channels;
  jack_nframes_t sample_rate;
  GSList *       plugins;
};

typedef struct _lv2_rack lv2_rack_t;

struct _lv2_rack
{
  lv2_mgr_t *    plugin_mgr;
  lv2_process_info_t *  procinfo;
  unsigned long     channels;
  GSList *          saved_plugins;
};

lv2_rack_t * lv2_rack_new     (const char * client_name, unsigned long channels);
void          lv2_rack_destroy (lv2_rack_t * lv2_rack);

int lv2_rack_open_file (lv2_rack_t * lv2_rack, const char * filename);
void lv2_rack_add_plugin (lv2_rack_t * lv2_rack, lv2_plugin_t * plugin);
void lv2_rack_add_saved_plugin (lv2_rack_t * lv2_rack, struct _saved_plugin * saved_plugin);

lv2_plugin_t * lv2_rack_instantiate_plugin (lv2_rack_t * lv2_rack, lv2_plugin_desc_t * desc);

#endif /* __LV2_RACK_H__ */
