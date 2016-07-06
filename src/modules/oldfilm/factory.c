/*
 * factory.c -- the factory method interfaces
 * Copyright (c) 2007 Marco Gittler <g.marco@freenet.de>
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
#include <limits.h>

extern mlt_filter filter_dust_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_grain_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_lines_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_oldfilm_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_tcolor_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_vignette_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

static mlt_properties oldfilm_metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/oldfilm/filter_%s.yml", mlt_environment( "MLT_DATA" ), id );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	MLT_REGISTER( filter_type, "oldfilm", filter_oldfilm_init );
	MLT_REGISTER( filter_type, "dust", filter_dust_init );
	MLT_REGISTER( filter_type, "lines", filter_lines_init );
	MLT_REGISTER( filter_type, "grain", filter_grain_init );
	MLT_REGISTER( filter_type, "tcolor", filter_tcolor_init );
	MLT_REGISTER( filter_type, "vignette", filter_vignette_init );
	
	MLT_REGISTER_METADATA( filter_type, "vignette", oldfilm_metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "tcolor", oldfilm_metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "grain", oldfilm_metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "lines", oldfilm_metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "dust", oldfilm_metadata, NULL );
	MLT_REGISTER_METADATA( filter_type, "oldfilm", oldfilm_metadata, NULL );
	
}



