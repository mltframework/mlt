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
#include <strings.h>

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

static void saved_rack_parse_plugin(lv2_context_t *lv2_context,
                                    saved_rack_t *saved_rack,
                                    saved_plugin_t *saved_plugin,
                                    const char *filename,
                                    xmlNodePtr plugin)
{
    lv2_plugin_desc_t *desc;
    lv2_settings_t *settings = NULL;
    xmlNodePtr node;
    xmlNodePtr sub_node;
    xmlChar *content;
    unsigned long control = 0;
#ifdef _WIN32
    xmlFreeFunc xmlFree = NULL;
    xmlMemGet(&xmlFree, NULL, NULL, NULL);
#endif

    for (node = plugin->children; node; node = node->next) {
        if (xmlStrcmp(node->name, _x("id")) == 0) {
            content = xmlNodeGetContent(node);

            desc = lv2_mgr_get_any_desc(lv2_context->plugin_mgr, _s(content));

            if (!desc) {
                mlt_log_verbose(
                    NULL,
                    _("The file '%s' contains an unknown plugin with ID '%s'; skipping\n"),
                    filename,
                    _s(content));
                xmlFree(content);
                return;
            }

            xmlFree(content);

            settings = lv2_settings_new(desc, saved_rack->channels, saved_rack->sample_rate);
        } else if (xmlStrcmp(node->name, _x("enabled")) == 0) {
            content = xmlNodeGetContent(node);
            lv2_settings_set_enabled(settings, xmlStrcmp(content, _x("true")) == 0 ? TRUE : FALSE);
            xmlFree(content);
        } else if (xmlStrcmp(node->name, _x("wet_dry_enabled")) == 0) {
            content = xmlNodeGetContent(node);
            lv2_settings_set_wet_dry_enabled(settings,
                                             xmlStrcmp(content, _x("true")) == 0 ? TRUE : FALSE);
            xmlFree(content);
        } else if (xmlStrcmp(node->name, _x("wet_dry_locked")) == 0) {
            content = xmlNodeGetContent(node);
            lv2_settings_set_wet_dry_locked(settings,
                                            xmlStrcmp(content, _x("true")) == 0 ? TRUE : FALSE);
            xmlFree(content);
        } else if (xmlStrcmp(node->name, _x("wet_dry_values")) == 0) {
            unsigned long channel = 0;

            for (sub_node = node->children; sub_node; sub_node = sub_node->next) {
                if (xmlStrcmp(sub_node->name, _x("value")) == 0) {
                    content = xmlNodeGetContent(sub_node);
                    lv2_settings_set_wet_dry_value(settings, channel, strtod(_s(content), NULL));
                    xmlFree(content);

                    channel++;
                }
            }
        } else if (xmlStrcmp(node->name, _x("lockall")) == 0) {
            content = xmlNodeGetContent(node);
            lv2_settings_set_lock_all(settings, xmlStrcmp(content, _x("true")) == 0 ? TRUE : FALSE);
            xmlFree(content);
        } else if (xmlStrcmp(node->name, _x("controlrow")) == 0) {
            gint copy = 0;

            for (sub_node = node->children; sub_node; sub_node = sub_node->next) {
                if (xmlStrcmp(sub_node->name, _x("lock")) == 0) {
                    content = xmlNodeGetContent(sub_node);
                    lv2_settings_set_lock(settings,
                                          control,
                                          xmlStrcmp(content, _x("true")) == 0 ? TRUE : FALSE);
                    xmlFree(content);
                } else if (xmlStrcmp(sub_node->name, _x("value")) == 0) {
                    content = xmlNodeGetContent(sub_node);
                    lv2_settings_set_control_value(settings,
                                                   copy,
                                                   control,
                                                   strtod(_s(content), NULL));
                    xmlFree(content);
                    copy++;
                }
            }

            control++;
        }
    }

    if (settings)
        saved_plugin->settings = settings;
}

static void saved_rack_parse_lv2context(lv2_context_t *lv2_context,
                                        saved_rack_t *saved_rack,
                                        const char *filename,
                                        xmlNodePtr lv2context)
{
    xmlNodePtr node;
    xmlChar *content;
    saved_plugin_t *saved_plugin;
#ifdef _WIN32
    xmlFreeFunc xmlFree = NULL;
    xmlMemGet(&xmlFree, NULL, NULL, NULL);
#endif

    for (node = lv2context->children; node; node = node->next) {
        if (xmlStrcmp(node->name, _x("channels")) == 0) {
            content = xmlNodeGetContent(node);
            saved_rack->channels = strtoul(_s(content), NULL, 10);
            xmlFree(content);
        } else if (xmlStrcmp(node->name, _x("samplerate")) == 0) {
            content = xmlNodeGetContent(node);
            saved_rack->sample_rate = strtoul(_s(content), NULL, 10);
            xmlFree(content);
        } else if (xmlStrcmp(node->name, _x("plugin")) == 0) {
            saved_plugin = g_malloc0(sizeof(saved_plugin_t));
            saved_rack->plugins = g_slist_append(saved_rack->plugins, saved_plugin);
            saved_rack_parse_plugin(lv2_context, saved_rack, saved_plugin, filename, node);
        }
    }
}

static saved_rack_t *saved_rack_new(lv2_context_t *lv2_context, const char *filename, xmlDocPtr doc)
{
    xmlNodePtr node;
    saved_rack_t *saved_rack;

    /* create the saved rack */
    saved_rack = g_malloc(sizeof(saved_rack_t));
    saved_rack->plugins = NULL;
    saved_rack->sample_rate = 48000;
    saved_rack->channels = 2;

    for (node = doc->children; node; node = node->next) {
        if (xmlStrcmp(node->name, _x("lv2context")) == 0)
            saved_rack_parse_lv2context(lv2_context, saved_rack, filename, node);
    }

    return saved_rack;
}

static void saved_rack_destroy(saved_rack_t *saved_rack)
{
    GSList *list;

    for (list = saved_rack->plugins; list; list = g_slist_next(list))
        lv2_settings_destroy(((saved_plugin_t *) list->data)->settings);
    g_slist_free(saved_rack->plugins);
    g_free(saved_rack);
}

/* EOF */
