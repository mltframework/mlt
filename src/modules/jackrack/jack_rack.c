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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#include <ladspa.h>
#include <libxml/tree.h>

#include "jack_rack.h"
#include "lock_free_fifo.h"
#include "plugin_settings.h"
#include "framework/mlt_log.h"

#ifndef _
#define _(x) x
#endif
#define _x (const xmlChar*)
#define _s (const char*)

extern plugin_mgr_t *g_jackrack_plugin_mgr;

jack_rack_t *
jack_rack_new (const char * client_name, unsigned long channels)
{
  jack_rack_t *rack;

  rack = g_malloc (sizeof (jack_rack_t));
  rack->saved_plugins  = NULL;
  rack->channels       = channels;
  rack->procinfo = process_info_new (client_name, channels, FALSE, FALSE);
  if (!rack->procinfo) {
    g_free (rack);
    return NULL;
  }
  rack->plugin_mgr = g_jackrack_plugin_mgr;
  plugin_mgr_set_plugins (rack->plugin_mgr, channels);

  return rack;
}


void
jack_rack_destroy (jack_rack_t * jack_rack)
{
  process_quit (jack_rack->procinfo);
  // plugin_mgr is shared and global now, so we do not destroy it with each instance
//  plugin_mgr_destroy (jack_rack->plugin_mgr);
  process_info_destroy (jack_rack->procinfo);
  g_slist_free (jack_rack->saved_plugins);
  g_free (jack_rack);
}

plugin_t *
jack_rack_instantiate_plugin (jack_rack_t * jack_rack, plugin_desc_t * desc)
{
  plugin_t * plugin;
  
  /* check whether or not the plugin is RT capable and confirm with the user if it isn't */
  if (!LADSPA_IS_HARD_RT_CAPABLE(desc->properties)) {
    mlt_log_info( NULL, "Plugin not RT capable. The plugin '%s' does not describe itself as being capable of real-time operation. You may experience drop outs or jack may even kick us out if you use it.\n",
               desc->name);
  }

  /* create the plugin */
  plugin = plugin_new (desc, jack_rack);

  if (!plugin) {
   mlt_log_error( NULL, "Error loading file plugin '%s' from file '%s'\n",
               desc->name, desc->object_file);
  }
  
  return plugin;
}


void
jack_rack_add_saved_plugin (jack_rack_t * jack_rack, saved_plugin_t * saved_plugin)
{
  plugin_t * plugin = jack_rack_instantiate_plugin (jack_rack, saved_plugin->settings->desc);
  if (!plugin)
    return;
  jack_rack->saved_plugins = g_slist_append (jack_rack->saved_plugins, saved_plugin);
  process_add_plugin (jack_rack->procinfo, plugin);
  jack_rack_add_plugin (jack_rack, plugin);
}


void
jack_rack_add_plugin (jack_rack_t * jack_rack, plugin_t * plugin)
{
  saved_plugin_t * saved_plugin = NULL;
  GSList * list;
  unsigned long control, channel;
  LADSPA_Data value;
  guint copy;
  
  /* see if there's any saved settings that match the plugin id */
  for (list = jack_rack->saved_plugins; list; list = g_slist_next (list))
    {
      saved_plugin = list->data;
      
      if (saved_plugin->settings->desc->id == plugin->desc->id)
        {
          /* process the settings! */
          jack_rack->saved_plugins = g_slist_remove (jack_rack->saved_plugins, saved_plugin);
          break;
        }
      saved_plugin = NULL;
    }
	
  /* initialize plugin parameters */
  plugin->enabled = settings_get_enabled (saved_plugin->settings);
  plugin->wet_dry_enabled = settings_get_wet_dry_enabled (saved_plugin->settings);
	
  for (control = 0; control < saved_plugin->settings->desc->control_port_count; control++)
    for (copy = 0; copy < plugin->copies; copy++)
      {
        value = settings_get_control_value (saved_plugin->settings, copy, control);
        plugin->holders[copy].control_memory[control] = value;
//mlt_log_debug( NULL, "setting control value %s (%d) = %f\n", saved_plugin->settings->desc->port_names[control], copy, value);
//        lff_write (plugin->holders[copy].ui_control_fifos + control, &value);
      }
  if (plugin->wet_dry_enabled)
    for (channel = 0; channel < jack_rack->channels; channel++)
      {
        value = settings_get_wet_dry_value (saved_plugin->settings, channel);
        plugin->wet_dry_values[channel] = value;
//mlt_log_debug( NULL, "setting wet/dry value %d = %f\n", channel, value);
//        lff_write (plugin->wet_dry_fifos + channel, &value);
      }
}


