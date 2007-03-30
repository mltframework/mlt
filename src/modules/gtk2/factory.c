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

#include "config.h"
#include <string.h>

#ifdef USE_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "producer_pixbuf.h"
#include "filter_rescale.h"
#endif

#ifdef USE_GTK2
#include "consumer_gtk2.h"
#endif

#ifdef USE_PANGO
#include "producer_pango.h"
#endif

static void initialise( )
{
	static int init = 0;
	if ( init == 0 )
	{
		init = 1;
		g_type_init( );
	}
}

void *mlt_create_producer( char *id, void *arg )
{
	initialise( );

#ifdef USE_PIXBUF
	if ( !strcmp( id, "pixbuf" ) )
		return producer_pixbuf_init( arg );
#endif

#ifdef USE_PANGO
	if ( !strcmp( id, "pango" ) )
		return producer_pango_init( arg );
#endif

	return NULL;
}

void *mlt_create_filter( char *id, void *arg )
{
	initialise( );

#ifdef USE_PIXBUF
	if ( !strcmp( id, "gtkrescale" ) )
		return filter_rescale_init( arg );
#endif

	return NULL;
}

void *mlt_create_transition( char *id, void *arg )
{
	return NULL;
}

void *mlt_create_consumer( char *id, void *arg )
{
	initialise( );

#ifdef USE_GTK2
	if ( !strcmp( id, "gtk2_preview" ) )
		return consumer_gtk2_preview_init( arg );
#endif

	return NULL;
}

