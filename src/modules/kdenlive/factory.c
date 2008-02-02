/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2007 Jean-Baptiste Mardelle
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

extern mlt_filter filter_boxblur_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_wave_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_framebuffer_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

void *mlt_create_producer( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	if ( !strcmp( id, "framebuffer" ) )
		return producer_framebuffer_init( profile, type, id, arg );
	return NULL;
}

void *mlt_create_filter( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	if ( !strcmp( id, "boxblur" ) )
		return filter_boxblur_init( profile, type, id, arg );
	if ( !strcmp( id, "wave" ) )
		return filter_wave_init( profile, type, id, arg );
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
