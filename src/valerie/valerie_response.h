/*
 * valerie_response.h -- Response
 * Copyright (C) 2002-2003 Ushodaya Enterprises Limited
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

#ifndef _VALERIE_RESPONSE_H_
#define _VALERIE_RESPONSE_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Structure for the response
*/

typedef struct
{
	char **array;
	int size;
	int count;
	int append;
}
*valerie_response, valerie_response_t;

/** API for accessing the response structure.
*/

extern valerie_response valerie_response_init( );
extern valerie_response valerie_response_clone( valerie_response );
extern int valerie_response_get_error_code( valerie_response );
extern const char *valerie_response_get_error_string( valerie_response );
extern char *valerie_response_get_line( valerie_response, int );
extern int valerie_response_count( valerie_response );
extern void valerie_response_set_error( valerie_response, int, const char * );
extern int valerie_response_printf( valerie_response, size_t, const char *, ... );
extern int valerie_response_write( valerie_response, const char *, int );
extern void valerie_response_close( valerie_response );

#ifdef __cplusplus
}
#endif

#endif
