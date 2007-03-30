/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>

#include "filter_affine.h"
#include "filter_charcoal.h"
#include "filter_invert.h"
#include "filter_sepia.h"
#include "transition_affine.h"

void *mlt_create_producer( char *id, void *arg )
{
	return NULL;
}

void *mlt_create_filter( char *id, void *arg )
{
	if ( !strcmp( id, "affine" ) )
		return filter_affine_init( arg );
	if ( !strcmp( id, "charcoal" ) )
		return filter_charcoal_init( arg );
	if ( !strcmp( id, "invert" ) )
		return filter_invert_init( arg );
	if ( !strcmp( id, "sepia" ) )
		return filter_sepia_init( arg );
	return NULL;
}

void *mlt_create_transition( char *id, void *arg )
{
	if ( !strcmp( id, "affine" ) )
		return transition_affine_init( arg );
	return NULL;
}

void *mlt_create_consumer( char *id, void *arg )
{
	return NULL;
}
