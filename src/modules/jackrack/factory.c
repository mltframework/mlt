/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2024 Meltytech, LLC
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt.h>

#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern mlt_consumer consumer_jack_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg);

#ifdef GPL
#include "plugin_mgr.h"
#include <ladspa.h>

#ifdef WITH_JACK
extern mlt_filter filter_jackrack_init(mlt_profile profile,
                                       mlt_service_type type,
                                       const char *id,
                                       char *arg);
#endif
extern mlt_filter filter_ladspa_init(mlt_profile profile,
                                     mlt_service_type type,
                                     const char *id,
                                     char *arg);
extern mlt_producer producer_ladspa_init(mlt_profile profile,
                                         mlt_service_type type,
                                         const char *id,
                                         char *arg);

#ifdef WITH_LV2

#include <lv2.h>

/* lv2 extenstions */
#include <lv2/atom/atom.h>
#include <lv2/midi/midi.h>
#include <lv2/port-groups/port-groups.h>
#include <lv2/port-props/port-props.h>
#include <lv2/presets/presets.h>
#include <lv2/resize-port/resize-port.h>
#include <lv2/ui/ui.h>
#include <lv2/worker/worker.h>

#include <lilv/lilv.h>

extern mlt_filter filter_lv2_init(mlt_profile profile,
                                  mlt_service_type type,
                                  const char *id,
                                  char *arg);
extern mlt_producer producer_lv2_init(mlt_profile profile,
                                      mlt_service_type type,
                                      const char *id,
                                      char *arg);
#endif

#ifdef WITH_VST2

#include "vestige.h"

extern mlt_filter filter_vst2_init(mlt_profile profile,
                                  mlt_service_type type,
                                  const char *id,
                                  char *arg);
extern mlt_producer producer_vst2_init(mlt_profile profile,
                                      mlt_service_type type,
                                      const char *id,
                                      char *arg);

#endif

plugin_mgr_t *g_jackrack_plugin_mgr = NULL;

#ifdef WITH_LV2
lv2_mgr_t *g_lv2_plugin_mgr = NULL;
#endif

#ifdef WITH_VST2
vst2_mgr_t *g_vst2_plugin_mgr = NULL;
#endif

static void add_port_to_metadata(mlt_properties p, plugin_desc_t *desc, int j)
{
    LADSPA_Data sample_rate = 48000;
    LADSPA_PortRangeHintDescriptor hint_descriptor = desc->port_range_hints[j].HintDescriptor;

    mlt_properties_set(p, "title", desc->port_names[j]);
    if (LADSPA_IS_HINT_INTEGER(hint_descriptor)) {
        mlt_properties_set(p, "type", "integer");
        mlt_properties_set_int(p,
                               "default",
                               plugin_desc_get_default_control_value(desc, j, sample_rate));
    } else if (LADSPA_IS_HINT_TOGGLED(hint_descriptor)) {
        mlt_properties_set(p, "type", "boolean");
        mlt_properties_set_int(p,
                               "default",
                               plugin_desc_get_default_control_value(desc, j, sample_rate));
    } else {
        mlt_properties_set(p, "type", "float");
        mlt_properties_set_double(p,
                                  "default",
                                  plugin_desc_get_default_control_value(desc, j, sample_rate));
    }
    /* set upper and lower, possibly adjusted to the sample rate */
    if (LADSPA_IS_HINT_BOUNDED_BELOW(hint_descriptor)) {
        LADSPA_Data lower = desc->port_range_hints[j].LowerBound;
        if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor))
            lower *= sample_rate;
        if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
            if (lower < FLT_EPSILON)
                lower = FLT_EPSILON;
        }
        mlt_properties_set_double(p, "minimum", lower);
    }
    if (LADSPA_IS_HINT_BOUNDED_ABOVE(hint_descriptor)) {
        LADSPA_Data upper = desc->port_range_hints[j].UpperBound;
        if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor))
            upper *= sample_rate;
        mlt_properties_set_double(p, "maximum", upper);
    }
    if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor))
        mlt_properties_set(p, "scale", "log");
    mlt_properties_set(p, "mutable", "yes");
    mlt_properties_set(p, "animation", "yes");
}

#endif

