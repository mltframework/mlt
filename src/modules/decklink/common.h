/*
 * common.h -- Blackmagic Design DeckLink common functions
 * Copyright (C) 2012 Dan Dennedy <dan@dennedy.org>
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
 * License along with consumer library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DECKLINK_COMMON_H
#define DECKLINK_COMMON_H

#ifdef _WIN32
#	include <objbase.h>
#	include "DeckLinkAPI_h.h"
	typedef BSTR DLString;
#else
#	include "DeckLinkAPI.h"
#	ifdef __APPLE__
		typedef CFStringRef DLString;
#	else
		typedef const char* DLString;
#	endif
#endif

#define SAFE_RELEASE(V) if (V) { V->Release(); V = NULL; }

char* getCString( DLString aDLString );
void freeCString( char* aCString );
void freeDLString( DLString aDLString );
void swab2( const void *from, void *to, int n );

#endif // DECKLINK_COMMON_H
