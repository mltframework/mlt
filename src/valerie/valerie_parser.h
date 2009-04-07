/*
 * valerie_parser.h -- Valerie Parser for Miracle Server
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

#ifndef _VALERIE_PARSER_H_
#define _VALERIE_PARSER_H_

/* MLT Header files */
#include <framework/mlt.h>

/* Application header files */
#include "valerie_response.h"
#include "valerie_notifier.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** Callbacks to define the parser.
*/

typedef valerie_response (*parser_connect)( void * );
typedef valerie_response (*parser_execute)( void *, char * );
typedef valerie_response (*parser_received)( void *, char *, char * );
typedef valerie_response (*parser_push)( void *, char *, mlt_service );
typedef void (*parser_close)( void * );

/** Structure for the valerie parser.
*/

typedef struct
{
	parser_connect connect;
	parser_execute execute;
	parser_push push;
	parser_received received;
	parser_close close;
	void *real;
	valerie_notifier notifier;
}
*valerie_parser, valerie_parser_t;

/** API for the parser - note that no constructor is defined here.
*/

extern valerie_response valerie_parser_connect( valerie_parser );
extern valerie_response valerie_parser_push( valerie_parser, char *, mlt_service );
extern valerie_response valerie_parser_received( valerie_parser, char *, char * );
extern valerie_response valerie_parser_execute( valerie_parser, char * );
extern valerie_response valerie_parser_executef( valerie_parser, const char *, ... );
extern valerie_response valerie_parser_run( valerie_parser, char * );
extern valerie_notifier valerie_parser_get_notifier( valerie_parser );
extern void valerie_parser_close( valerie_parser );

#ifdef __cplusplus
}
#endif

#endif
