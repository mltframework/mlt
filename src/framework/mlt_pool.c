/*
 * mlt_pool.c -- memory pooling functionality
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

#include "mlt_properties.h"
#include "mlt_deque.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/** Singleton repositories
*/

static mlt_properties pools = NULL;

/** Private pooling structure.
*/

typedef struct mlt_pool_s
{
	pthread_mutex_t lock;
	mlt_deque stack;
	int size;
}
*mlt_pool;

/** Private release structure
*/

typedef struct mlt_release_s
{
	void *ptr;
	mlt_pool pool;
}
*mlt_release;

/** Create a pool.
*/

static mlt_pool pool_init( int size )
{
	// Create the pool
	mlt_pool this = calloc( 1, sizeof( struct mlt_pool_s ) );

	// Initialise it
	if ( this != NULL )
	{
		// Initialise the mutex
		pthread_mutex_init( &this->lock, NULL );

		// Create the stack
		this->stack = mlt_deque_init( );

		// Assign the size
		this->size = size;
	}

	// Return it
	return this;
}

/** Get an item from the pool.
*/

static mlt_release pool_fetch( mlt_pool this )
{
	// We will generate a release object
	mlt_release release = NULL;

	// Sanity check
	if ( this != NULL )
	{
		// Lock the pool
		pthread_mutex_lock( &this->lock );

		// Check if the stack is empty
		if ( mlt_deque_count( this->stack ) != 0 )
		{
			// Pop the top of the stack
			release = mlt_deque_pop_back( this->stack );
		}
		else
		{
			// We need to generate a release item
			release = calloc( 1, sizeof( struct mlt_release_s ) );

			// Initialise it
			if ( release != NULL )
			{
				// Allocate the real memory
				release->ptr = malloc( this->size );

				// Assign the pool
				release->pool = this;
			}
		}

		// Unlock the pool
		pthread_mutex_unlock( &this->lock );
	}

	// Return the generated release object
	return release;
}

/** Return an item to the pool.
*/

static void pool_return( mlt_pool this, mlt_release that )
{
	// Sanity checks
	if ( this != NULL && that != NULL )
	{
		// Ensure that the pools match
		if ( this == that->pool )
		{
			// Lock the pool
			pthread_mutex_lock( &this->lock );

			// Push the that back back on to the stack
			mlt_deque_push_back( this->stack, that );

			// Unlock the pool
			pthread_mutex_unlock( &this->lock );

			// Ensure that we don't clean up
			that = NULL;
		}
	}

	// Tidy up - this will only occur if the returned item is incorrect
	if ( that != NULL )
	{
		// Free the memory
		free( that->ptr );

		// Free the release itself
		free( that );
	}
}

/** Destroy a pool.
*/

static void pool_close( mlt_pool this )
{
	if ( this != NULL )
	{
		// We need to free up all items in the pool
		mlt_release release = NULL;

		// Iterate through the stack until depleted
		while ( ( release = mlt_deque_pop_back( this->stack ) ) != NULL )
		{
			// We'll return this item to NULL
			pool_return( NULL, release );
		}

		// We can now close the stack
		mlt_deque_close( this->stack );

		// Destroy the mutex
		pthread_mutex_destroy( &this->lock );

		// Close the pool
		free( this );
	}
}

/** Initialise the pool.
*/

void mlt_pool_init( )
{
	// Loop variable used to create the pools
	int i = 0;

	// Create the pools
	pools = mlt_properties_new( );

	// Create the pools
	for ( i = 8; i < 32; i ++ )
	{
		// Each properties item needs a name
		char name[ 32 ];

		// Construct a pool
		mlt_pool pool = pool_init( 1 << i );

		// Generate a name
		sprintf( name, "%d", i );

		// Register with properties
		mlt_properties_set_data( pools, name, pool, 0, ( mlt_destructor )pool_close, NULL );
	}
}

/** Allocate size bytes from the pool.
*/

void *mlt_pool_allocate( int size, void **release )
{
	// This is the real release structure we'll return
	mlt_release real = NULL;

	// This will be used to obtain the pool to use
	mlt_pool pool = NULL;

	// Determines the index of the pool to use
	int index = 1;

	// Minimum size pooled is 256 bytes
	size = size >> 8;
	while ( ( 1 << index ) < size )
		index ++;

	// Now get the pool at the index
	pool = mlt_properties_get_data_at( pools, index + 1, NULL );

	// Now get the real item
	real = pool_fetch( pool );

	// Deal with return
	if ( real != NULL )
	{
		// Assign to release
		*release = real;

		// Return the pointer
		return real->ptr;
	}

	// Otherwise return a NULL to indicate failure
	return NULL;
}

/** Release the allocated memory.
*/

void mlt_pool_release( void *release )
{
	// Sanity check
	if ( release != NULL )
	{
		// Get the real release structure
		mlt_release real = release;

		// Return to the pool
		pool_return( real->pool, real );
	}
}

/** Close the pool.
*/

void mlt_pool_close( )
{
	// Close the properties
	mlt_properties_close( pools );
}

