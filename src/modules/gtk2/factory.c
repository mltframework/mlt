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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "producer_pixbuf.h"
#include "producer_pango.h"
#include "filter_rescale.h"

void *mlt_create_producer( char *id, void *arg )
{
	g_type_init( );
	if ( !strcmp( id, "pixbuf" ) )
		return producer_pixbuf_init( arg );
	else if ( !strcmp( id, "pango" ) )
		return producer_pango_init( arg );
	return NULL;
}

void *mlt_create_filter( char *id, void *arg )
{
	g_type_init( );
	if ( !strcmp( id, "gtkrescale" ) )
		return filter_rescale_init( arg );
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

