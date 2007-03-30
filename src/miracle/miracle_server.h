/*
 * miracle_server.h
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

#ifndef _MIRACLE_SERVER_H_
#define _MIRACLE_SERVER_H_

/* System header files */
#include <pthread.h>

/* Application header files */
#include <valerie/valerie_parser.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Servers default port
*/

#define DEFAULT_TCP_PORT 5250

/** Structure for the server
*/

typedef struct
{
	struct mlt_properties_s parent;
	char *id;
	int port;
	int socket;
	valerie_parser parser;
	pthread_t thread;
	int shutdown;
	int proxy;
	char remote_server[ 50 ];
	int remote_port;
	char *config;
}
*miracle_server, miracle_server_t;

/** API for the server
*/

extern miracle_server miracle_server_init( char * );
extern const char *miracle_server_id( miracle_server );
extern void miracle_server_set_config( miracle_server, char * );
extern void miracle_server_set_port( miracle_server, int );
extern void miracle_server_set_proxy( miracle_server, char * );
extern int miracle_server_execute( miracle_server );
extern mlt_properties miracle_server_fetch_unit( miracle_server, int );
extern void miracle_server_shutdown( miracle_server );
extern void miracle_server_close( miracle_server );

#ifdef __cplusplus
}
#endif

#endif
