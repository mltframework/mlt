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

#include <framework/mlt.h>

#include <string.h>
#include <limits.h>
#ifdef SOX14
#include <sox.h>
#endif

extern mlt_filter filter_sox_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	mlt_properties result = NULL;

	// Load the yaml file
	snprintf( file, PATH_MAX, "%s/sox/filter_%s.yml", mlt_environment( "MLT_DATA" ), "sox" );
	result = mlt_properties_parse_yaml( file );

#ifdef SOX14
	if ( result && ( type == filter_type ) )
	{
		// Annotate the yaml properties with sox effect usage.
		mlt_properties params = mlt_properties_get_data( result, "parameters", NULL );
		const sox_effect_handler_t *e;
		int i;

		for ( i = 0; sox_effect_fns[i]; i++ )
		{
			e = sox_effect_fns[i]();
			if ( e && e->name && !strcmp( e->name, id + 4 ) )
			{
				mlt_properties p = mlt_properties_get_data( params, "0", NULL );

				mlt_properties_set( result, "identifier", e->name );
				mlt_properties_set( result, "title", e->name );
				mlt_properties_set( p, "type", "string" );
				mlt_properties_set( p, "title", "Options" );
				if ( e->usage )
					mlt_properties_set( p, "format", e->usage );
				break;
			}
		}
	}
#endif
	return result;
}

MLT_REPOSITORY
{
	MLT_REGISTER( filter_type, "sox", filter_sox_init );
	MLT_REGISTER_METADATA( filter_type, "sox", metadata, NULL );
#ifdef SOX14
	int i;
	const sox_effect_handler_t *e;
	char name[64] = "sox.";
	for ( i = 0; sox_effect_fns[i]; i++ )
	{
		e = sox_effect_fns[i]();
		if ( e && e->name && !( e->flags & SOX_EFF_DEPRECATED )
#if (SOX_LIB_VERSION_CODE >= SOX_LIB_VERSION(14,3,0))
			&& !( e->flags & SOX_EFF_INTERNAL )
#endif
			)
		{
			strcpy( name + 4, e->name );
			MLT_REGISTER( filter_type, name, filter_sox_init );
			MLT_REGISTER_METADATA( filter_type, name, metadata, NULL );
		}
	}
#endif
}