static mlt_properties metadata(mlt_service_type type, const char *id, char *data)
{
    char file[PATH_MAX];
    if (type == mlt_service_filter_type) {
        snprintf(file,
                 PATH_MAX,
                 "%s/jackrack/%s",
                 mlt_environment("MLT_DATA"),
                 strncmp(id, "ladspa.", 7) ? data : "filter_ladspa.yml");
    } else {
        snprintf(file,
                 PATH_MAX,
                 "%s/jackrack/%s",
                 mlt_environment("MLT_DATA"),
                 strncmp(id, "ladspa.", 7) ? data : "producer_ladspa.yml");
    }
    mlt_properties result = mlt_properties_parse_yaml(file);

#ifdef GPL
    if (!strncmp(id, "ladspa.", 7)) {
        // Annotate the yaml properties with ladspa control port info.
        plugin_desc_t *desc = plugin_mgr_get_any_desc(g_jackrack_plugin_mgr,
                                                      strtol(id + 7, NULL, 10));

        if (desc) {
            mlt_properties params = mlt_properties_new();
            mlt_properties p;
            char key[20];
            int i;

            mlt_properties_set(result, "identifier", id);
            mlt_properties_set(result, "title", desc->name);
            mlt_properties_set(result, "creator", desc->maker ? desc->maker : "unknown");
            mlt_properties_set(result, "description", "LADSPA plugin");
            mlt_properties_set_data(result,
                                    "parameters",
                                    params,
                                    0,
                                    (mlt_destructor) mlt_properties_close,
                                    NULL);
            for (i = 0; i < desc->control_port_count; i++) {
                int j = desc->control_port_indicies[i];
                p = mlt_properties_new();
                snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
                mlt_properties_set_data(params,
                                        key,
                                        p,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);
                snprintf(key, sizeof(key), "%d", j);
                mlt_properties_set(p, "identifier", key);
                add_port_to_metadata(p, desc, j);
                mlt_properties_set(p, "mutable", "yes");
            }
            for (i = 0; i < desc->status_port_count; i++) {
                int j = desc->status_port_indicies[i];
                p = mlt_properties_new();
                snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
                mlt_properties_set_data(params,
                                        key,
                                        p,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);
                snprintf(key, sizeof(key), "%d[*]", j);
                mlt_properties_set(p, "identifier", key);
                add_port_to_metadata(p, desc, j);
                mlt_properties_set(p, "readonly", "yes");
            }

            p = mlt_properties_new();
            snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
            mlt_properties_set_data(params, key, p, 0, (mlt_destructor) mlt_properties_close, NULL);
            mlt_properties_set(p, "identifier", "instances");
            mlt_properties_set(p, "title", "Instances");
            mlt_properties_set(p,
                               "description",
                               "The number of instances of the plugin that are in use.\n"
                               "MLT will create the number of plugins that are required "
                               "to support the number of audio channels.\n"
                               "Status parameters (readonly) are provided for each instance "
                               "and are accessed by specifying the instance number after the "
                               "identifier (starting at zero).\n"
                               "e.g. 9[0] provides the value of status 9 for the first instance.");
            mlt_properties_set(p, "type", "integer");
            mlt_properties_set(p, "readonly", "yes");

            if (type == mlt_service_filter_type) {
                p = mlt_properties_new();
                snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
                mlt_properties_set_data(params,
                                        key,
                                        p,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);
                mlt_properties_set(p, "identifier", "wetness");
                mlt_properties_set(p, "title", "Wet/Dry");
                mlt_properties_set(p, "type", "float");
                mlt_properties_set_double(p, "default", 1);
                mlt_properties_set_double(p, "minimum", 0);
                mlt_properties_set_double(p, "maximum", 1);
                mlt_properties_set(p, "mutable", "yes");
                mlt_properties_set(p, "animation", "yes");
            }
        }
    }
#endif

    return result;
}

#ifdef WITH_LV2

