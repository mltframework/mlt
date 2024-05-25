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

#include <float.h>
#include <math.h>
#include <string.h>

#include "plugin.h"
#include "plugin_desc.h"

#define set_string_property(property, value) \
\
    if (property) \
        g_free(property); \
\
    if (value) \
        (property) = g_strdup(value); \
    else \
        (property) = NULL;

#ifdef WITH_LV2

extern LilvNode *lv2_input_class;
extern LilvNode *lv2_output_class;
extern LilvNode *lv2_audio_class;
extern LilvNode *lv2_control_class;
extern LilvNode *lv2_atom_class;
extern LilvNode *lv2_integer_property;
extern LilvNode *lv2_logarithmic_property;
extern LilvNode *lv2_toggled_property;
extern LilvNode *lv2_enumeration_property;

#endif

void plugin_desc_set_ports(plugin_desc_t *pd,
                           unsigned long port_count,
                           const LADSPA_PortDescriptor *port_descriptors,
                           const LADSPA_PortRangeHint *port_range_hints,
                           const char *const *port_names);

static void plugin_desc_init(plugin_desc_t *pd)
{
    pd->object_file = NULL;
    pd->id = 0;
    pd->name = NULL;
    pd->maker = NULL;
    pd->properties = 0;
    pd->channels = 0;
    pd->port_count = 0;
    pd->port_descriptors = NULL;
    pd->port_range_hints = NULL;
    pd->audio_input_port_indicies = NULL;
    pd->audio_output_port_indicies = NULL;
    pd->audio_aux_port_indicies = NULL;
    pd->control_port_count = 0;
    pd->control_port_indicies = NULL;
    pd->status_port_count = 0;
    pd->status_port_indicies = NULL;
    pd->aux_channels = 0;
    pd->aux_are_input = TRUE;
    pd->has_input = TRUE;
}

static void plugin_desc_free_ports(plugin_desc_t *pd)
{
    if (pd->port_count) {
        g_free(pd->port_descriptors);
        g_free(pd->port_range_hints);
        g_free(pd->audio_input_port_indicies);
        g_free(pd->audio_output_port_indicies);
        g_free(pd->port_names);
        g_free(pd->control_port_indicies);
        g_free(pd->status_port_indicies);
        g_free(pd->audio_aux_port_indicies);
        pd->port_descriptors = NULL;
        pd->port_range_hints = NULL;
        pd->audio_input_port_indicies = NULL;
        pd->audio_output_port_indicies = NULL;
        pd->port_names = NULL;
        pd->control_port_indicies = NULL;
        pd->status_port_indicies = NULL;
        pd->audio_aux_port_indicies = NULL;

        pd->port_count = 0;
    }
}

static void plugin_desc_free(plugin_desc_t *pd)
{
    plugin_desc_set_object_file(pd, NULL);
    plugin_desc_set_name(pd, NULL);
    plugin_desc_set_maker(pd, NULL);
    plugin_desc_free_ports(pd);
}

plugin_desc_t *plugin_desc_new()
{
    plugin_desc_t *pd;
    pd = g_malloc(sizeof(plugin_desc_t));
    plugin_desc_init(pd);
    return pd;
}

plugin_desc_t *plugin_desc_new_with_descriptor(const char *object_file,
                                               unsigned long index,
                                               const LADSPA_Descriptor *descriptor)
{
    plugin_desc_t *pd;
    pd = plugin_desc_new();

    plugin_desc_set_object_file(pd, object_file);
    plugin_desc_set_index(pd, index);
    plugin_desc_set_id(pd, descriptor->UniqueID);
    plugin_desc_set_name(pd, descriptor->Name);
    plugin_desc_set_maker(pd, descriptor->Maker);
    plugin_desc_set_properties(pd, descriptor->Properties);
    plugin_desc_set_ports(pd,
                          descriptor->PortCount,
                          descriptor->PortDescriptors,
                          descriptor->PortRangeHints,
                          descriptor->PortNames);

    pd->rt = LADSPA_IS_HARD_RT_CAPABLE(pd->properties) ? TRUE : FALSE;

    return pd;
}

void plugin_desc_destroy(plugin_desc_t *pd)
{
    plugin_desc_free(pd);
    g_free(pd);
}

