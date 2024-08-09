/*
 * JACK Rack
 *
 * Original:
 * Copyright (C) Robert Ham 2002, 2003 (node@users.sourceforge.net)
 *
 * Modification for MLT:
 * Copyright (C) 2004-2024 Meltytech, LLC
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

#include "framework/mlt_properties.h"
#include "plugin_desc.h"

#ifdef WITH_LV2
#include <lilv/lilv.h>
#endif

typedef struct _plugin_mgr plugin_mgr_t;

struct _plugin_mgr
{
    GSList *all_plugins;

    GSList *plugins;
    unsigned long plugin_count;
    mlt_properties blacklist;
};

#ifdef WITH_LV2
typedef struct _lv2_mgr lv2_mgr_t;

struct _lv2_mgr
{
    LilvWorld *lv2_world;
    LilvPlugins *plugin_list;

    GSList *all_plugins; /* this contain instances of lv2_plugin_desc_t */

    GSList *plugins;
    unsigned long plugin_count;
    mlt_properties blacklist;
};
#endif

#ifdef WITH_VST2
typedef struct _vst2_mgr vst2_mgr_t;

struct _vst2_mgr
{
    GSList *all_plugins;

    GSList *plugins;
    unsigned long plugin_count;
    mlt_properties blacklist;
};
#endif

struct _ui;

plugin_mgr_t *plugin_mgr_new();
void plugin_mgr_destroy(plugin_mgr_t *plugin_mgr);

void plugin_mgr_set_plugins(plugin_mgr_t *plugin_mgr, unsigned long rack_channels);

plugin_desc_t *plugin_mgr_get_desc(plugin_mgr_t *plugin_mgr, unsigned long id);
plugin_desc_t *plugin_mgr_get_any_desc(plugin_mgr_t *plugin_mgr, unsigned long id);

#ifdef WITH_LV2
lv2_mgr_t *lv2_mgr_new();
void lv2_mgr_destroy(lv2_mgr_t *plugin_mgr);

void lv2_mgr_set_plugins(lv2_mgr_t *plugin_mgr, unsigned long rack_channels);

lv2_plugin_desc_t *lv2_mgr_get_desc(lv2_mgr_t *plugin_mgr, char *id);
lv2_plugin_desc_t *lv2_mgr_get_any_desc(lv2_mgr_t *plugin_mgr, char *id);
#endif

#ifdef WITH_VST2
vst2_mgr_t *vst2_mgr_new();
void vst2_mgr_destroy(vst2_mgr_t *plugin_mgr);

void vst2_mgr_set_plugins(vst2_mgr_t *vst2_mgr, unsigned long rack_channels);

vst2_plugin_desc_t *vst2_mgr_get_desc(vst2_mgr_t *vst2_mgr, unsigned long id);
vst2_plugin_desc_t *vst2_mgr_get_any_desc(vst2_mgr_t *vst2_mgr, unsigned long id);;
#endif

#endif /* __JR_PLUGIN_MANAGER_H__ */
