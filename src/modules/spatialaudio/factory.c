/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2024 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt.h>

#include "mltspatialaudio_export.h"
#include <limits.h>
#include <string.h>
extern mlt_filter filter_ambisonic_decoder_init(mlt_profile profile,
                                                mlt_service_type type,
                                                const char *id,
                                                char *arg);
extern mlt_filter filter_ambisonic_encoder_init(mlt_profile profile,
                                                mlt_service_type type,
                                                const char *id,
                                                char *arg);

static mlt_properties metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    mlt_properties result = NULL;

    // Load the yaml file
    snprintf(file, PATH_MAX, "%s/spatialaudio/filter_%s.yml", mlt_environment("MLT_DATA"), id);
    result = mlt_properties_parse_yaml(file);

    return result;
}

MLTSPATIALAUDIO_EXPORT MLT_REPOSITORY
{
    MLT_REGISTER(mlt_service_filter_type, "ambisonic-decoder", filter_ambisonic_decoder_init);
    MLT_REGISTER_METADATA(mlt_service_filter_type, "ambisonic-decoder", metadata, NULL);
    MLT_REGISTER(mlt_service_filter_type, "ambisonic-encoder", filter_ambisonic_encoder_init);
    MLT_REGISTER_METADATA(mlt_service_filter_type, "ambisonic-encoder", metadata, NULL);
}