void plugin_desc_set_object_file(plugin_desc_t *pd, const char *object_file)
{
    set_string_property(pd->object_file, object_file);
}

void plugin_desc_set_index(plugin_desc_t *pd, unsigned long index)
{
    pd->index = index;
}

void plugin_desc_set_id(plugin_desc_t *pd, unsigned long id)
{
    pd->id = id;
}

void plugin_desc_set_name(plugin_desc_t *pd, const char *name)
{
    set_string_property(pd->name, name);
}

void plugin_desc_set_maker(plugin_desc_t *pd, const char *maker)
{
    set_string_property(pd->maker, maker);
}

void plugin_desc_set_properties(plugin_desc_t *pd, LADSPA_Properties properties)
{
    pd->properties = properties;
}

static void plugin_desc_add_audio_port_index(unsigned long **indices,
                                             unsigned long *current_port_count,
                                             unsigned long index)
{
    (*current_port_count)++;

    if (*current_port_count == 0)
        *indices = g_malloc(sizeof(unsigned long) * *current_port_count);
    else
        *indices = g_realloc(*indices, sizeof(unsigned long) * *current_port_count);

    (*indices)[*current_port_count - 1] = index;
}

static void plugin_desc_set_port_counts(plugin_desc_t *pd)
{
    unsigned long i;
    unsigned long icount = 0;
    unsigned long ocount = 0;

    for (i = 0; i < pd->port_count; i++) {
        if (LADSPA_IS_PORT_AUDIO(pd->port_descriptors[i])) {
            if (LADSPA_IS_PORT_INPUT(pd->port_descriptors[i]))
                plugin_desc_add_audio_port_index(&pd->audio_input_port_indicies, &icount, i);
            else
                plugin_desc_add_audio_port_index(&pd->audio_output_port_indicies, &ocount, i);
        } else {
            if (LADSPA_IS_PORT_OUTPUT(pd->port_descriptors[i])) {
                pd->status_port_count++;
                if (pd->status_port_count == 0)
                    pd->status_port_indicies = g_malloc(sizeof(unsigned long)
                                                        * pd->status_port_count);
                else
                    pd->status_port_indicies = g_realloc(pd->status_port_indicies,
                                                         sizeof(unsigned long)
                                                             * pd->status_port_count);
                pd->status_port_indicies[pd->status_port_count - 1] = i;
            } else {
                pd->control_port_count++;
                if (pd->control_port_count == 0)
                    pd->control_port_indicies = g_malloc(sizeof(unsigned long)
                                                         * pd->control_port_count);
                else
                    pd->control_port_indicies = g_realloc(pd->control_port_indicies,
                                                          sizeof(unsigned long)
                                                              * pd->control_port_count);
                pd->control_port_indicies[pd->control_port_count - 1] = i;
            }
        }
    }

    if (icount == ocount)
        pd->channels = icount;
    else if (icount == 0) {
        pd->channels = ocount;
        pd->has_input = FALSE;
    } else { /* deal with auxiliary ports */
        unsigned long **port_indicies;
        unsigned long port_count;
        unsigned long i, j;

        if (icount > ocount) {
            pd->channels = ocount;
            pd->aux_channels = icount - ocount;
            pd->aux_are_input = TRUE;
            port_indicies = &pd->audio_input_port_indicies;
            port_count = icount;
        } else {
            pd->channels = icount;
            pd->aux_channels = ocount - icount;
            pd->aux_are_input = FALSE;
            port_indicies = &pd->audio_output_port_indicies;
            port_count = ocount;
        }

        /* allocate indices */
        pd->audio_aux_port_indicies = g_malloc(sizeof(unsigned long) * pd->aux_channels);

        /* copy indices */
        for (i = pd->channels, j = 0; i < port_count; i++, j++)
            pd->audio_aux_port_indicies[j] = (*port_indicies)[i];

        /* shrink the main indices to only have channels indices */
        *port_indicies = g_realloc(*port_indicies, sizeof(unsigned long) * pd->channels);
    }
}

