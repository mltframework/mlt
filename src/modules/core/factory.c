/*
 * factory.c -- the factory method interfaces
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

#include <string.h>

#include "filter_deinterlace.h"
#include "filter_greyscale.h"
#include "filter_resize.h"
#include "filter_gamma.h"
#include "producer_ppm.h"
#include "transition_composite.h"

void *mlt_create_producer( char *id, void *arg )
{
	if ( !strcmp( id, "ppm" ) )
		return producer_ppm_init( arg );
	return NULL;
}

void *mlt_create_filter( char *id, void *arg )
{
	if ( !strcmp( id, "deinterlace" ) )
		return filter_deinterlace_init( arg );
	if ( !strcmp( id, "gamma" ) )
		return filter_gamma_init( arg );
	if ( !strcmp( id, "greyscale" ) )
		return filter_greyscale_init( arg );
	if ( !strcmp( id, "resize" ) )
		return filter_resize_init( arg );
	return NULL;
}

void *mlt_create_transition( char *id, void *arg )
{
	if ( !strcmp( id, "composite" ) )
		return transition_composite_init( arg );
	return NULL;
}

void *mlt_create_consumer( char *id, void *arg )
{
	return NULL;
}

