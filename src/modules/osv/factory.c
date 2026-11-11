/*
 * factory.c -- the factory method interfaces for DJI Osmo 360 producer
 * Copyright (C) 2026 Meltytech, LLC
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
#include "mltosv_export.h"

extern mlt_producer producer_osv_init(mlt_profile profile,
                                      mlt_service_type type,
                                      const char *id,
                                      char *arg);

MLTOSV_EXPORT MLT_REPOSITORY
{
    MLT_REGISTER(mlt_service_producer_type, "osv", producer_osv_init);
}
