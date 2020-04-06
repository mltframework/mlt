/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#include <string.h>
#include <framework/mlt.h>
#include <gdk/gdk.h>
#include <stdlib.h>

#ifdef USE_GTK2
extern mlt_consumer consumer_gtk2_preview_init( mlt_profile profile, void *widget );
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

void *create_service( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	initialise( );

#ifdef USE_GTK2
	if ( !strcmp( id, "gtk2_preview" ) )
		return consumer_gtk2_preview_init( profile, arg );
#endif

	return NULL;
}

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/gtk2/%s", mlt_environment( "MLT_DATA" ), (char*) data );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	MLT_REGISTER( consumer_type, "gtk2_preview", create_service );

	MLT_REGISTER_METADATA( consumer_type, "gtk2_preview", metadata, "consumer_gtk2_preview.yml" );
}
