/*
 * valerie_status.h -- Unit Status Handling
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

#ifndef _VALERIE_STATUS_H_
#define _VALERIE_STATUS_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** Status codes
*/

typedef enum
{
	unit_unknown = 0,
	unit_undefined,
	unit_offline,
	unit_not_loaded,
	unit_stopped,
	unit_playing,
	unit_paused,
	unit_disconnected
}
unit_status;

/** Status structure.
*/

typedef struct
{
	int unit;
	unit_status status;
	char clip[ 2048 ];
	int64_t position;
	int speed;
	double fps;
	int64_t in;
	int64_t out;
	int64_t length;
	char tail_clip[ 2048 ];
	int64_t tail_position;
	int64_t tail_in;
	int64_t tail_out;
	int64_t tail_length;
	int seek_flag;
	int generation;
	int clip_index;
	int dummy;
}
*valerie_status, valerie_status_t;

/** DV1394 Status API
*/

extern void valerie_status_parse( valerie_status, char * );
extern char *valerie_status_serialise( valerie_status, char *, int );
extern int valerie_status_compare( valerie_status, valerie_status );
extern valerie_status valerie_status_copy( valerie_status, valerie_status );

#ifdef __cplusplus
}
#endif

#endif
