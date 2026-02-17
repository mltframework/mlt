/*
 * MLT OpenFX
 *
 * Copyright (C) 2025 Meltytech, LLC
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

#include "ofxCore.h"
#include <dlfcn.h>
#include <framework/mlt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef OfxStatus (*OfxSetHostFn)(const OfxHost *host);
typedef OfxPlugin *(*OfxGetPluginFn)(int nth);
typedef int (*OfxGetNumberOfPluginsFn)(void);

typedef enum {
    mltofx_prop_none = 0,
    mltofx_prop_int = 1,
    mltofx_prop_string = 2,
    mltofx_prop_double = 8,
    mltofx_prop_pointer = 16,
    mltofx_prop_color = 32,
    mltofx_prop_double2d = 64,
} mltofx_property_type;

typedef enum {
    mltofx_depth_none = 0,
    mltofx_depth_byte = 1,
    mltofx_depth_short = 2,
} mltofx_depths_mask;

typedef enum {
    mltofx_components_none = 0,
    mltofx_components_rgb = 1,
    mltofx_components_rgba = 2,
} mltofx_components_mask;

void mltofx_init_host_properties(OfxPropertySetHandle host_properties);

void mltofx_create_instance(OfxPlugin *plugin, mlt_properties image_effect);

void mltofx_destroy_instance(OfxPlugin *plugin, mlt_properties image_effect);

void mltofx_set_source_clip_data(OfxPlugin *plugin,
                                 mlt_properties image_effect,
                                 uint8_t *image,
                                 int width,
                                 int height,
                                 mlt_image_format format);

void mltofx_set_output_clip_data(OfxPlugin *plugin,
                                 mlt_properties image_effect,
                                 uint8_t *image,
                                 int width,
                                 int height,
                                 mlt_image_format format);

int mltofx_detect_plugin(OfxPlugin *plugin);

void *mltofx_fetch_params(OfxPlugin *plugin, mlt_properties params, mlt_properties mlt_metadata);

void mltofx_param_set_value(mlt_properties params, char *key, mltofx_property_type type, ...);

void mltofx_get_clip_preferences(OfxPlugin *plugin, mlt_properties image_effect);

void mltofx_get_region_of_definition(OfxPlugin *plugin, mlt_properties image_effect);

void mltofx_get_regions_of_interest(OfxPlugin *plugin,
                                    mlt_properties image_effect,
                                    double width,
                                    double height);

void mltofx_begin_sequence_render(OfxPlugin *plugin, mlt_properties image_effect);

void mltofx_end_sequence_render(OfxPlugin *plugin, mlt_properties image_effect);

void mltofx_action_render(OfxPlugin *plugin, mlt_properties image_effect, int width, int height);

mltofx_depths_mask mltofx_plugin_supported_depths(mlt_properties image_effect);

mltofx_components_mask mltofx_plugin_supported_components(mlt_properties image_effect);

#endif
