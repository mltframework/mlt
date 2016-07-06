/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2005 Visual Media Fx Inc.
 * Author: Charles Yates <charles.yates@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <limits.h>
#include <framework/mlt.h>

extern mlt_filter filter_chroma_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_chroma_hold_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_mono_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_shape_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_pgm_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/vmfx/%s", mlt_environment( "MLT_DATA" ), (char*) data );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	MLT_REGISTER( filter_type, "chroma", filter_chroma_init );
	MLT_REGISTER( filter_type, "chroma_hold", filter_chroma_hold_init );
	MLT_REGISTER( filter_type, "threshold", filter_mono_init );
	MLT_REGISTER( filter_type, "shape", filter_shape_init );
	MLT_REGISTER( producer_type, "pgm", producer_pgm_init );

	MLT_REGISTER_METADATA( filter_type, "chroma", metadata, "filter_chroma.yml" );
	MLT_REGISTER_METADATA( filter_type, "chroma_hold", metadata, "filter_chroma_hold.yml" );
	MLT_REGISTER_METADATA( filter_type, "threshold", metadata, "filter_mono.yml" );
	MLT_REGISTER_METADATA( filter_type, "shape", metadata, "filter_shape.yml" );
	MLT_REGISTER_METADATA( producer_type, "pgm", metadata, "producer_pgm.yml" );
}
