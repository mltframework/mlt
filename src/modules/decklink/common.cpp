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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "common.h"
#include <stdlib.h>
#include <unistd.h>

#ifdef __APPLE__

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

#elif defined(_WIN32)

char* getCString( DLString aDLString )
{
	char* CString = NULL;
	if ( aDLString )
	{
		int size = WideCharToMultiByte( CP_UTF8, 0, aDLString, -1, NULL, 0, NULL, NULL );
		if (size)
		{
			CString = new char[ size ];
			size = WideCharToMultiByte( CP_UTF8, 0, aDLString, -1, CString, size, NULL, NULL );
			if ( !size )
			{
				delete[] CString;
				CString = NULL;
			}
		}
	}
	return CString;
}

void freeCString( char* aCString )
{
	delete[] aCString;
}

void freeDLString( DLString aDLString )
{
	SysFreeString( aDLString );
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


void swab2( const void *from, void *to, int n )
{
#if defined(USE_SSE)
#define SWAB_STEP 16
	__asm__ volatile
	(
		"loop_start:                            \n\t"

		/* load */
		"movdqa         0(%[from]), %%xmm0      \n\t"
		"add            $0x10, %[from]          \n\t"

		/* duplicate to temp registers */
		"movdqa         %%xmm0, %%xmm1          \n\t"

		/* shift right temp register */
		"psrlw          $8, %%xmm1              \n\t"

		/* shift left main register */
		"psllw          $8, %%xmm0              \n\t"

		/* compose them back */
		"por           %%xmm0, %%xmm1           \n\t"

		/* save */
		"movdqa         %%xmm1, 0(%[to])        \n\t"
		"add            $0x10, %[to]            \n\t"

		"dec            %[cnt]                  \n\t"
		"jnz            loop_start              \n\t"

		:
		: [from]"r"(from), [to]"r"(to), [cnt]"r"(n / SWAB_STEP)
		//: "xmm0", "xmm1"
	);

	from = (unsigned char*) from + n - (n % SWAB_STEP);
	to = (unsigned char*) to + n - (n % SWAB_STEP);
	n = (n % SWAB_STEP);
#endif
	swab((char*) from, (char*) to, n);
};