void plugin_desc_set_ports(plugin_desc_t *pd,
                           unsigned long port_count,
                           const LADSPA_PortDescriptor *port_descriptors,
                           const LADSPA_PortRangeHint *port_range_hints,
                           const char *const *port_names)
{
    unsigned long i;

    plugin_desc_free_ports(pd);

    if (!port_count)
        return;

    pd->port_count = port_count;
    pd->port_descriptors = g_malloc(sizeof(LADSPA_PortDescriptor) * port_count);
    pd->port_range_hints = g_malloc(sizeof(LADSPA_PortRangeHint) * port_count);
    pd->port_names = g_malloc(sizeof(char *) * port_count);

    memcpy(pd->port_descriptors, port_descriptors, sizeof(LADSPA_PortDescriptor) * port_count);
    memcpy(pd->port_range_hints, port_range_hints, sizeof(LADSPA_PortRangeHint) * port_count);

    for (i = 0; i < port_count; i++)
        pd->port_names[i] = g_strdup(port_names[i]);

    plugin_desc_set_port_counts(pd);
}

LADSPA_Data plugin_desc_get_default_control_value(plugin_desc_t *pd,
                                                  unsigned long port_index,
                                                  guint32 sample_rate)
{
    LADSPA_Data upper, lower;
    LADSPA_PortRangeHintDescriptor hint_descriptor;

    hint_descriptor = pd->port_range_hints[port_index].HintDescriptor;

    /* set upper and lower, possibly adjusted to the sample rate */
    if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
        upper = pd->port_range_hints[port_index].UpperBound * (LADSPA_Data) sample_rate;
        lower = pd->port_range_hints[port_index].LowerBound * (LADSPA_Data) sample_rate;
    } else {
        upper = pd->port_range_hints[port_index].UpperBound;
        lower = pd->port_range_hints[port_index].LowerBound;
    }

    if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
        if (lower < FLT_EPSILON)
            lower = FLT_EPSILON;
    }

    if (LADSPA_IS_HINT_HAS_DEFAULT(hint_descriptor)) {
        if (LADSPA_IS_HINT_DEFAULT_MINIMUM(hint_descriptor)) {
            return lower;

        } else if (LADSPA_IS_HINT_DEFAULT_LOW(hint_descriptor)) {
            if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
                return exp(log(lower) * 0.75 + log(upper) * 0.25);
            } else {
                return lower * 0.75 + upper * 0.25;
            }

        } else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(hint_descriptor)) {
            if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
                return exp(log(lower) * 0.5 + log(upper) * 0.5);
            } else {
                return lower * 0.5 + upper * 0.5;
            }

        } else if (LADSPA_IS_HINT_DEFAULT_HIGH(hint_descriptor)) {
            if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor)) {
                return exp(log(lower) * 0.25 + log(upper) * 0.75);
            } else {
                return lower * 0.25 + upper * 0.75;
            }

        } else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint_descriptor)) {
            return upper;

        } else if (LADSPA_IS_HINT_DEFAULT_0(hint_descriptor)) {
            return 0.0;

        } else if (LADSPA_IS_HINT_DEFAULT_1(hint_descriptor)) {
            if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
                return (LADSPA_Data) sample_rate;
            } else {
                return 1.0;
            }

        } else if (LADSPA_IS_HINT_DEFAULT_100(hint_descriptor)) {
            if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
                return 100.0 * (LADSPA_Data) sample_rate;
            } else {
                return 100.0;
            }

        } else if (LADSPA_IS_HINT_DEFAULT_440(hint_descriptor)) {
            if (LADSPA_IS_HINT_SAMPLE_RATE(hint_descriptor)) {
                return 440.0 * (LADSPA_Data) sample_rate;
            } else {
                return 440.0;
            }
        }

    } else { /* try and find a reasonable default */

        if (LADSPA_IS_HINT_BOUNDED_BELOW(hint_descriptor)) {
            return lower;
        } else if (LADSPA_IS_HINT_BOUNDED_ABOVE(hint_descriptor)) {
            return upper;
        }
    }

    return 0.0;
}

