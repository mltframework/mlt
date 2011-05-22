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

#ifndef __JR_PLUGIN_MANAGER_H__
#define __JR_PLUGIN_MANAGER_H__

#include <glib.h>

#include "plugin_desc.h"
#include "framework/mlt_properties.h"

typedef struct _plugin_mgr plugin_mgr_t;

struct _plugin_mgr
{
  GSList * all_plugins;

  GSList * plugins;
  unsigned long plugin_count;
  mlt_properties blacklist;
};

struct _ui;

plugin_mgr_t * plugin_mgr_new ();
void           plugin_mgr_destroy (plugin_mgr_t * plugin_mgr);

void plugin_mgr_set_plugins (plugin_mgr_t * plugin_mgr, unsigned long rack_channels);

plugin_desc_t * plugin_mgr_get_desc (plugin_mgr_t * plugin_mgr, unsigned long id);
plugin_desc_t * plugin_mgr_get_any_desc (plugin_mgr_t * plugin_mgr, unsigned long id);

#endif /* __JR_PLUGIN_MANAGER_H__ */
