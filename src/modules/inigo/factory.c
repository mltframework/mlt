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

#include <string.h>
#include <framework/mlt.h>

extern mlt_producer producer_inigo_file_init( mlt_profile profile, char *file );
extern mlt_producer producer_inigo_init( mlt_profile profile, char **argv );

void *mlt_create_producer( mlt_profile profile, mlt_service_type type, const char *id, void *arg )
{
	if ( !strcmp( id, "inigo_file" ) )
		return producer_inigo_file_init( profile, arg );
	if ( !strcmp( id, "inigo" ) )
		return producer_inigo_init( profile, arg );
	return NULL;
}

void *mlt_create_filter( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	return NULL;
}

void *mlt_create_transition( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	return NULL;
}

void *mlt_create_consumer( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	return NULL;
}

