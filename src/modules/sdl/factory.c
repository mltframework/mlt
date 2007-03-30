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

#include "consumer_sdl.h"

#ifdef WITH_SDL_IMAGE
#include "producer_sdl_image.h"
#endif

void *mlt_create_producer( char *id, void *arg )
{
#ifdef WITH_SDL_IMAGE
	if ( !strcmp( id, "sdl_image" ) )
		return producer_sdl_image_init( arg );
#endif
	return NULL;
}

void *mlt_create_filter( char *id, void *arg )
{
	return NULL;
}

void *mlt_create_transition( char *id, void *arg )
{
	return NULL;
}

void *mlt_create_consumer( char *id, void *arg )
{
	if ( !strcmp( id, "sdl" ) )
		return consumer_sdl_init( arg );
	if ( !strcmp( id, "sdl_still" ) )
		return consumer_sdl_still_init( arg );
	if ( !strcmp( id, "sdl_preview" ) )
		return consumer_sdl_preview_init( arg );
	return NULL;
}