static void lv2_add_port_to_metadata(mlt_properties p, lv2_plugin_desc_t *desc, int j)
{
    LADSPA_PortRangeHintDescriptor hint_descriptor = desc->port_range_hints[j].HintDescriptor;

    mlt_properties_set(p, "title", desc->port_names[j]);
    if (LADSPA_IS_HINT_INTEGER(hint_descriptor)) {
        mlt_properties_set(p, "type", "integer");
        mlt_properties_set_int(p, "default", (int) desc->def_values[j]);
        mlt_properties_set_double(p, "minimum", (int) desc->min_values[j]);
        mlt_properties_set_double(p, "maximum", (int) desc->max_values[j]);
    } else if (LADSPA_IS_HINT_TOGGLED(hint_descriptor)) {
        mlt_properties_set(p, "type", "boolean");
        mlt_properties_set_int(p, "default", desc->def_values[j]);
    } else {
        mlt_properties_set(p, "type", "float");
        mlt_properties_set_double(p, "default", desc->def_values[j]);
        mlt_properties_set_double(p, "minimum", desc->min_values[j]);
        mlt_properties_set_double(p, "maximum", desc->max_values[j]);
    }

    if (LADSPA_IS_HINT_ENUMERATION(hint_descriptor)) {
        mlt_properties_set(p, "type", "string");

        char *str_ptr = strchr(desc->uri, '^');
        while (str_ptr != NULL) {
            *str_ptr++ = ':';
            str_ptr = strchr(str_ptr, '^');
        }

        LilvNode *puri_temp = lilv_new_uri(g_lv2_plugin_mgr->lv2_world, desc->uri);

        str_ptr = strchr(desc->uri, ':');
        while (str_ptr != NULL) {
            *str_ptr++ = '^';
            str_ptr = strchr(str_ptr, ':');
        }

        const LilvPlugin *p_temp = lilv_plugins_get_by_uri(g_lv2_plugin_mgr->plugin_list, puri_temp);
        const LilvPort *port_temp = lilv_plugin_get_port_by_index(p_temp, j);

        lilv_node_free(puri_temp);

        mlt_properties values_temp = mlt_properties_new();
        mlt_properties_set_data(p,
                                "values",
                                values_temp,
                                0,
                                (mlt_destructor) mlt_properties_close,
                                NULL);

        // Fill scalePoints Map
        LilvScalePoints *sp = lilv_port_get_scale_points(p_temp, port_temp);
        if (sp) {
            LILV_FOREACH(scale_points, s, sp)
            {
                const LilvScalePoint *p = lilv_scale_points_get(sp, s);
                const LilvNode *val = lilv_scale_point_get_value(p);
                if (!lilv_node_is_float(val) && !lilv_node_is_int(val)) {
                    continue;
                }

                const float f = lilv_node_as_float(val);

                char key_temp[20];

                if (lilv_node_is_float(val)) {
                    snprintf(key_temp, 20, "%f", f);
                } else if (lilv_node_is_int(val)) {
                    snprintf(key_temp, 20, "%d", (int) f);
                }

                mlt_properties_set(values_temp,
                                   key_temp,
                                   lilv_node_as_string(lilv_scale_point_get_label(p)));
            }

            lilv_scale_points_free(sp);
        }
    }

    if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor))
        mlt_properties_set(p, "scale", "log");
    mlt_properties_set(p, "mutable", "yes");
    mlt_properties_set(p, "animation", "yes");
}

