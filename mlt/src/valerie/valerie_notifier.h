/*
 * valerie_notifier.h -- Unit Status Notifier Handling
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

#ifndef _VALERIE_NOTIFIER_H_
#define _VALERIE_NOTIFIER_H_

/* System header files */
#include <pthread.h>

/* Application header files */
#include "valerie_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_UNITS 16

/** Status notifier definition.
*/

typedef struct
{
	pthread_mutex_t mutex;
	pthread_mutex_t cond_mutex;
	pthread_cond_t cond;
	valerie_status_t last;
	valerie_status_t store[ MAX_UNITS ];
}
*valerie_notifier, valerie_notifier_t;

extern valerie_notifier valerie_notifier_init( );
extern void valerie_notifier_get( valerie_notifier, valerie_status, int );
extern int valerie_notifier_wait( valerie_notifier, valerie_status );
extern void valerie_notifier_put( valerie_notifier, valerie_status );
extern void valerie_notifier_disconnected( valerie_notifier );
extern void valerie_notifier_close( valerie_notifier );

#ifdef __cplusplus
}
#endif

#endif
