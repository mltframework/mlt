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

#include <math.h>
#include <float.h>
#include <string.h>

#include "plugin_desc.h"
#include "plugin.h"

#define set_string_property(property, value) \
  \
  if (property) \
    g_free (property); \
  \
  if (value) \
    (property) = g_strdup (value); \
  else \
    (property) = NULL;


void
plugin_desc_set_ports (plugin_desc_t * pd,
                       unsigned long port_count,
 	               const LADSPA_PortDescriptor * port_descriptors,
                       const LADSPA_PortRangeHint * port_range_hints,
                       const char * const * port_names);



static void
plugin_desc_init (plugin_desc_t * pd)
{
  pd->object_file      = NULL;
  pd->id               = 0;
  pd->name             = NULL;
  pd->maker            = NULL;
  pd->properties       = 0;
  pd->channels         = 0;
  pd->port_count       = 0;
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

static void
plugin_desc_free_ports (plugin_desc_t * pd)
{
  if (pd->port_count)
    {
      g_free (pd->port_descriptors);
      g_free (pd->port_range_hints);
      pd->port_descriptors = NULL;
      pd->port_range_hints = NULL;
      pd->port_count = 0;
    }
}

static void
plugin_desc_free (plugin_desc_t * pd)
{
  plugin_desc_set_object_file (pd, NULL);
  plugin_desc_set_name        (pd, NULL);
  plugin_desc_set_maker       (pd, NULL);
  plugin_desc_free_ports      (pd);
}

plugin_desc_t *
plugin_desc_new ()
{
  plugin_desc_t * pd;
  pd = g_malloc (sizeof (plugin_desc_t));
  plugin_desc_init (pd);
  return pd;
}

plugin_desc_t *
plugin_desc_new_with_descriptor (const char * object_file,
                                 unsigned long index,
                                 const LADSPA_Descriptor * descriptor)
{
  plugin_desc_t * pd;
  pd = plugin_desc_new ();
  
  plugin_desc_set_object_file (pd, object_file);
  plugin_desc_set_index       (pd, index);
  plugin_desc_set_id          (pd, descriptor->UniqueID);
  plugin_desc_set_name        (pd, descriptor->Name);
  plugin_desc_set_maker       (pd, descriptor->Maker);
  plugin_desc_set_properties  (pd, descriptor->Properties);
  plugin_desc_set_ports       (pd,
                               descriptor->PortCount,
                               descriptor->PortDescriptors,
                               descriptor->PortRangeHints,
                               descriptor->PortNames);
  
  pd->rt = LADSPA_IS_HARD_RT_CAPABLE(pd->properties) ? TRUE : FALSE;

  return pd;
}

void
plugin_desc_destroy (plugin_desc_t * pd)
{
  plugin_desc_free (pd);
  g_free (pd);
}

void
plugin_desc_set_object_file (plugin_desc_t * pd, const char * object_file)
{
  set_string_property (pd->object_file, object_file);
}

void
plugin_desc_set_index          (plugin_desc_t * pd, unsigned long index)
{
  pd->index = index;
}


void
plugin_desc_set_id          (plugin_desc_t * pd, unsigned long id)
{
  pd->id = id;
}

void
plugin_desc_set_name        (plugin_desc_t * pd, const char * name)
{
  set_string_property (pd->name, name);
}

void
plugin_desc_set_maker       (plugin_desc_t * pd, const char * maker)
{
  set_string_property (pd->maker, maker);
}

void
plugin_desc_set_properties  (plugin_desc_t * pd, LADSPA_Properties properties)
{
  pd->properties = properties;
}

static void
plugin_desc_add_audio_port_index (unsigned long ** indicies,
                                  unsigned long * current_port_count,
                                  unsigned long index)
{
  (*current_port_count)++;
  
  if (*current_port_count == 0)
    *indicies = g_malloc (sizeof (unsigned long) * *current_port_count);
  else
    *indicies = g_realloc (*indicies, sizeof (unsigned long) * *current_port_count);
  
  (*indicies)[*current_port_count - 1] = index;
}

static void
plugin_desc_set_port_counts (plugin_desc_t * pd)
{
  unsigned long i;
  unsigned long icount = 0;
  unsigned long ocount = 0;
  
  for (i = 0; i < pd->port_count; i++)
    {
      if (LADSPA_IS_PORT_AUDIO (pd->port_descriptors[i]))
        {
          if (LADSPA_IS_PORT_INPUT (pd->port_descriptors[i]))
            plugin_desc_add_audio_port_index (&pd->audio_input_port_indicies, &icount, i);
          else
            plugin_desc_add_audio_port_index (&pd->audio_output_port_indicies, &ocount, i);
        }
      else
        {
          if (LADSPA_IS_PORT_OUTPUT (pd->port_descriptors[i]))
            {
              pd->status_port_count++;
              if (pd->status_port_count == 0)
                pd->status_port_indicies = g_malloc (sizeof (unsigned long) * pd->status_port_count);
              else
                pd->status_port_indicies = g_realloc (pd->status_port_indicies,
                                                       sizeof (unsigned long) * pd->status_port_count);
              pd->status_port_indicies[pd->status_port_count - 1] = i;
            }
          else
            {
              pd->control_port_count++;
              if (pd->control_port_count == 0)
                pd->control_port_indicies = g_malloc (sizeof (unsigned long) * pd->control_port_count);
              else
                pd->control_port_indicies = g_realloc (pd->control_port_indicies,
                                                       sizeof (unsigned long) * pd->control_port_count);
              pd->control_port_indicies[pd->control_port_count - 1] = i;
            }
        }
    }
  
  if (icount == ocount)
    pd->channels = icount;
  else if( icount == 0 )
    {
      pd->channels = ocount;
      pd->has_input = FALSE;
    }
  else
    { /* deal with auxilliary ports */
      unsigned long ** port_indicies;
      unsigned long port_count;
      unsigned long i, j;
     
      if (icount > ocount)
        {
          pd->channels = ocount;
          pd->aux_channels = icount - ocount;
          pd->aux_are_input = TRUE;
          port_indicies = &pd->audio_input_port_indicies;
          port_count = icount;
        }
      else
        {
          pd->channels = icount;
          pd->aux_channels = ocount - icount;
          pd->aux_are_input = FALSE;
          port_indicies = &pd->audio_output_port_indicies;
          port_count = ocount;
        }
      
      /* allocate indicies */
      pd->audio_aux_port_indicies = g_malloc (sizeof (unsigned long) * pd->aux_channels);
      
      /* copy indicies */
      for (i = pd->channels, j = 0; i < port_count; i++, j++)
        pd->audio_aux_port_indicies[j] = (*port_indicies)[i];
      
      /* shrink the main indicies to only have channels indicies */
      *port_indicies = g_realloc (*port_indicies, sizeof (unsigned long) * pd->channels);
    }
}

void
plugin_desc_set_ports (plugin_desc_t * pd,
                       unsigned long port_count,
                       const LADSPA_PortDescriptor * port_descriptors,
                       const LADSPA_PortRangeHint * port_range_hints,
                       const char * const * port_names)
{
  unsigned long i;

  plugin_desc_free_ports (pd);
  
  if (!port_count)
    return;
  
  pd->port_count = port_count;
  pd->port_descriptors = g_malloc (sizeof (LADSPA_PortDescriptor) * port_count);
  pd->port_range_hints = g_malloc (sizeof (LADSPA_PortRangeHint) * port_count);
  pd->port_names       = g_malloc (sizeof (char *) * port_count);
  
  memcpy (pd->port_descriptors, port_descriptors, sizeof (LADSPA_PortDescriptor) * port_count);
  memcpy (pd->port_range_hints, port_range_hints, sizeof (LADSPA_PortRangeHint) * port_count);
  
  for (i = 0; i < port_count; i++)
    pd->port_names[i] = g_strdup (port_names[i]);
  
  plugin_desc_set_port_counts (pd);
}


LADSPA_Data
plugin_desc_get_default_control_value (plugin_desc_t * pd, unsigned long port_index, guint32 sample_rate)
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
  
  if (LADSPA_IS_HINT_LOGARITHMIC(hint_descriptor))
    {
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

LADSPA_Data
plugin_desc_change_control_value (plugin_desc_t * pd,
                                  unsigned long control_index,
                                  LADSPA_Data value,
                                  guint32 old_sample_rate,
                                  guint32 new_sample_rate)
{
  
  if (LADSPA_IS_HINT_SAMPLE_RATE (pd->port_range_hints[control_index].HintDescriptor))
    {
      LADSPA_Data old_sr, new_sr;
  
      old_sr = (LADSPA_Data) old_sample_rate;
      new_sr = (LADSPA_Data) new_sample_rate;
  
      value /= old_sr;
      value *= new_sr;
    }
  
  return value;
}

gint
plugin_desc_get_copies (plugin_desc_t * pd, unsigned long rack_channels)
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

/* EOF */