LADSPA_Data plugin_desc_change_control_value(plugin_desc_t *pd,
                                             unsigned long control_index,
                                             LADSPA_Data value,
                                             guint32 old_sample_rate,
                                             guint32 new_sample_rate)
{
    if (LADSPA_IS_HINT_SAMPLE_RATE(pd->port_range_hints[control_index].HintDescriptor)) {
        LADSPA_Data old_sr, new_sr;

        old_sr = (LADSPA_Data) old_sample_rate;
        new_sr = (LADSPA_Data) new_sample_rate;

        value /= old_sr;
        value *= new_sr;
    }

    return value;
}

gint plugin_desc_get_copies(plugin_desc_t *pd, unsigned long rack_channels)
{
    gint copies = 1;

    if (pd->channels > rack_channels)
        return 0;

    while (pd->channels * copies < rack_channels)
        copies++;

    if (pd->channels * copies > rack_channels)
        return 0;

    return copies;
}

#ifdef WITH_LV2
static void lv2_plugin_desc_init(lv2_plugin_desc_t *pd)
{
    pd->uri = NULL;
    pd->id = 0;
    pd->name = NULL;
    pd->maker = NULL;
    pd->properties = 0;
    pd->channels = 0;
    pd->port_count = 0;
    pd->port_descriptors = NULL;
    pd->port_range_hints = NULL;
    pd->audio_input_port_indicies = NULL;
    pd->audio_output_port_indicies = NULL;
    pd->audio_aux_port_indicies = NULL;
    pd->control_port_count = 0;
    pd->control_port_indicies = NULL;
    pd->status_port_count = 0;
    pd->status_port_indicies = NULL;
    pd->aux_channels = 0;
    pd->aux_are_input = TRUE;
    pd->has_input = TRUE;
}

void lv2_plugin_desc_set_uri(lv2_plugin_desc_t *pd, const char *uri)
{
    set_string_property(pd->uri, uri);
}

void lv2_plugin_desc_set_index(lv2_plugin_desc_t *pd, unsigned long index)
{
    pd->index = index;
}

void lv2_plugin_desc_set_id(lv2_plugin_desc_t *pd, unsigned long id)
{
    pd->id = id;
}

void lv2_plugin_desc_set_name(lv2_plugin_desc_t *pd, const char *name)
{
    set_string_property(pd->name, name);
}

void lv2_plugin_desc_set_maker(lv2_plugin_desc_t *pd, const char *maker)
{
    set_string_property(pd->maker, maker);
}

void lv2_plugin_desc_set_properties(lv2_plugin_desc_t *pd, LADSPA_Properties properties)
{
    pd->properties = properties;
}

static void lv2_plugin_desc_set_port_counts(lv2_plugin_desc_t *pd)
{
    unsigned long i;
    unsigned long icount = 0;
    unsigned long ocount = 0;

    for (i = 0; i < pd->port_count; i++) {
        /* if (LADSPA_IS_PORT_ATOM (pd->port_descriptors[i]))
         	{
         	  if(yes) printf("%li atom port\n", i);
         	}
         else */
        if (LADSPA_IS_PORT_AUDIO(pd->port_descriptors[i])) {
            if (LADSPA_IS_PORT_INPUT(pd->port_descriptors[i])) {
                plugin_desc_add_audio_port_index(&pd->audio_input_port_indicies, &icount, i);
            } else {
                plugin_desc_add_audio_port_index(&pd->audio_output_port_indicies, &ocount, i);
            }
        } else {
            if (LADSPA_IS_PORT_OUTPUT(pd->port_descriptors[i])) {
                pd->status_port_count++;
                if (pd->status_port_count == 0)
                    pd->status_port_indicies = g_malloc(sizeof(unsigned long)
                                                        * pd->status_port_count);
                else
                    pd->status_port_indicies = g_realloc(pd->status_port_indicies,
                                                         sizeof(unsigned long)
                                                             * pd->status_port_count);
                pd->status_port_indicies[pd->status_port_count - 1] = i;
            } else {
                pd->control_port_count++;
                if (pd->control_port_count == 0)
                    pd->control_port_indicies = g_malloc(sizeof(unsigned long)
                                                         * pd->control_port_count);
                else
                    pd->control_port_indicies = g_realloc(pd->control_port_indicies,
                                                          sizeof(unsigned long)
                                                              * pd->control_port_count);
                pd->control_port_indicies[pd->control_port_count - 1] = i;
            }
        }
    }

    if (icount == ocount)
        pd->channels = icount;
    else if (icount == 0) {
        pd->channels = ocount;
        pd->has_input = FALSE;
    } else { /* deal with auxiliary ports */
        unsigned long **port_indicies;
        unsigned long port_count;
        unsigned long i, j;

        if (icount > ocount) {
            pd->channels = ocount;
            pd->aux_channels = icount - ocount;
            pd->aux_are_input = TRUE;
            port_indicies = &pd->audio_input_port_indicies;
            port_count = icount;
        } else {
            pd->channels = icount;
            pd->aux_channels = ocount - icount;
            pd->aux_are_input = FALSE;
            port_indicies = &pd->audio_output_port_indicies;
            port_count = ocount;
        }

        /* allocate indices */
        pd->audio_aux_port_indicies = g_malloc(sizeof(unsigned long) * pd->aux_channels);

        /* copy indices */
        for (i = pd->channels, j = 0; i < port_count; i++, j++)
            pd->audio_aux_port_indicies[j] = (*port_indicies)[i];

        /* shrink the main indices to only have channels indices */
        *port_indicies = g_realloc(*port_indicies, sizeof(unsigned long) * pd->channels);
    }
}

