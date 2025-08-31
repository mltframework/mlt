/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include "mltvorbis_export.h"
#include <framework/mlt.h>
#include <limits.h>
#include <string.h>
extern mlt_producer producer_vorbis_init(mlt_profile profile,
                                         mlt_service_type type,
                                         const char *id,
                                         char *arg);

static mlt_properties metadata(mlt_service_type type, const char *id, void *data)
{
    char file[PATH_MAX];
    snprintf(file, PATH_MAX, "%s/vorbis/%s", mlt_environment("MLT_DATA"), (char *) data);
    return mlt_properties_parse_yaml(file);
}

MLTVORBIS_EXPORT MLT_REPOSITORY
{
    MLT_REGISTER(mlt_service_producer_type, "vorbis", producer_vorbis_init);
    MLT_REGISTER_METADATA(mlt_service_producer_type, "vorbis", metadata, "producer_vorbis.yml");
}
