/*
 * mlt_consumer.c -- abstraction for all consumer services
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

#include "config.h"
#include "mlt_consumer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/** Public final methods
*/

int mlt_consumer_init( mlt_consumer this, void *child )
{
	memset( this, 0, sizeof( struct mlt_consumer_s ) );
	this->child = child;
	return mlt_service_init( &this->parent, this );
}

/** Get the parent service object.
*/

mlt_service mlt_consumer_service( mlt_consumer this )
{
	return &this->parent;
}

/** Get the consumer properties.
*/

mlt_properties mlt_consumer_properties( mlt_consumer this )
{
	return mlt_service_properties( &this->parent );
}

/** Connect the consumer to the producer.
*/

int mlt_consumer_connect( mlt_consumer this, mlt_service producer )
{
	return mlt_service_connect_producer( &this->parent, producer, 0 );
}

/** Start the consumer.
*/

int mlt_consumer_start( mlt_consumer this )
{
	if ( this->start != NULL )
		return this->start( this );
	return 0;
}

/** Stop the consumer.
*/

int mlt_consumer_stop( mlt_consumer this )
{
	if ( this->stop != NULL )
		return this->stop( this );
	return 0;
}

/** Determine if the consumer is stopped.
*/

int mlt_consumer_is_stopped( mlt_consumer this )
{
	if ( this->is_stopped != NULL )
		return this->is_stopped( this );
	return 0;
}

/** Close the consumer.
*/

void mlt_consumer_close( mlt_consumer this )
{
	// Get the childs close function
	void ( *consumer_close )( ) = this->close;

	// Make sure it only gets called once
	this->close = NULL;

	// Call the childs close if available
	if ( consumer_close != NULL )
		consumer_close( this );
	else
		mlt_service_close( &this->parent );
}

