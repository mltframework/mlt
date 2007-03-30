/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2005 Visual Media Fx Inc.
 * Author: Charles Yates <charles.yates@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>

#include "filter_chroma.h"
#include "filter_chroma_hold.h"
#include "filter_mono.h"
#include "filter_shape.h"
#include "producer_pgm.h"

void *mlt_create_producer( char *id, void *arg )
{
	if ( !strcmp( id, "pgm" ) )
		return producer_pgm_init( arg );
	return NULL;
}

void *mlt_create_filter( char *id, void *arg )
{
	if ( !strcmp( id, "chroma" ) )
		return filter_chroma_init( arg );
	if ( !strcmp( id, "chroma_hold" ) )
		return filter_chroma_hold_init( arg );
	if ( !strcmp( id, "threshold" ) )
		return filter_mono_init( arg );
	if ( !strcmp( id, "shape" ) )
		return filter_shape_init( arg );
	return NULL;
}

void *mlt_create_transition( char *id, void *arg )
{
	return NULL;
}

void *mlt_create_consumer( char *id, void *arg )
{
	return NULL;
}
