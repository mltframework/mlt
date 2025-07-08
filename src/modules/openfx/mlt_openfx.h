/*
 * MLT OpenFX
 *
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

#ifndef MLT_OPENFX_H
#define MLT_OPENFX_H

#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dlfcn.h>
#include "ofxCore.h"

typedef enum {
  mltofx_prop_none    = 0,
  mltofx_prop_int     = 1,
  mltofx_prop_string  = 2,
  mltofx_prop_double  = 8,
  mltofx_prop_pointer = 16,
} mltofx_property_type;

void *
mltofx_fetch_params (OfxPlugin      *plugin,
		     mlt_properties  params);

void
mltofx_create_instance (OfxPlugin *plugin, mlt_properties image_effect);

void
mltofx_begin_sequence_render (OfxPlugin *plugin, mlt_properties image_effect);

void
mltofx_end_sequence_render (OfxPlugin *plugin, mlt_properties image_effect);

void
mltofx_get_regions_of_interest (OfxPlugin *plugin, mlt_properties image_effect, double width, double height);

void
mltofx_get_clip_preferences (OfxPlugin *plugin, mlt_properties image_effect);

void
mltofx_set_source_clip_data (OfxPlugin *plugin, mlt_properties image_effect, uint8_t *image, int width, int height);

void
mltofx_set_output_clip_data (OfxPlugin *plugin, mlt_properties image_effect, uint8_t *image, int width, int height);

void
mltofx_action_render (OfxPlugin *plugin, mlt_properties image_effect, int width, int height);

void
mltofx_destroy_instance (OfxPlugin *plugin, mlt_properties image_effect);

void
mltofx_init_host_properties (OfxPropertySetHandle host_properties);

void
mltofx_param_set_value (mlt_properties params, char *key, mltofx_property_type type, ...);

#endif
