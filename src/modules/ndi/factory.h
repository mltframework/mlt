/*
 * factory.h -- output through NewTek NDI
 * Copyright (C) 2016 Maksym Veremeyenko <verem@m1stereo.tv>
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
 * License along with consumer library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef FACTORY_H
#define FACTORY_H

#include <framework/mlt.h>

#define NDI_CON_STR_MAX 32768

mlt_producer producer_ndi_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

mlt_consumer consumer_ndi_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

int swab_sliced( int id, int idx, int jobs, void* cookie );

void swab2( const void *from, void *to, int n );

#endif /* FACTORY_H */
