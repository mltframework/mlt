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
#include <stdio.h>
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

/* Adapted from g_win32_getlocale() - free() the result */
char* getlocale()
{
	LCID lcid;
	LANGID langid;
	char *ev;
	int primary, sub;
	char iso639[10];
	char iso3166[10];
	const char *script = "";
	char result[33];

	/* Let the user override the system settings through environment
	 * variables, as on POSIX systems.
	 */
	if (((ev = getenv ("LC_ALL")) != NULL && ev[0] != '\0')
		|| ((ev = getenv ("LC_MESSAGES")) != NULL && ev[0] != '\0')
		|| ((ev = getenv ("LANG")) != NULL && ev[0] != '\0'))
	  return strdup (ev);

	lcid = GetThreadLocale ();

	if (!GetLocaleInfo (lcid, LOCALE_SISO639LANGNAME, iso639, sizeof (iso639)) ||
		!GetLocaleInfo (lcid, LOCALE_SISO3166CTRYNAME, iso3166, sizeof (iso3166)))
	  return strdup ("C");

	/* Strip off the sorting rules, keep only the language part.  */
	langid = LANGIDFROMLCID (lcid);

	/* Split into language and territory part.  */
	primary = PRIMARYLANGID (langid);
	sub = SUBLANGID (langid);

	/* Handle special cases */
	switch (primary)
	  {
	  case LANG_AZERI:
		switch (sub)
		 {
		 case SUBLANG_AZERI_LATIN:
		   script = "@Latn";
		   break;
		 case SUBLANG_AZERI_CYRILLIC:
		   script = "@Cyrl";
		   break;
		 }
		break;
	  case LANG_SERBIAN:             /* LANG_CROATIAN == LANG_SERBIAN */
		switch (sub)
		 {
		 case SUBLANG_SERBIAN_LATIN:
		 case 0x06: /* Serbian (Latin) - Bosnia and Herzegovina */
		   script = "@Latn";
		   break;
		 }
		break;
	  case LANG_UZBEK:
		switch (sub)
		 {
		 case SUBLANG_UZBEK_LATIN:
		   script = "@Latn";
		   break;
		 case SUBLANG_UZBEK_CYRILLIC:
		   script = "@Cyrl";
		   break;
		 }
		break;
	  }
	snprintf (result, sizeof(result), "%s_%s%s", iso639, iso3166, script);
	result[sizeof(result) - 1] = '\0';
	return strdup (result);
}

FILE* win32_fopen(const char *filename_utf8, const char *mode_utf8)
{
	// Convert UTF-8 to wide chars.
	int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename_utf8, -1, NULL, 0);
	if (n > 0) {
		wchar_t *filename_w = (wchar_t *) calloc(n, sizeof(wchar_t));
		if (filename_w) {
			int m = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, mode_utf8, -1, NULL, 0);
			if (m > 0) {
				wchar_t *mode_w = (wchar_t *) calloc(m, sizeof(wchar_t));
				if (mode_w) {
					MultiByteToWideChar(CP_UTF8, 0, filename_utf8, -1, filename_w, n);
					MultiByteToWideChar(CP_UTF8, 0, mode_utf8, -1, mode_w, n);
					FILE *fh = _wfopen(filename_w, mode_w);
					free(filename_w);
					if (fh)
						return fh;
				}
			}
		}
	}
	// Try with regular old fopen.
	return fopen(filename_utf8, mode_utf8);
}