static void lv2_plugin_desc_free_ports(lv2_plugin_desc_t *pd)
{
    if (pd->port_count) {
        g_free(pd->port_descriptors);
        g_free(pd->port_range_hints);
        g_free(pd->audio_input_port_indicies);
        g_free(pd->audio_output_port_indicies);
        g_free(pd->port_names);
        g_free(pd->control_port_indicies);
        g_free(pd->status_port_indicies);
        g_free(pd->audio_aux_port_indicies);
        pd->port_descriptors = NULL;
        pd->port_range_hints = NULL;
        pd->audio_input_port_indicies = NULL;
        pd->audio_output_port_indicies = NULL;
        pd->port_names = NULL;
        pd->control_port_indicies = NULL;
        pd->status_port_indicies = NULL;
        pd->audio_aux_port_indicies = NULL;

        pd->port_count = 0;
    }
}

static void lv2_plugin_desc_free(lv2_plugin_desc_t *pd)
{
    g_free(pd->def_values);
    g_free(pd->min_values);
    g_free(pd->max_values);
    lv2_plugin_desc_set_uri(pd, NULL);
    lv2_plugin_desc_set_name(pd, NULL);
    lv2_plugin_desc_set_maker(pd, NULL);
    lv2_plugin_desc_free_ports(pd);
}

void lv2_plugin_desc_destroy(lv2_plugin_desc_t *pd)
{
    lv2_plugin_desc_free(pd);
    g_free(pd);
}

lv2_plugin_desc_t *lv2_plugin_desc_new()
{
    lv2_plugin_desc_t *pd;
    pd = g_malloc(sizeof(lv2_plugin_desc_t));
    lv2_plugin_desc_init(pd);
    return pd;
}

void lv2_plugin_desc_set_ports(lv2_plugin_desc_t *pd,
                               unsigned long port_count,
                               const LADSPA_PortDescriptor *port_descriptors,
                               const LADSPA_PortRangeHint *port_range_hints,
                               const char *const *port_names)
{
    unsigned long i;

    lv2_plugin_desc_free_ports(pd);

    if (!port_count)
        return;

    pd->port_count = port_count;
    pd->port_descriptors = g_malloc(sizeof(LADSPA_PortDescriptor) * port_count);
    pd->port_range_hints = g_malloc(sizeof(LADSPA_PortRangeHint) * port_count);
    pd->port_names = g_malloc(sizeof(char *) * port_count);

    memcpy(pd->port_descriptors, port_descriptors, sizeof(LADSPA_PortDescriptor) * port_count);
    memcpy(pd->port_range_hints, port_range_hints, sizeof(LADSPA_PortRangeHint) * port_count);

    for (i = 0; i < port_count; i++)
        pd->port_names[i] = g_strdup(port_names[i]);

    lv2_plugin_desc_set_port_counts(pd);
}

