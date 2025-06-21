/*
 * LV2 Context
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
    #include <strings.h>
#endif

#include <ladspa.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "framework/mlt_log.h"
#include "lock_free_fifo.h"
#include "lv2_context.h"
#include "lv2_plugin_settings.h"

#ifndef _
#define _(x) x
#endif
#define _x (const xmlChar *)
#define _s (const char *)

extern lv2_mgr_t *g_lv2_plugin_mgr;

lv2_context_t *lv2_context_new(const char *client_name, unsigned long channels)
{
    lv2_context_t *rack;

    rack = g_malloc(sizeof(lv2_context_t));
    rack->saved_plugins = NULL;
    rack->channels = channels;
    rack->procinfo = lv2_process_info_new(client_name, channels, FALSE, FALSE);
    if (!rack->procinfo) {
        g_free(rack);
        return NULL;
    }

    rack->plugin_mgr = g_lv2_plugin_mgr;
    lv2_mgr_set_plugins(rack->plugin_mgr, channels);

    return rack;
}

void lv2_context_destroy(lv2_context_t *lv2_context)
{
    lv2_process_quit(lv2_context->procinfo);
    // plugin_mgr is shared and global now, so we do not destroy it with each instance
    // lv2_mgr_destroy (lv2_context->plugin_mgr);
    lv2_process_info_destroy(lv2_context->procinfo);
    g_slist_free(lv2_context->saved_plugins);
    g_free(lv2_context);
}

lv2_plugin_t *lv2_context_instantiate_plugin(lv2_context_t *lv2_context, lv2_plugin_desc_t *desc)
{
    lv2_plugin_t *plugin;

    /* check whether or not the plugin is RT capablex and confirm with the user if it isn't */
    if (!LADSPA_IS_HARD_RT_CAPABLE(desc->properties)) {
        mlt_log_info(NULL,
                     "Plugin not RT capable. The plugin '%s' does not describe itself as being "
                     "capable of real-time operation. You may experience drop outs or jack may "
                     "even kick us out if you use it.\n",
                     desc->name);
    }

    /* create the plugin */
    plugin = lv2_plugin_new(desc, lv2_context);

    if (!plugin) {
        mlt_log_error(NULL,
                      "Error loading file plugin '%s' from file '%s'\n",
                      desc->name,
                      desc->uri);
    }

    return plugin;
}

void lv2_context_add_saved_plugin(lv2_context_t *lv2_context, saved_plugin_t *saved_plugin)
{
    lv2_plugin_t *plugin = lv2_context_instantiate_plugin(lv2_context, saved_plugin->settings->desc);
    if (!plugin) {
        mlt_log_warning(NULL,
                        "%s: could not instantiate object file '%s'\n",
                        __FUNCTION__,
                        saved_plugin->settings->desc->uri);
        return;
    }
    lv2_context->saved_plugins = g_slist_append(lv2_context->saved_plugins, saved_plugin);
    lv2_process_add_plugin(lv2_context->procinfo, plugin);
    lv2_context_add_plugin(lv2_context, plugin);
}

void lv2_context_add_plugin(lv2_context_t *lv2_context, lv2_plugin_t *plugin)
{
    saved_plugin_t *saved_plugin = NULL;
    GSList *list;
    unsigned long control, channel;
    LADSPA_Data value;
    guint copy;

    /* see if there's any saved settings that match the plugin id */
    for (list = lv2_context->saved_plugins; list; list = g_slist_next(list)) {
        saved_plugin = list->data;

        if (saved_plugin->settings->desc->id == plugin->desc->id) {
            /* process the settings! */
            lv2_context->saved_plugins = g_slist_remove(lv2_context->saved_plugins, saved_plugin);
            break;
        }
        saved_plugin = NULL;
    }

    if (!saved_plugin)
        return;

    /* initialize plugin parameters */
    plugin->enabled = lv2_settings_get_enabled(saved_plugin->settings);
    plugin->wet_dry_enabled = lv2_settings_get_wet_dry_enabled(saved_plugin->settings);

    for (control = 0; control < saved_plugin->settings->desc->control_port_count; control++)
        for (copy = 0; copy < plugin->copies; copy++) {
            value = lv2_settings_get_control_value(saved_plugin->settings, copy, control);
            plugin->holders[copy].control_memory[control] = value;
            //mlt_log_debug( NULL, "setting control value %s (%d) = %f\n", saved_plugin->settings->desc->port_names[control], copy, value);
            //        lff_write (plugin->holders[copy].ui_control_fifos + control, &value);
        }
    if (plugin->wet_dry_enabled)
        for (channel = 0; channel < lv2_context->channels; channel++) {
            value = lv2_settings_get_wet_dry_value(saved_plugin->settings, channel);
            plugin->wet_dry_values[channel] = value;
            //mlt_log_debug( NULL, "setting wet/dry value %d = %f\n", channel, value);
            //        lff_write (plugin->wet_dry_fifos + channel, &value);
        }
}

/* EOF */