static mlt_properties lv2_metadata(mlt_service_type type, const char *id, char *data)
{
    char file[PATH_MAX];
    if (type == mlt_service_filter_type) {
        snprintf(file,
                 PATH_MAX,
                 "%s/jackrack/%s",
                 mlt_environment("MLT_DATA"),
                 strncmp(id, "lv2.", 4) ? data : "filter_lv2.yml");
    } else {
        snprintf(file,
                 PATH_MAX,
                 "%s/jackrack/%s",
                 mlt_environment("MLT_DATA"),
                 strncmp(id, "lv2.", 4) ? data : "producer_lv2.yml");
    }
    mlt_properties result = mlt_properties_parse_yaml(file);

    if (!strncmp(id, "lv2.", 4)) {
        // Annotate the yaml properties with lv2 control port info.
        lv2_plugin_desc_t *desc = lv2_mgr_get_any_desc(g_lv2_plugin_mgr, (char *) &id[4]);

        if (desc) {
            mlt_properties params = mlt_properties_new();
            mlt_properties p;
            char key[20];
            int i;

            mlt_properties_set(result, "identifier", id);
            mlt_properties_set(result, "title", desc->name);
            mlt_properties_set(result, "creator", desc->maker ? desc->maker : "unknown");
            mlt_properties_set(result, "description", "LV2 plugin");
            mlt_properties_set_data(result,
                                    "parameters",
                                    params,
                                    0,
                                    (mlt_destructor) mlt_properties_close,
                                    NULL);
            for (i = 0; i < desc->control_port_count; i++) {
                int j = desc->control_port_indicies[i];
                p = mlt_properties_new();
                snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
                mlt_properties_set_data(params,
                                        key,
                                        p,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);
                snprintf(key, sizeof(key), "%d", j);
                mlt_properties_set(p, "identifier", key);
                lv2_add_port_to_metadata(p, desc, j);
                mlt_properties_set(p, "mutable", "yes");
            }
            for (i = 0; i < desc->status_port_count; i++) {
                int j = desc->status_port_indicies[i];
                p = mlt_properties_new();
                snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
                mlt_properties_set_data(params,
                                        key,
                                        p,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);
                snprintf(key, sizeof(key), "%d[*]", j);
                mlt_properties_set(p, "identifier", key);
                lv2_add_port_to_metadata(p, desc, j);
                mlt_properties_set(p, "readonly", "yes");
            }

            p = mlt_properties_new();
            snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
            mlt_properties_set_data(params, key, p, 0, (mlt_destructor) mlt_properties_close, NULL);
            mlt_properties_set(p, "identifier", "instances");
            mlt_properties_set(p, "title", "Instances");
            mlt_properties_set(p,
                               "description",
                               "The number of instances of the plugin that are in use.\n"
                               "MLT will create the number of plugins that are required "
                               "to support the number of audio channels.\n"
                               "Status parameters (readonly) are provided for each instance "
                               "and are accessed by specifying the instance number after the "
                               "identifier (starting at zero).\n"
                               "e.g. 9[0] provides the value of status 9 for the first instance.");
            mlt_properties_set(p, "type", "integer");
            mlt_properties_set(p, "readonly", "yes");

            if (type == mlt_service_filter_type) {
                p = mlt_properties_new();
                snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
                mlt_properties_set_data(params,
                                        key,
                                        p,
                                        0,
                                        (mlt_destructor) mlt_properties_close,
                                        NULL);
                mlt_properties_set(p, "identifier", "wetness");
                mlt_properties_set(p, "title", "Wet/Dry");
                mlt_properties_set(p, "type", "float");
                mlt_properties_set_double(p, "default", 1);
                mlt_properties_set_double(p, "minimum", 0);
                mlt_properties_set_double(p, "maximum", 1);
                mlt_properties_set(p, "mutable", "yes");
                mlt_properties_set(p, "animation", "yes");
            }
        }
    }

    return result;
}

#endif

#ifdef WITH_VST2

static void vst2_add_port_to_metadata(mlt_properties p, vst2_plugin_desc_t *desc, int j)
{
  LADSPA_Data sample_rate = 48000;
  LADSPA_PortRangeHintDescriptor hint_descriptor = desc->port_range_hints[j].HintDescriptor;
      
  mlt_properties_set(p, "title", desc->port_names[j]);
  if (LADSPA_IS_HINT_INTEGER(hint_descriptor)) {
    mlt_properties_set(p, "type", "integer");
    mlt_properties_set_int(p,
   			   "default",
   			   vst2_plugin_desc_get_default_control_value(desc, j-(desc->effect->numInputs+desc->effect->numOutputs), sample_rate));
  } else if (LADSPA_IS_HINT_TOGGLED(hint_descriptor)) {
    mlt_properties_set(p, "type", "boolean");
    mlt_properties_set_int(p,
   			   "default",
   			   vst2_plugin_desc_get_default_control_value(desc, j-(desc->effect->numInputs+desc->effect->numOutputs), sample_rate));
  } else {
    mlt_properties_set(p, "type", "float");
    mlt_properties_set_double(p,
   			      "default",
   			      vst2_plugin_desc_get_default_control_value(desc, j-(desc->effect->numInputs+desc->effect->numOutputs), sample_rate));
  }
  /* set upper and lower, possibly adjusted to the sample rate */
  if (LADSPA_IS_HINT_BOUNDED_BELOW(hint_descriptor)) {
    LADSPA_Data lower = desc->port_range_hints[j].LowerBound;
    if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor))
      lower *= sample_rate;
    if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
      if (lower < FLT_EPSILON)
   	lower = FLT_EPSILON;
    }
    mlt_properties_set_double(p, "minimum", lower);
  }
  if (LADSPA_IS_HINT_BOUNDED_ABOVE(hint_descriptor)) {
    LADSPA_Data upper = desc->port_range_hints[j].UpperBound;
    if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor))
      upper *= sample_rate;
    mlt_properties_set_double(p, "maximum", upper);
  }
  if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor))
    mlt_properties_set(p, "scale", "log");
  mlt_properties_set(p, "mutable", "yes");
  mlt_properties_set(p, "animation", "yes");
}
   
