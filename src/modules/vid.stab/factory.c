/*
 * Copyright (C) 2013 Dan Dennedy <dan@dennedy.org>
 * factory.c -- the factory method interfaces
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
#include <limits.h>
#include <framework/mlt.h>

// extern mlt_filter filter_vidstab_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_deshake_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_detect_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_transform_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/vid.stab/filter_%s.yml", mlt_environment( "MLT_DATA" ), id );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	MLT_REGISTER( filter_type, "vid.stab.deshake", filter_deshake_init );
	MLT_REGISTER( filter_type, "vid.stab.detect", filter_detect_init );
	MLT_REGISTER( filter_type, "vid.stab.transform", filter_transform_init );
	
	MLT_REGISTER_METADATA( filter_type, "vid.stab.deshake", metadata, "filter_deshake.yml" );
	MLT_REGISTER_METADATA( filter_type, "vid.stab.detect", metadata, "filter_detect.yml" );
	MLT_REGISTER_METADATA( filter_type, "vid.stab.transform", metadata, "filter_transform.yml" );
}
