/*
 * valerie_tokeniser.h -- String tokeniser
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

#ifndef _VALERIE_TOKENISER_H_
#define _VALERIE_TOKENISER_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** Structure for tokeniser.
*/

typedef struct
{
	char *input;
	char **tokens;
	int count;
	int size;
}
*valerie_tokeniser, valerie_tokeniser_t;

/** Remote parser API.
*/

extern valerie_tokeniser valerie_tokeniser_init( );
extern int valerie_tokeniser_parse_new( valerie_tokeniser, char *, char * );
extern char *valerie_tokeniser_get_input( valerie_tokeniser );
extern int valerie_tokeniser_count( valerie_tokeniser );
extern char *valerie_tokeniser_get_string( valerie_tokeniser, int );
extern void valerie_tokeniser_close( valerie_tokeniser );

#ifdef __cplusplus
}
#endif

#endif