static mlt_properties vst2_metadata(mlt_service_type type, const char *id, char *data)
{
  char file[PATH_MAX];
  if (type == mlt_service_filter_type) {
    snprintf(file,
	     PATH_MAX,
	     "%s/jackrack/%s",
	     mlt_environment("MLT_DATA"),
	     strncmp(id, "vst2.", 5) ? data : "filter_vst2.yml");
  } else {
    snprintf(file,
	     PATH_MAX,
	     "%s/jackrack/%s",
	     mlt_environment("MLT_DATA"),
	     strncmp(id, "vst2.", 5) ? data : "producer_vst2.yml");
  }
  mlt_properties result = mlt_properties_parse_yaml(file);
   
#ifdef GPL
  if (!strncmp(id, "vst2.", 5)) {
    // Annotate the yaml properties with ladspa control port info.
    vst2_plugin_desc_t *desc = vst2_mgr_get_any_desc(g_vst2_plugin_mgr,
						     strtol(id + 5, NULL, 10));
   
    if (desc) {
      mlt_properties params = mlt_properties_new();
      mlt_properties p;
      char key[20];
      int i;
   
      mlt_properties_set(result, "identifier", id);
      mlt_properties_set(result, "title", desc->name);
      mlt_properties_set(result, "creator", desc->maker ? desc->maker : "unknown");
      mlt_properties_set(result, "description", "VST2 plugin");
      mlt_properties_set_data(result,
			      "parameters",
			      params,
			      0,
			      (mlt_destructor) mlt_properties_close,
			      NULL);
      for (i = 0; i < desc->control_port_count; i++) {
	int j = desc->control_port_indicies[i];

	p = mlt_properties_new();
	snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
	mlt_properties_set_data(params,
				key,
				p,
				0,
				(mlt_destructor) mlt_properties_close,
				NULL);
	snprintf(key, sizeof(key), "%d", j-(desc->effect->numInputs+desc->effect->numOutputs));
	mlt_properties_set(p, "identifier", key);
	vst2_add_port_to_metadata(p, desc, j);
	mlt_properties_set(p, "mutable", "yes");
      }
      /* for (i = 0; i < desc->status_port_count; i++) {
         	int j = desc->status_port_indicies[i];
         	p = mlt_properties_new();
         	snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
         	mlt_properties_set_data(params,
         				key,
         				p,
         				0,
         				(mlt_destructor) mlt_properties_close,
         				NULL);
         	snprintf(key, sizeof(key), "%d[*]", j);
         	mlt_properties_set(p, "identifier", key);
         	vst2_add_port_to_metadata(p, desc, j);
         	mlt_properties_set(p, "readonly", "yes");
         } */
   
      p = mlt_properties_new();
      snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
      mlt_properties_set_data(params, key, p, 0, (mlt_destructor) mlt_properties_close, NULL);
      mlt_properties_set(p, "identifier", "instances");
      mlt_properties_set(p, "title", "Instances");
      mlt_properties_set(p,
			 "description",
			 "The number of instances of the plugin that are in use.\n"
			 "MLT will create the number of plugins that are required "
			 "to support the number of audio channels.\n"
			 "Status parameters (readonly) are provided for each instance "
			 "and are accessed by specifying the instance number after the "
			 "identifier (starting at zero).\n"
			 "e.g. 9[0] provides the value of status 9 for the first instance.");
      mlt_properties_set(p, "type", "integer");
      mlt_properties_set(p, "readonly", "yes");
   
      if (type == mlt_service_filter_type) {
	p = mlt_properties_new();
	snprintf(key, sizeof(key), "%d", mlt_properties_count(params));
	mlt_properties_set_data(params,
				key,
				p,
				0,
				(mlt_destructor) mlt_properties_close,
				NULL);
	mlt_properties_set(p, "identifier", "wetness");
	mlt_properties_set(p, "title", "Wet/Dry");
	mlt_properties_set(p, "type", "float");
	mlt_properties_set_double(p, "default", 1);
	mlt_properties_set_double(p, "minimum", 0);
	mlt_properties_set_double(p, "maximum", 1);
	mlt_properties_set(p, "mutable", "yes");
	mlt_properties_set(p, "animation", "yes");
      }
    }
  }
#endif
   
  return result;
}

