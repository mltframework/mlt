/*
 * mlt_consumer.h -- abstraction for all consumer services
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
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

#ifndef _MLT_CONSUMER_H_
#define _MLT_CONSUMER_H_

#include "mlt_service.h"
#include <pthread.h>

/** The interface definition for all consumers.
*/

struct mlt_consumer_s
{
	// We're implementing service here
	struct mlt_service_s parent;

	// public virtual
	int ( *start )( mlt_consumer );
	int ( *stop )( mlt_consumer );
	int ( *is_stopped )( mlt_consumer );
	void ( *close )( mlt_consumer );

	// Private data
	void *private;
	void *child;

	int ahead;
	mlt_image_format format;
	mlt_deque queue;
	pthread_t ahead_thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

/** Public final methods
*/

extern int mlt_consumer_init( mlt_consumer this, void *child );
extern mlt_consumer mlt_consumer_new( );
extern mlt_service mlt_consumer_service( mlt_consumer this );
extern mlt_properties mlt_consumer_properties( mlt_consumer this );
extern int mlt_consumer_connect( mlt_consumer this, mlt_service producer );
extern int mlt_consumer_start( mlt_consumer this );
extern mlt_frame mlt_consumer_get_frame( mlt_consumer this );
extern mlt_frame mlt_consumer_rt_frame( mlt_consumer this );
extern int mlt_consumer_stop( mlt_consumer this );
extern int mlt_consumer_is_stopped( mlt_consumer this );
extern void mlt_consumer_close( mlt_consumer );

#endif