lv2_plugin_desc_t *lv2_plugin_desc_new_with_descriptor(const char *uri,
                                                       unsigned long index,
                                                       const LilvPlugin *plugin)
{
    lv2_plugin_desc_t *pd;
    pd = lv2_plugin_desc_new();

    LilvNode *val = NULL;

    char *str_ptr = strchr(uri, ':');
    while (str_ptr != NULL) {
        *str_ptr++ = '<';
        str_ptr = strchr(str_ptr, ':');
    }

    lv2_plugin_desc_set_uri(pd, uri);

    str_ptr = strchr(uri, '<');
    while (str_ptr != NULL) {
        *str_ptr++ = ':';
        str_ptr = strchr(str_ptr, '<');
    }

    lv2_plugin_desc_set_index(pd, index);

    val = lilv_plugin_get_name(plugin);

    lv2_plugin_desc_set_name(pd, lilv_node_as_string(val));

    lv2_plugin_desc_set_maker(pd, lilv_node_as_string(lilv_plugin_get_author_name(plugin)));

    int PortCount = lilv_plugin_get_num_ports(plugin);
    char **PortNames = calloc(PortCount, sizeof(char *));
    LADSPA_PortDescriptor *port_descriptors = calloc(PortCount, sizeof(LADSPA_PortDescriptor));
    LADSPA_PortRangeHint *PortRangeHints = calloc(PortCount, sizeof(LADSPA_PortRangeHint));

    pd->min_values = calloc(PortCount, sizeof(LADSPA_Data)),
    pd->max_values = calloc(PortCount, sizeof(LADSPA_Data)),
    pd->def_values = calloc(PortCount, sizeof(LADSPA_Data));

    lilv_plugin_get_port_ranges_float(plugin, pd->min_values, pd->max_values, pd->def_values);

    int i;
    for (i = 0; i < PortCount; ++i) {
        const LilvPort *port = lilv_plugin_get_port_by_index(plugin, i);

        if (lilv_port_is_a(plugin, port, lv2_audio_class)) {
            port_descriptors[i] |= LADSPA_PORT_AUDIO;
        }
        if (lilv_port_is_a(plugin, port, lv2_input_class)) {
            port_descriptors[i] |= LADSPA_PORT_INPUT;
        }
        if (lilv_port_is_a(plugin, port, lv2_output_class)) {
            port_descriptors[i] |= LADSPA_PORT_OUTPUT;
        }
        if (lilv_port_is_a(plugin, port, lv2_control_class)) {
            port_descriptors[i] |= LADSPA_PORT_CONTROL;
        }
        if (lilv_port_is_a(plugin, port, lv2_atom_class)) {
            port_descriptors[i] |= LADSPA_PORT_ATOM;
        }

        if (lilv_port_has_property(plugin, port, lv2_integer_property)) {
            PortRangeHints[i].HintDescriptor |= LADSPA_HINT_INTEGER;
        }

        if (lilv_port_has_property(plugin, port, lv2_logarithmic_property)) {
            PortRangeHints[i].HintDescriptor |= LADSPA_HINT_LOGARITHMIC;
        }

        if (lilv_port_has_property(plugin, port, lv2_toggled_property)) {
            PortRangeHints[i].HintDescriptor |= LADSPA_HINT_TOGGLED;
        }

        if (lilv_port_has_property(plugin, port, lv2_enumeration_property)) {
            PortRangeHints[i].HintDescriptor |= LADSPA_HINT_ENUMERATION;
        }

        PortRangeHints[i].LowerBound = pd->min_values[i];
        PortRangeHints[i].UpperBound = pd->max_values[i];

        PortNames[i] = (char *) lilv_node_as_string(lilv_port_get_name(plugin, port));
    }

    lv2_plugin_desc_set_ports(pd,
                              PortCount,
                              port_descriptors,
                              PortRangeHints,
                              (const char *const *) PortNames);

    free(PortNames);
    free(port_descriptors);

    return pd;
}

gint lv2_plugin_desc_get_copies(lv2_plugin_desc_t *pd, unsigned long rack_channels)
{
    gint copies = 1;

    if (pd->channels > rack_channels)
        return 0;

    while (pd->channels * copies < rack_channels)
        copies++;

    if (pd->channels * copies > rack_channels)
        return 0;

    return copies;
}
#endif

/* EOF */
