/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2007 Jean-Baptiste Mardelle
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

#include "filter_oldfilm.h"
#include "filter_grain.h"
#include "filter_dust.h"
#include "filter_lines.h"

void *mlt_create_producer( char *id, void *arg )
{
	return NULL;
}

void *mlt_create_filter( char *id, void *arg )
{
	if ( !strcmp( id, "oldfilm" ) )
                return filter_oldfilm_init( arg );
        if ( !strcmp( id, "dust" ) )
                return filter_dust_init( arg );
        if ( !strcmp( id, "lines" ) )
                return filter_lines_init( arg );
        if ( !strcmp( id, "grain" ) )
                return filter_grain_init( arg );

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
