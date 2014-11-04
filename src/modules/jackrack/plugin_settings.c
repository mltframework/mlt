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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <math.h>

#include "plugin_settings.h"


static void
settings_set_to_default (settings_t * settings, guint32 sample_rate)
{
  unsigned long control;
  guint copy;
  LADSPA_Data value;
  
  for (control = 0; control < settings->desc->control_port_count; control++)
    {
      value = plugin_desc_get_default_control_value (settings->desc, control, sample_rate);

      for (copy = 0; copy < settings->copies; copy++)
        {
          settings->control_values[copy][control] = value;
        }
          
      settings->locks[control] = TRUE;
    }
}

settings_t *
settings_new     (plugin_desc_t * desc, unsigned long channels, guint32 sample_rate)
{
  settings_t * settings;
  unsigned long channel;
  guint copies;
  
  settings = g_malloc (sizeof (settings_t));
  copies = plugin_desc_get_copies (desc, channels);
  
  settings->sample_rate = sample_rate;
  settings->desc = desc;
  settings->copies = copies;
  settings->channels = channels;
  settings->lock_all = TRUE;
  settings->enabled = FALSE;
  settings->locks = NULL;
  settings->control_values = NULL;
  settings->wet_dry_enabled = FALSE;
  settings->wet_dry_locked = TRUE;
  
  /* control settings */  
  if (desc->control_port_count > 0)
    {
      guint copy;
      
      settings->locks = g_malloc (sizeof (gboolean) * desc->control_port_count);

      settings->control_values = g_malloc (sizeof (LADSPA_Data *) * copies);
      for (copy = 0; copy < copies; copy++)
        {
          settings->control_values[copy] = g_malloc (sizeof (LADSPA_Data) * desc->control_port_count);
        }
      
      settings_set_to_default (settings, sample_rate);
    }
  
  /* wet/dry settings */
  settings->wet_dry_values = g_malloc (sizeof (LADSPA_Data) * channels);
  for (channel = 0; channel < channels; channel++)
    settings->wet_dry_values[channel] = 1.0;
  
  return settings;
}

settings_t *
settings_dup     (settings_t * other)
{
  settings_t * settings;
  plugin_desc_t * desc;
  unsigned long channel;
  
  settings = g_malloc (sizeof (settings_t));
  
  settings->sample_rate     = other->sample_rate;
  settings->desc            = other->desc;
  settings->copies          = settings_get_copies (other);
  settings->channels        = settings_get_channels (other);
  settings->wet_dry_enabled = settings_get_wet_dry_enabled (other);
  settings->wet_dry_locked  = settings_get_wet_dry_locked (other);
  settings->lock_all        = settings_get_lock_all (other);
  settings->enabled         = settings_get_enabled (other);
  settings->locks           = NULL;
  settings->control_values  = NULL;
  
  desc = other->desc;
  
  if (desc->control_port_count > 0)
    {
      guint copy;
      unsigned long control;
      
      settings->locks = g_malloc (sizeof (gboolean) * desc->control_port_count);
      for (control = 0; control < desc->control_port_count; control++)
        settings_set_lock (settings, control, settings_get_lock (other, control));

      settings->control_values = g_malloc (sizeof (LADSPA_Data *) * settings->copies);
      for (copy = 0; copy < settings->copies; copy++)
        {
          settings->control_values[copy] = g_malloc (sizeof (LADSPA_Data) * desc->control_port_count);

          for (control = 0; control < desc->control_port_count; control++)
            {
              settings->control_values[copy][control] = settings_get_control_value (other, copy, control);
            }
        }
    }
  
  settings->wet_dry_values = g_malloc (sizeof (LADSPA_Data) * settings->channels);
  for (channel = 0; channel < settings->channels; channel++)
    settings->wet_dry_values[channel] = settings_get_wet_dry_value (other, channel);
  
  return settings;
}

void
settings_destroy (settings_t * settings)
{
  if (settings->desc->control_port_count > 0)
    {
      guint i;
      for (i = 0; i < settings->copies; i++)
        g_free (settings->control_values[i]);

      g_free (settings->control_values);
      g_free (settings->locks);
    }
    
  g_free (settings->wet_dry_values);
  
  g_free (settings);
}

static void
settings_set_copies (settings_t * settings, guint copies)
{
  guint copy;
  guint last_copy;
  unsigned long control;
  
  if (copies <= settings->copies)
    return;
  
  last_copy = settings->copies - 1;
  
  settings->control_values = g_realloc (settings->control_values,
                                        sizeof (LADSPA_Data *) * copies);
  
  /* copy over the last settings to the new copies */
  for (copy = settings->copies; copy < copies; copy++)
    {
      for (control = 0; control < settings->desc->control_port_count; control++)
        {
          settings->control_values[copy][control] = 
            settings->control_values[last_copy][control];
        }
    }
  
  settings->copies = copies;
}