static void
saved_rack_parse_plugin (jack_rack_t * jack_rack, saved_rack_t * saved_rack, saved_plugin_t * saved_plugin,
                         const char * filename, xmlNodePtr plugin)
{
  plugin_desc_t * desc;
  settings_t * settings = NULL;
  xmlNodePtr node;
  xmlNodePtr sub_node;
  xmlChar *content;
  unsigned long num;
  unsigned long control = 0;
#ifdef WIN32
  xmlFreeFunc xmlFree = NULL;
  xmlMemGet( &xmlFree, NULL, NULL, NULL);
#endif

  for (node = plugin->children; node; node = node->next)
    {
      if (xmlStrcmp (node->name, _x("id")) == 0)
        {
          content = xmlNodeGetContent (node);
          num = strtoul (_s(content), NULL, 10);
          xmlFree (content);

          desc = plugin_mgr_get_any_desc (jack_rack->plugin_mgr, num);
          if (!desc)
            {
              mlt_log_verbose( NULL, _("The file '%s' contains an unknown plugin with ID '%ld'; skipping\n"), filename, num);
              return;
            }
          
          settings = settings_new (desc, saved_rack->channels, saved_rack->sample_rate);
        }
      else if (xmlStrcmp (node->name, _x("enabled")) == 0)
        {
          content = xmlNodeGetContent (node);
          settings_set_enabled (settings, xmlStrcmp (content, _x("true")) == 0 ? TRUE : FALSE);
          xmlFree (content);
        }
      else if (xmlStrcmp (node->name, _x("wet_dry_enabled")) == 0)
        {
          content = xmlNodeGetContent (node);
          settings_set_wet_dry_enabled (settings, xmlStrcmp (content, _x("true")) == 0 ? TRUE : FALSE);
          xmlFree (content);
        }
      else if (xmlStrcmp (node->name, _x("wet_dry_locked")) == 0)
        {
          content = xmlNodeGetContent (node);
          settings_set_wet_dry_locked (settings, xmlStrcmp (content, _x("true")) == 0 ? TRUE : FALSE);
          xmlFree (content);
        }
      else if (xmlStrcmp (node->name, _x("wet_dry_values")) == 0)
        {
          unsigned long channel = 0;
          
          for (sub_node = node->children; sub_node; sub_node = sub_node->next)
            {
              if (xmlStrcmp (sub_node->name, _x("value")) == 0)
                {
                  content = xmlNodeGetContent (sub_node);
                  settings_set_wet_dry_value (settings, channel, strtod (_s(content), NULL));
                  xmlFree (content);
                  
                  channel++;
                }
            }
        }
      else if (xmlStrcmp (node->name, _x("lockall")) == 0)
        {
          content = xmlNodeGetContent (node);
          settings_set_lock_all (settings, xmlStrcmp (content, _x("true")) == 0 ? TRUE : FALSE);
          xmlFree (content);
        }
      else if (xmlStrcmp (node->name, _x("controlrow")) == 0)
        {
          gint copy = 0;

          for (sub_node = node->children; sub_node; sub_node = sub_node->next)
            {
              if (xmlStrcmp (sub_node->name, _x("lock")) == 0)
                {
                  content = xmlNodeGetContent (sub_node);
                  settings_set_lock (settings, control, xmlStrcmp (content, _x("true")) == 0 ? TRUE : FALSE);
                  xmlFree (content);
                }
              else if (xmlStrcmp (sub_node->name, _x("value")) == 0)
                {
                  content = xmlNodeGetContent (sub_node);
                  settings_set_control_value (settings, copy, control, strtod (_s(content), NULL));
                  xmlFree (content);
                  copy++;
                }
            }
          
          control++;
        }
    }
  
  if (settings)
    saved_plugin->settings = settings;
}

