/*
 * mlt_field.h -- A field for planting multiple transitions and services
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MLT_FIELD_H_
#define _MLT_FIELD_H_

#include "mlt_types.h"

extern mlt_field mlt_field_init( mlt_service producer );
extern mlt_service mlt_field_service( mlt_field this );
extern mlt_properties mlt_field_properties( mlt_field this );
extern int mlt_field_plant_filter( mlt_field this, mlt_filter that, int track );
extern int mlt_field_plant_transition( mlt_field this, mlt_transition that, int a_track, int b_track );
extern void mlt_field_close( mlt_field this );

#endif

