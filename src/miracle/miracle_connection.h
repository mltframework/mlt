/*
 * miracle_connection.h -- DV Connection Handler
 * Copyright (C) 2002-2003 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _DV_CONNECTION_H_
#define _DV_CONNECTION_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <valerie/valerie_parser.h>
#include <valerie/valerie_tokeniser.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Connection structure
*/

typedef struct 
{
	int fd;
	struct sockaddr_in sin;
	valerie_parser parser;
} 
connection_t;

/** Enumeration for responses.
*/

typedef enum 
{
	RESPONSE_SUCCESS = 200,
	RESPONSE_SUCCESS_N = 201,
	RESPONSE_SUCCESS_1 = 202,
	RESPONSE_UNKNOWN_COMMAND = 400,
	RESPONSE_TIMEOUT = 401,
	RESPONSE_MISSING_ARG = 402,
	RESPONSE_INVALID_UNIT = 403,
	RESPONSE_BAD_FILE = 404,
	RESPONSE_OUT_OF_RANGE = 405,
	RESPONSE_TOO_MANY_FILES = 406,
	RESPONSE_ERROR = 500
} 
response_codes;

/* the following struct is passed as the single argument 
   to all command callback functions */

typedef struct 
{
	valerie_parser    parser;
	valerie_response  response;
	valerie_tokeniser tokeniser;
	char         *command;
	int           unit;
	void         *argument;
	char         *root_dir;
} 
command_argument_t, *command_argument;

/* A handler is defined as follows. */
typedef int (*command_handler_t) ( command_argument );


extern void *parser_thread( void *arg );

#ifdef __cplusplus
}
#endif

#endif
