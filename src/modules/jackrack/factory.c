/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <framework/mlt.h>
#include <ladspa.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>

#include "plugin_mgr.h"

extern mlt_filter filter_jackrack_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_ladspa_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

plugin_mgr_t *g_jackrack_plugin_mgr = NULL;

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/jackrack/filter_%s.yml",
		mlt_environment( "MLT_DATA" ), strncmp( id, "ladspa.", 7 ) ? id : "ladspa" );
	mlt_properties result = mlt_properties_parse_yaml( file );

	if ( !strncmp( id, "ladspa.", 7 ) )
	{
		// Annotate the yaml properties with ladspa control port info.
		plugin_desc_t *desc = plugin_mgr_get_any_desc( g_jackrack_plugin_mgr, strtol( id + 7, NULL, 10 ) );

		if ( desc )
		{
			mlt_properties params = mlt_properties_new();
			mlt_properties p;
			char key[20];
			int i;

			mlt_properties_set( result, "identifier", id );
			mlt_properties_set( result, "title", desc->name );
			mlt_properties_set_data( result, "parameters", params, 0, (mlt_destructor) mlt_properties_close, NULL );
			for ( i = 0; i < desc->control_port_count; i++ )
			{
				int j = desc->control_port_indicies[i];
				LADSPA_Data sample_rate = 48000;
				LADSPA_PortRangeHintDescriptor hint_descriptor = desc->port_range_hints[j].HintDescriptor;

				p = mlt_properties_new();
				snprintf( key, sizeof(key), "%d", i );
				mlt_properties_set_data( params, key, p, 0, (mlt_destructor) mlt_properties_close, NULL );
				snprintf( key, sizeof(key), "%d", j );
				mlt_properties_set( p, "identifier", key );
				mlt_properties_set( p, "title", desc->port_names[ j ] );
				if ( LADSPA_IS_HINT_INTEGER( hint_descriptor ) )
				{
					mlt_properties_set( p, "type", "integer" );
					mlt_properties_set_int( p, "default", plugin_desc_get_default_control_value( desc, j, sample_rate ) );
				}
				else if ( LADSPA_IS_HINT_TOGGLED( hint_descriptor ) )
				{
					mlt_properties_set( p, "type", "boolean" );
					mlt_properties_set_int( p, "default", plugin_desc_get_default_control_value( desc, j, sample_rate ) );
				}
				else
				{
					mlt_properties_set( p, "type", "float" );
					mlt_properties_set_double( p, "default", plugin_desc_get_default_control_value( desc, j, sample_rate ) );
				}
				/* set upper and lower, possibly adjusted to the sample rate */
				if ( LADSPA_IS_HINT_BOUNDED_BELOW( hint_descriptor ) )
				{
					LADSPA_Data lower = desc->port_range_hints[j].LowerBound;
					if ( LADSPA_IS_HINT_SAMPLE_RATE( hint_descriptor ) )
						lower *= sample_rate;
					if ( LADSPA_IS_HINT_LOGARITHMIC( hint_descriptor ) )
					{
						if (lower < FLT_EPSILON)
							lower = FLT_EPSILON;
					}
					mlt_properties_set_double( p, "minimum", lower );
				}
				if ( LADSPA_IS_HINT_BOUNDED_ABOVE( hint_descriptor ) )
				{
					LADSPA_Data upper = desc->port_range_hints[j].UpperBound;
					if ( LADSPA_IS_HINT_SAMPLE_RATE( hint_descriptor ) )
						upper *= sample_rate;
					mlt_properties_set_double( p, "maximum", upper );
				}
				if ( LADSPA_IS_HINT_LOGARITHMIC( hint_descriptor ) )
					mlt_properties_set( p, "scale", "log" );
			}
			p = mlt_properties_new();
			snprintf( key, sizeof(key), "%d", i );
			mlt_properties_set_data( params, key, p, 0, (mlt_destructor) mlt_properties_close, NULL );
			mlt_properties_set( p, "identifier", "wetness" );
			mlt_properties_set( p, "title", "Wet/Dry" );
			mlt_properties_set( p, "type", "float" );
			mlt_properties_set_double( p, "default", 1 );
			mlt_properties_set_double( p, "minimum", 0 );
			mlt_properties_set_double( p, "maximum", 1 );
		}
	}

	return result;
}

MLT_REPOSITORY
{
	GSList *list;
	g_jackrack_plugin_mgr = plugin_mgr_new();

	for ( list = g_jackrack_plugin_mgr->all_plugins; list; list = g_slist_next( list ) )
	{
		plugin_desc_t *desc = (plugin_desc_t *) list->data;
		char *s = malloc( strlen( "ladpsa." ) + 21 );

		sprintf( s, "ladspa.%lu", desc->id );
		MLT_REGISTER( filter_type, s, filter_ladspa_init );
		MLT_REGISTER_METADATA( filter_type, s, metadata, NULL );
	}
//	mlt_factory_register_for_clean_up( g_jackrack_plugin_mgr, (mlt_destructor) plugin_mgr_destroy );

	MLT_REGISTER( filter_type, "jackrack", filter_jackrack_init );
	MLT_REGISTER_METADATA( filter_type, "jackrack", metadata, NULL );
	MLT_REGISTER( filter_type, "ladspa", filter_ladspa_init );
	MLT_REGISTER_METADATA( filter_type, "ladspa", metadata, NULL );
}
