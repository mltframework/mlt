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

#ifndef __JR_PLUGIN_DESC_H__
#define __JR_PLUGIN_DESC_H__

#include <glib.h>
#include <ladspa.h>

#ifdef WITH_LV2

#define LADSPA_PORT_ATOM 10
#define LADSPA_IS_PORT_ATOM(x) ((x) & LADSPA_PORT_ATOM)
#define LADSPA_HINT_ENUMERATION LADSPA_HINT_DEFAULT_LOW
#define LADSPA_IS_HINT_ENUMERATION(x) ((x) & LADSPA_HINT_ENUMERATION)

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
#endif

#ifdef WITH_VST2

#include "vestige.h"

#endif

typedef struct _plugin_desc plugin_desc_t;

struct _plugin_desc
{
    char *object_file;
    unsigned long index;
    unsigned long id;
    char *name;
    char *maker;
    LADSPA_Properties properties;
    gboolean rt;

    unsigned long channels;

    gboolean aux_are_input;
    unsigned long aux_channels;

    unsigned long port_count;
    LADSPA_PortDescriptor *port_descriptors;
    LADSPA_PortRangeHint *port_range_hints;
    char **port_names;

    unsigned long *audio_input_port_indicies;
    unsigned long *audio_output_port_indicies;

    unsigned long *audio_aux_port_indicies;

    unsigned long control_port_count;
    unsigned long *control_port_indicies;

    unsigned long status_port_count;
    unsigned long *status_port_indicies;

    gboolean has_input;
};

plugin_desc_t *plugin_desc_new();
plugin_desc_t *plugin_desc_new_with_descriptor(const char *object_file,
                                               unsigned long index,
                                               const LADSPA_Descriptor *descriptor);
void plugin_desc_destroy(plugin_desc_t *pd);

void plugin_desc_set_object_file(plugin_desc_t *pd, const char *object_file);
void plugin_desc_set_index(plugin_desc_t *pd, unsigned long index);
void plugin_desc_set_id(plugin_desc_t *pd, unsigned long id);
void plugin_desc_set_name(plugin_desc_t *pd, const char *name);
void plugin_desc_set_maker(plugin_desc_t *pd, const char *maker);
void plugin_desc_set_properties(plugin_desc_t *pd, LADSPA_Properties properties);

struct _plugin *plugin_desc_instantiate(plugin_desc_t *pd);

LADSPA_Data plugin_desc_get_default_control_value(plugin_desc_t *pd,
                                                  unsigned long port_index,
                                                  guint32 sample_rate);
LADSPA_Data plugin_desc_change_control_value(
    plugin_desc_t *, unsigned long, LADSPA_Data, guint32, guint32);

gint plugin_desc_get_copies(plugin_desc_t *pd, unsigned long rack_channels);

#ifdef WITH_LV2
typedef struct _lv2_plugin_desc lv2_plugin_desc_t;

struct _lv2_plugin_desc
{
    char *uri; /* file name */
    unsigned long index;
    unsigned long id;
    char *name;
    char *maker;
    LADSPA_Properties properties;
    gboolean rt;

    unsigned long channels;

    gboolean aux_are_input;
    unsigned long aux_channels;

    unsigned long port_count;
    LADSPA_PortDescriptor *port_descriptors;
    LADSPA_PortRangeHint *port_range_hints;
    char **port_names;

    unsigned long *audio_input_port_indicies;
    unsigned long *audio_output_port_indicies;

    unsigned long *audio_aux_port_indicies;

    unsigned long control_port_count;
    unsigned long *control_port_indicies;

    unsigned long status_port_count;
    unsigned long *status_port_indicies;

    float *def_values, *min_values, *max_values;

    gboolean has_input;
};

lv2_plugin_desc_t *lv2_plugin_desc_new_with_descriptor(const char *uri,
                                                       unsigned long index,
                                                       const LilvPlugin *plugin);
void lv2_plugin_desc_destroy(lv2_plugin_desc_t *pd);
lv2_plugin_desc_t *lv2_plugin_desc_new();
void lv2_plugin_desc_set_uri(lv2_plugin_desc_t *pd, const char *uri);
void lv2_plugin_desc_set_index(lv2_plugin_desc_t *pd, unsigned long index);
void lv2_plugin_desc_set_id(lv2_plugin_desc_t *pd, unsigned long id);
void lv2_plugin_desc_set_name(lv2_plugin_desc_t *pd, const char *name);
void lv2_plugin_desc_set_maker(lv2_plugin_desc_t *pd, const char *maker);
void lv2_plugin_desc_set_properties(lv2_plugin_desc_t *pd, LADSPA_Properties properties);

struct _lv2_plugin *lv2_plugin_desc_instantiate(lv2_plugin_desc_t *pd);

LADSPA_Data lv2_plugin_desc_change_control_value(
    lv2_plugin_desc_t *, unsigned long, LADSPA_Data, guint32, guint32);

gint lv2_plugin_desc_get_copies(lv2_plugin_desc_t *pd, unsigned long rack_channels);
#endif

#ifdef WITH_VST2

typedef struct _vst2_plugin_desc vst2_plugin_desc_t;

struct _vst2_plugin_desc
{
    char *object_file;
    unsigned long index;
    unsigned long id;
    char *name;
    char *maker;
    LADSPA_Properties properties;
    AEffect *effect;
    gboolean rt;

    unsigned long channels;

    gboolean aux_are_input;
    unsigned long aux_channels;

    unsigned long port_count;
    LADSPA_PortDescriptor *port_descriptors;
    LADSPA_PortRangeHint *port_range_hints;
    char **port_names;

    unsigned long *audio_input_port_indicies;
    unsigned long *audio_output_port_indicies;

    unsigned long *audio_aux_port_indicies;

    unsigned long control_port_count;
    unsigned long *control_port_indicies;

    unsigned long status_port_count;
    unsigned long *status_port_indicies;

    float *def_values;

    gboolean has_input;
};

vst2_plugin_desc_t *vst2_plugin_desc_new();
vst2_plugin_desc_t *vst2_plugin_desc_new_with_descriptor(const char *object_file,
                                                         unsigned long index,
                                                         AEffect *descriptor);
void vst2_plugin_desc_destroy(vst2_plugin_desc_t *pd);

void vst2_plugin_desc_set_object_file(vst2_plugin_desc_t *pd, const char *object_file);
void vst2_plugin_desc_set_index(vst2_plugin_desc_t *pd, unsigned long index);
void vst2_plugin_desc_set_id(vst2_plugin_desc_t *pd, unsigned long id);
void vst2_plugin_desc_set_name(vst2_plugin_desc_t *pd, const char *name);
void vst2_plugin_desc_set_maker(vst2_plugin_desc_t *pd, const char *maker);
void vst2_plugin_desc_set_properties(vst2_plugin_desc_t *pd, LADSPA_Properties properties);

struct _vst2 *vst2_plugin_desc_instantiate(vst2_plugin_desc_t *pd);

LADSPA_Data vst2_plugin_desc_get_default_control_value(vst2_plugin_desc_t *pd,
                                                  unsigned long port_index,
                                                  guint32 sample_rate);
LADSPA_Data vst2_plugin_desc_change_control_value(
    vst2_plugin_desc_t *, unsigned long, LADSPA_Data, guint32, guint32);

gint vst2_plugin_desc_get_copies(vst2_plugin_desc_t *pd, unsigned long rack_channels);

#endif

#endif /* __JR_PLUGIN_DESC_H__ */