static void
saved_rack_parse_jackrack (jack_rack_t * jack_rack, saved_rack_t * saved_rack, const char * filename, xmlNodePtr jackrack)
{
  xmlNodePtr node;
  xmlChar *content;
  saved_plugin_t * saved_plugin;
#ifdef WIN32
  xmlFreeFunc xmlFree = NULL;
  xmlMemGet( &xmlFree, NULL, NULL, NULL);
#endif

  for (node = jackrack->children; node; node = node->next)
    {
      if (xmlStrcmp (node->name, _x("channels")) == 0)
        {
          content = xmlNodeGetContent (node);
          saved_rack->channels = strtoul (_s(content), NULL, 10);
          xmlFree (content);
        }
      else if (xmlStrcmp (node->name, _x("samplerate")) == 0)
        {
          content = xmlNodeGetContent (node);
          saved_rack->sample_rate = strtoul (_s(content), NULL, 10);
          xmlFree (content);
        }
      else if (xmlStrcmp (node->name, _x("plugin")) == 0)
        {
          saved_plugin = g_malloc0 (sizeof (saved_plugin_t));
          saved_rack->plugins = g_slist_append (saved_rack->plugins, saved_plugin);
          saved_rack_parse_plugin (jack_rack, saved_rack, saved_plugin, filename, node);
        }
    }
}

static saved_rack_t *
saved_rack_new (jack_rack_t * jack_rack, const char * filename, xmlDocPtr doc)
{
  xmlNodePtr node;
  saved_rack_t *saved_rack;
  
  /* create the saved rack */
  saved_rack = g_malloc (sizeof (saved_rack_t));
  saved_rack->plugins = NULL;
  saved_rack->sample_rate = 48000;
  saved_rack->channels = 2;
  
  for (node = doc->children; node; node = node->next)
    {
      if (xmlStrcmp (node->name, _x("jackrack")) == 0)
        saved_rack_parse_jackrack (jack_rack, saved_rack, filename, node);
    }
  
  return saved_rack;
}

static void
saved_rack_destroy (saved_rack_t * saved_rack)
{
  GSList * list;
  
  for (list = saved_rack->plugins; list; list = g_slist_next (list))
    settings_destroy (((saved_plugin_t *) list->data)->settings);
  g_slist_free (saved_rack->plugins);
  g_free (saved_rack);
}


int
jack_rack_open_file (jack_rack_t * jack_rack, const char * filename)
{
  xmlDocPtr doc;
  saved_rack_t * saved_rack;
  GSList * list;
  saved_plugin_t * saved_plugin;

  doc = xmlParseFile (filename);
  if (!doc)
    {
      mlt_log_error( NULL, _("Could not parse file '%s'\n"), filename);
      return 1;
    }
  
  if (xmlStrcmp ( ((xmlDtdPtr)doc->children)->name, _x("jackrack")) != 0)
    {
      mlt_log_error( NULL, _("The file '%s' is not a JACK Rack settings file\n"), filename);
      return 1;
    }
  
  saved_rack = saved_rack_new (jack_rack, filename, doc);
  xmlFreeDoc (doc);
  
  if (!saved_rack)
    return 1;

  for (list = saved_rack->plugins; list; list = g_slist_next (list))
    {
      saved_plugin = list->data;
      
      settings_set_sample_rate (saved_plugin->settings, sample_rate);
      
      jack_rack_add_saved_plugin (jack_rack, saved_plugin);
    }
  
  saved_rack_destroy (saved_rack);
  
  return 0;
}


/* EOF */
