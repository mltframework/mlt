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

#ifndef __JR_JACK_RACK_H__
#define __JR_JACK_RACK_H__

#include <glib.h>
#include <ladspa.h>

#include "plugin.h"
#include "plugin_mgr.h"
#include "plugin_settings.h"
#include "process.h"

typedef struct _saved_plugin saved_plugin_t;

struct _saved_plugin
{
  settings_t     *settings;
};

typedef struct _saved_rack   saved_rack_t;

struct _saved_rack
{
  unsigned long  channels;
  jack_nframes_t sample_rate;
  GSList *       plugins;
};

typedef struct _jack_rack jack_rack_t;

struct _jack_rack
{
  plugin_mgr_t *    plugin_mgr;
  process_info_t *  procinfo;
  unsigned long     channels;
  GSList *          saved_plugins;
};

jack_rack_t * jack_rack_new     (const char * client_name, unsigned long channels);
void          jack_rack_destroy (jack_rack_t * jack_rack);

int jack_rack_open_file (jack_rack_t * jack_rack, const char * filename);
void jack_rack_add_plugin (jack_rack_t * jack_rack, plugin_t * plugin);
void jack_rack_add_saved_plugin (jack_rack_t * jack_rack, struct _saved_plugin * saved_plugin);

plugin_t * jack_rack_instantiate_plugin (jack_rack_t * jack_rack, plugin_desc_t * desc);

#endif /* __JR_JACK_RACK_H__ */
