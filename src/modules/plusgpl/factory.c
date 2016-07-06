/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2008-2014 Dan Dennedy <dan@dennedy.org>
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <framework/mlt.h>
#include <limits.h>

extern mlt_consumer consumer_cbrts_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_burn_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_lumaliftgaingamma_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_rotoscoping_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_telecide_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/plusgpl/%s", mlt_environment( "MLT_DATA" ), (char*) data );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	MLT_REGISTER( consumer_type, "cbrts", consumer_cbrts_init );
	MLT_REGISTER( filter_type, "BurningTV", filter_burn_init );
	MLT_REGISTER( filter_type, "burningtv", filter_burn_init );
	MLT_REGISTER( filter_type, "lumaliftgaingamma", filter_lumaliftgaingamma_init );
	MLT_REGISTER( filter_type, "rotoscoping", filter_rotoscoping_init );
	MLT_REGISTER( filter_type, "telecide", filter_telecide_init );

	MLT_REGISTER_METADATA( consumer_type, "cbrts", metadata, "consumer_cbrts.yml" );
	MLT_REGISTER_METADATA( filter_type, "BurningTV", metadata, "filter_burningtv.yml" );
	MLT_REGISTER_METADATA( filter_type, "burningtv", metadata, "filter_burningtv.yml" );
	MLT_REGISTER_METADATA( filter_type, "lumaliftgaingamma", metadata, "filter_lumaliftgaingamma.yml" );
	MLT_REGISTER_METADATA( filter_type, "rotoscoping", metadata, "filter_rotoscoping.yml" );
}
