/**
 * \file win32.c
 * \brief Miscellaneous utility functions for Windows.
 *
 * Copyright (C) 2003-2016 Meltytech, LLC
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

#include <errno.h> 
#include <time.h> 
#include <windows.h>
#include <pthread.h>

#include <iconv.h>
#include <locale.h>
#include <ctype.h>
#include "../framework/mlt_properties.h"
#include "../framework/mlt_log.h"

int usleep(unsigned int useconds)
{
	HANDLE timer;
	LARGE_INTEGER due;

	due.QuadPart = -(10 * (__int64)useconds);

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &due, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
	return 0;
}


int nanosleep( const struct timespec * rqtp, struct timespec * rmtp )
{
	if (rqtp->tv_nsec > 999999999) {
		/* The time interval specified 1,000,000 or more microseconds. */
		errno = EINVAL;
		return -1;
	}
	return usleep( rqtp->tv_sec * 1000000 + rqtp->tv_nsec / 1000 );
} 

int setenv(const char *name, const char *value, int overwrite)
{
	int result = 1; 
	if (overwrite == 0 && getenv (name))  {
		result = 0;
	} else  {
		result = SetEnvironmentVariable (name,value); 
	 }

	return result; 
} 

static int iconv_from_utf8( mlt_properties properties, const char *prop_name, const char *prop_name_out, const char* encoding )
{
	char *text = mlt_properties_get( properties, prop_name );
	int result = 0;

	if ( text ) {
		iconv_t cd = iconv_open( encoding, "UTF-8" );
		if ( cd != (iconv_t) -1 ) {
			size_t inbuf_n = strlen( text );
			size_t outbuf_n = inbuf_n * 6;
			char *outbuf = mlt_pool_alloc( outbuf_n );
			char *outbuf_p = outbuf;

			memset( outbuf, 0, outbuf_n );

			if ( text != NULL && strcmp( text, "" ) && iconv( cd, &text, &inbuf_n, &outbuf_p, &outbuf_n ) != -1 )
				mlt_properties_set( properties, prop_name_out, outbuf );
			else
				mlt_properties_set( properties, prop_name_out, "" );

			mlt_pool_release( outbuf );
			result = iconv_close( cd );
		} else {
			result = -1;
		}
	}
	return result;
}

static int iconv_to_utf8( mlt_properties properties, const char *prop_name, const char *prop_name_out, const char* encoding )
{
	char *text = mlt_properties_get( properties, prop_name );
	int result = 0;

	if ( text ) {
		iconv_t cd = iconv_open( "UTF-8", encoding );
		if ( cd != (iconv_t) -1 ) {
			size_t inbuf_n = strlen( text );
			size_t outbuf_n = inbuf_n * 6;
			char *outbuf = mlt_pool_alloc( outbuf_n );
			char *outbuf_p = outbuf;

			memset( outbuf, 0, outbuf_n );

			if ( text != NULL && strcmp( text, "" ) && iconv( cd, &text, &inbuf_n, &outbuf_p, &outbuf_n ) != -1 )
				mlt_properties_set( properties, prop_name_out, outbuf );
			else
				mlt_properties_set( properties, prop_name_out, "" );

			mlt_pool_release( outbuf );
			result = iconv_close( cd );
		} else {
			result = -1;
		}
	}
	return result;
}

int mlt_properties_from_utf8( mlt_properties properties, const char *prop_name, const char *prop_name_out )
{
	int result = -1;
	UINT codepage = GetACP();

	if ( codepage > 0 ) {
		// numeric code page
		char codepage_str[10];
		snprintf( codepage_str, sizeof(codepage_str), "CP%u", codepage );
		codepage_str[sizeof(codepage_str) - 1] = '\0';
		result = iconv_from_utf8( properties, prop_name, prop_name_out, codepage_str );
	}
	if ( result < 0 ) {
		result = mlt_properties_set( properties, prop_name_out,
									 mlt_properties_get( properties, prop_name ) );
        mlt_log_warning( NULL, "iconv failed to convert \"%s\" from UTF-8 to code page %u: %s\n",
            prop_name, codepage, mlt_properties_get( properties, prop_name ) );
	}
	return result;
}

int mlt_properties_to_utf8( mlt_properties properties, const char *prop_name, const char *prop_name_out )
{
	int result = -1;
	UINT codepage = GetACP();

	if ( codepage > 0 ) {
		// numeric code page
		char codepage_str[10];
		snprintf( codepage_str, sizeof(codepage_str), "CP%u", codepage );
		codepage_str[sizeof(codepage_str) - 1] = '\0';
		result = iconv_to_utf8( properties, prop_name, prop_name_out, codepage_str );
	}
	if ( result < 0 ) {
		result = mlt_properties_set( properties, prop_name_out,
									 mlt_properties_get( properties, prop_name ) );
		mlt_log_warning( NULL, "iconv failed to convert \"%s\" from code page %u to UTF-8\n", prop_name, codepage );
	}
	return result;
}
