/*
 * valerie_socket.h -- Client Socket
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _VALERIE_SOCKET_H_
#define _VALERIE_SOCKET_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** Structure for socket.
*/

typedef struct
{
	char *server;
	int port;
	int fd;
	int no_close;
}
*valerie_socket, valerie_socket_t;

/** Remote parser API.
*/

extern valerie_socket valerie_socket_init( char *, int );
extern int valerie_socket_connect( valerie_socket );
extern valerie_socket valerie_socket_init_fd( int );
extern int valerie_socket_read_data( valerie_socket, char *, int );
extern int valerie_socket_write_data( valerie_socket, const char *, int );
extern void valerie_socket_close( valerie_socket );

#ifdef __cplusplus
}
#endif

#endif