#endif

MLT_REPOSITORY
{
#ifdef GPL
    GSList *list;
    g_jackrack_plugin_mgr = plugin_mgr_new();

    for (list = g_jackrack_plugin_mgr->all_plugins; list; list = g_slist_next(list)) {
        plugin_desc_t *desc = (plugin_desc_t *) list->data;
        char *s = malloc(strlen("ladpsa.") + 21);

        sprintf(s, "ladspa.%lu", desc->id);

        if (desc->has_input) {
            MLT_REGISTER(mlt_service_filter_type, s, filter_ladspa_init);
            MLT_REGISTER_METADATA(mlt_service_filter_type, s, metadata, NULL);
        } else {
            MLT_REGISTER(mlt_service_producer_type, s, producer_ladspa_init);
            MLT_REGISTER_METADATA(mlt_service_producer_type, s, metadata, NULL);
        }

        free(s);
    }
    mlt_factory_register_for_clean_up(g_jackrack_plugin_mgr, (mlt_destructor) plugin_mgr_destroy);

#ifdef WITH_LV2
    g_lv2_plugin_mgr = lv2_mgr_new();

    char global_lv2_world[20];
    snprintf(global_lv2_world, 20, "%p", g_lv2_plugin_mgr->lv2_world);
    mlt_environment_set("global_lv2_world", global_lv2_world);

    for (list = g_lv2_plugin_mgr->all_plugins; list; list = g_slist_next(list)) {
        lv2_plugin_desc_t *desc = (lv2_plugin_desc_t *) list->data;
        char *s = NULL;
        s = calloc(1, strlen("lv2.") + strlen(desc->uri) + 1);

        sprintf(s, "lv2.%s", desc->uri);

        char *str_ptr = strchr(s, ':');
        while (str_ptr != NULL) {
            *str_ptr++ = '^';
            str_ptr = strchr(str_ptr, ':');
        }

        if (desc->has_input) {
            MLT_REGISTER(mlt_service_filter_type, s, filter_lv2_init);
            MLT_REGISTER_METADATA(mlt_service_filter_type, s, lv2_metadata, NULL);
        } else {
            MLT_REGISTER(mlt_service_producer_type, s, producer_lv2_init);
            MLT_REGISTER_METADATA(mlt_service_producer_type, s, lv2_metadata, NULL);
        }

        if (s) {
            free(s);
        }
    }
#endif

#ifdef WITH_VST2
   
    g_vst2_plugin_mgr = vst2_mgr_new();
   
    for (list = g_vst2_plugin_mgr->all_plugins; list; list = g_slist_next(list)) {
      vst2_plugin_desc_t *desc = (vst2_plugin_desc_t *) list->data;
      char *s = malloc(strlen("vst2.") + 21);
   
      sprintf(s, "vst2.%lu", desc->id);
   
      if (desc->has_input) {
   	MLT_REGISTER(mlt_service_filter_type, s, filter_vst2_init);
   	MLT_REGISTER_METADATA(mlt_service_filter_type, s, vst2_metadata, NULL);
      } else {
   	MLT_REGISTER(mlt_service_producer_type, s, producer_vst2_init);
   	MLT_REGISTER_METADATA(mlt_service_producer_type, s, vst2_metadata, NULL);
      }
   
      free(s);
    }
    mlt_factory_register_for_clean_up(g_vst2_plugin_mgr, (mlt_destructor) vst2_mgr_destroy);
   
#endif

#ifdef WITH_JACK
    MLT_REGISTER(mlt_service_filter_type, "jack", filter_jackrack_init);
    MLT_REGISTER_METADATA(mlt_service_filter_type, "jack", metadata, "filter_jack.yml");
    MLT_REGISTER(mlt_service_filter_type, "jackrack", filter_jackrack_init);
    MLT_REGISTER_METADATA(mlt_service_filter_type, "jackrack", metadata, "filter_jackrack.yml");
#endif
    MLT_REGISTER(mlt_service_filter_type, "ladspa", filter_ladspa_init);
    MLT_REGISTER_METADATA(mlt_service_filter_type, "ladspa", metadata, "filter_ladspa.yml");
#endif
#ifdef WITH_JACK
    MLT_REGISTER(mlt_service_consumer_type, "jack", consumer_jack_init);
    MLT_REGISTER_METADATA(mlt_service_consumer_type, "jack", metadata, "consumer_jack.yml");
#endif
}