static void
settings_set_channels (settings_t * settings, unsigned long channels)
{
  unsigned long channel;
  LADSPA_Data last_value;
      
  if (channels <= settings->channels)
    return;
  
  settings->wet_dry_values = g_realloc (settings->wet_dry_values, sizeof (LADSPA_Data) * channels);
  
  last_value = settings->wet_dry_values[settings->channels - 1];
  
  for (channel = settings->channels; channel < channels; channel++)
    settings->wet_dry_values[channel] = last_value;
  
  settings->channels = channels;
}

void
settings_set_sample_rate (settings_t * settings, guint32 sample_rate)
{
  LADSPA_Data old_sample_rate;
  LADSPA_Data new_sample_rate;

  g_return_if_fail (settings != NULL);
  
  if (settings->sample_rate == sample_rate)
    return;
  
  if (settings->desc->control_port_count > 0)
    {
      unsigned long control;
      guint copy;
      
      new_sample_rate = (LADSPA_Data) sample_rate;
      old_sample_rate = (LADSPA_Data) settings->sample_rate;

      for (control = 0; control < settings->desc->control_port_count; control++)
        {
          for (copy = 0; copy < settings->copies; copy++)
            {
              if (LADSPA_IS_HINT_SAMPLE_RATE (settings->desc->port_range_hints[control].HintDescriptor))
                {
                  settings->control_values[copy][control] =
                    (settings->control_values[copy][control] / old_sample_rate) * new_sample_rate;
                }
            }
        }
    }
  
  settings->sample_rate = sample_rate;
}

void
settings_set_control_value (settings_t * settings, guint copy, unsigned long control_index, LADSPA_Data value)
{
  g_return_if_fail (settings != NULL);
  g_return_if_fail (control_index < settings->desc->control_port_count);
  
  if (copy >= settings->copies)
    settings_set_copies (settings, copy + 1);
  
  settings->control_values[copy][control_index] = value;
}

void
settings_set_lock          (settings_t * settings, unsigned long control_index, gboolean locked)
{
  g_return_if_fail (settings != NULL);
  g_return_if_fail (control_index < settings->desc->control_port_count);
  
  settings->locks[control_index] = locked;
}

void
settings_set_lock_all (settings_t * settings, gboolean lock_all)
{
  g_return_if_fail (settings != NULL);

  settings->lock_all = lock_all;
}

void
settings_set_enabled (settings_t * settings, gboolean enabled)
{
  g_return_if_fail (settings != NULL);

  settings->enabled = enabled;
}

void
settings_set_wet_dry_enabled (settings_t * settings, gboolean enabled)
{
  g_return_if_fail (settings != NULL);
  
  settings->wet_dry_enabled = enabled;
}

void
settings_set_wet_dry_locked  (settings_t * settings, gboolean locked)
{
  g_return_if_fail (settings != NULL);
  
  settings->wet_dry_locked = locked;
}

void
settings_set_wet_dry_value   (settings_t * settings, unsigned long channel, LADSPA_Data value)
{
  g_return_if_fail (settings != NULL);

  if (channel >= settings->channels)
    settings_set_channels (settings, channel + 1);
  
  settings->wet_dry_values[channel] = value;
}


LADSPA_Data
settings_get_control_value (settings_t * settings, guint copy, unsigned long control_index)
{
  g_return_val_if_fail (settings != NULL, NAN);
  g_return_val_if_fail (control_index < settings->desc->control_port_count, NAN);

  if (copy >= settings->copies)
    settings_set_copies (settings, copy - 1);

  return settings->control_values[copy][control_index];
}

gboolean
settings_get_lock          (const settings_t * settings, unsigned long control_index)
{
  g_return_val_if_fail (settings != NULL, FALSE);
  
  return settings->locks[control_index]; 
}

gboolean
settings_get_lock_all      (const settings_t * settings)
{
  g_return_val_if_fail (settings != NULL, FALSE);

  return settings->lock_all;
}

gboolean
settings_get_enabled      (const settings_t * settings)
{
  g_return_val_if_fail (settings != NULL, FALSE);
  
  return settings->enabled;
}

guint
settings_get_copies        (const settings_t * settings)
{
  g_return_val_if_fail (settings != NULL, 0);
  
  return settings->copies;
}


unsigned long
settings_get_channels        (const settings_t * settings)
{
  g_return_val_if_fail (settings != NULL, 0);
  
  return settings->channels;
}

gboolean
settings_get_wet_dry_enabled (const settings_t * settings)
{
  g_return_val_if_fail (settings != NULL, FALSE);

  return settings->wet_dry_enabled;
}

gboolean
settings_get_wet_dry_locked  (const settings_t * settings)
{
  g_return_val_if_fail (settings != NULL, FALSE);
  
  return settings->wet_dry_locked;
}

LADSPA_Data
settings_get_wet_dry_value   (settings_t * settings, unsigned long channel)
{
  g_return_val_if_fail (settings != NULL, NAN);

  if (channel >= settings->channels)
    settings_set_channels (settings, channel + 1);
  
  return settings->wet_dry_values[channel];
}



/* EOF */
