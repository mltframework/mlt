/*
 * common.cpp -- Blackmagic Design DeckLink common functions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "common.h"
#include <stdlib.h>

#ifdef __DARWIN__

char* getCString( DLString aDLString )
{
	char* CString = (char*) malloc( 64 );
	CFStringGetCString( aDLString, CString, 64, kCFStringEncodingMacRoman );
	return CString;
}

void freeCString( char* aCString )
{
	if ( aCString ) free( aCString );
}

void freeDLString( DLString aDLString )
{
	if ( aDLString ) CFRelease( aDLString );
}

#else

char* getCString( DLString aDLString )
{
	return aDLString? (char*) aDLString : NULL;
}

void freeCString( char* aCString )
{
}

void freeDLString( DLString aDLString )
{
	if ( aDLString ) free( (void*) aDLString );
}

#endif

