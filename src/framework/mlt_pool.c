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

// Not nice - memalign is defined here apparently?
#ifdef linux
#include <malloc.h>
#endif

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
	int count;
}
*mlt_pool;

typedef struct mlt_release_s
{
	mlt_pool pool;
	int references;
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

static void *pool_fetch( mlt_pool this )
{
	// We will generate a release object
	void *ptr = NULL;

	// Sanity check
	if ( this != NULL )
	{
		// Lock the pool
		pthread_mutex_lock( &this->lock );

		// Check if the stack is empty
		if ( mlt_deque_count( this->stack ) != 0 )
		{
			// Pop the top of the stack
			ptr = mlt_deque_pop_back( this->stack );

			// Assign the reference
			( ( mlt_release )ptr )->references = 1;
		}
		else
		{
			// We need to generate a release item
#ifdef linux
			mlt_release release = memalign( 16, this->size );
#else
			mlt_release release = malloc( this->size );
#endif

			// Initialise it
			if ( release != NULL )
			{
				// Increment the number of items allocated to this pool
				this->count ++;

				// Assign the pool
				release->pool = this;

				// Assign the reference
				release->references = 1;

				// Determine the ptr
				ptr = ( void * )release + sizeof( struct mlt_release_s );
			}
		}

		// Unlock the pool
		pthread_mutex_unlock( &this->lock );
	}

	// Return the generated release object
	return ptr;
}

/** Return an item to the pool.
*/

static void pool_return( void *ptr )
{
	// Sanity checks
	if ( ptr != NULL )
	{
		// Get the release pointer
		mlt_release that = ptr - sizeof( struct mlt_release_s );

		// Get the pool
		mlt_pool this = that->pool;

		if ( this != NULL )
		{
			// Lock the pool
			pthread_mutex_lock( &this->lock );

			// Push the that back back on to the stack
			mlt_deque_push_back( this->stack, ptr );

			// Unlock the pool
			pthread_mutex_unlock( &this->lock );

			// Ensure that we don't clean up
			ptr = NULL;
		}
	}

	// Tidy up - this will only occur if the returned item is incorrect
	if ( ptr != NULL )
	{
		// Free the release itself
		free( ptr - sizeof( struct mlt_release_s ) );
	}
}

/** Destroy a pool.
*/

static void pool_close( mlt_pool this )
{
	if ( this != NULL )
	{
		// We need to free up all items in the pool
		void *release = NULL;

		// Iterate through the stack until depleted
		while ( ( release = mlt_deque_pop_back( this->stack ) ) != NULL )
		{
			// We'll free this item now
			free( release - sizeof( struct mlt_release_s ) );
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
	for ( i = 8; i < 31; i ++ )
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

void *mlt_pool_alloc( int size )
{
	// This will be used to obtain the pool to use
	mlt_pool pool = NULL;

	// Determines the index of the pool to use
	int index = 8;

	// Minimum size pooled is 256 bytes
	size = size + sizeof( mlt_release );
	while ( ( 1 << index ) < size )
		index ++;

	// Now get the pool at the index
	pool = mlt_properties_get_data_at( pools, index - 8, NULL );

	// Now get the real item
	return pool_fetch( pool );
}

/** Allocate size bytes from the pool.
*/

void *mlt_pool_realloc( void *ptr, int size )
{
	// Result to return
	void *result = NULL;

	// Check if we actually have an address
	if ( ptr != NULL )
	{
		// Get the release pointer
		mlt_release that = ptr - sizeof( struct mlt_release_s );

		// If the current pool this ptr belongs to is big enough
		if ( size > that->pool->size - sizeof( struct mlt_release_s ) )
		{
			// Allocate
			result = mlt_pool_alloc( size );

			// Copy
			memcpy( result, ptr, that->pool->size - sizeof( struct mlt_release_s ) );

			// Release
			mlt_pool_release( ptr );
		}
		else
		{
			// Nothing to do
			result = ptr;
		}
	}
	else
	{
		// Simply allocate
		result = mlt_pool_alloc( size );
	}

	return result;
}

/** Purge unused items in the pool.
*/

void mlt_pool_purge( )
{
	int i = 0;

	// For each pool
	for ( i = 0; i < mlt_properties_count( pools ); i ++ )
	{
		// Get the pool
		mlt_pool this = mlt_properties_get_data_at( pools, i, NULL );

		// Pointer to unused memory
		void *release = NULL;

		// Lock the pool
		pthread_mutex_lock( &this->lock );

		// We'll free all unused items now
		while ( ( release = mlt_deque_pop_back( this->stack ) ) != NULL )
			free( release - sizeof( struct mlt_release_s ) );

		// Unlock the pool
		pthread_mutex_unlock( &this->lock );
	}
}

/** Release the allocated memory.
*/

void mlt_pool_release( void *release )
{
	// Return to the pool
	pool_return( release );
}

/** Close the pool.
*/

void mlt_pool_close( )
{
#ifdef _MLT_POOL_CHECKS_
	// Stats dump on close
	int i = 0;
	fprintf( stderr, "Usage:\n\n" );
	for ( i = 0; i < mlt_properties_count( pools ); i ++ )
	{
		mlt_pool pool = mlt_properties_get_data_at( pools, i, NULL );
		if ( pool->count )
			fprintf( stderr, "%d: allocated %d returned %d %c\n", pool->size, pool->count, mlt_deque_count( pool->stack ),
																  pool->count !=  mlt_deque_count( pool->stack ) ? '*' : ' ' );
	}
#endif

	// Close the properties
	mlt_properties_close( pools );
}

