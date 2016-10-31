/*
 * producer_ndi.c -- output through NewTek NDI
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

#define __STDC_FORMAT_MACROS  /* see inttypes.h */
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#define _XOPEN_SOURCE

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>
#include <pthread.h>

#include "factory.h"

#include "Processing.NDI.Lib.h"


/** Initialise the producer.
 */
mlt_consumer producer_ndi_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	return NULL;
}
