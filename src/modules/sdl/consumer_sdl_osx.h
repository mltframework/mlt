/*
 * consumer_sdl_osx.m -- An OS X compatibility shim for SDL
 * Copyright (C) 2010-2014 Meltytech, LLC
 * Author: Dan Dennedy
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

#ifndef _CONSUMER_SDL_OSX_H_
#define _CONSUMER_SDL_OSX_H_

#ifdef __APPLE__
void* mlt_cocoa_autorelease_init();
void  mlt_cocoa_autorelease_close( void* );

#else
static inline void *mlt_cocoa_autorelease_init()
{
	return NULL;
}

static inline void mlt_cocoa_autorelease_close(void* p)
{
}
#endif

#endif
