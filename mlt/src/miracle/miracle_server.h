/*
 * dvserver.h -- DV Server
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

#ifndef _DV_SERVER_H_
#define _DV_SERVER_H_

/* System header files */
#include <pthread.h>

/* Application header files */
#include <dvparser.h>

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
	char *id;
	int port;
	int socket;
	dv_parser parser;
	pthread_t thread;
	int shutdown;
	int proxy;
	char remote_server[ 50 ];
	int remote_port;
}
*dv_server, dv_server_t;

/** API for the server
*/

extern dv_server dv_server_init( char * );
extern void dv_server_set_port( dv_server, int );
extern void dv_server_set_proxy( dv_server, char * );
extern int dv_server_execute( dv_server );
extern void dv_server_shutdown( dv_server );

#ifdef __cplusplus
}
#endif

#endif
