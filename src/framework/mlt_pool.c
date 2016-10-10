/**
 * \file mlt_pool.c
 * \brief memory pooling functionality
 * \see mlt_pool_s
 *
 * Copyright (C) 2003-2014 Meltytech, LLC
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mlt_properties.h"
#include "mlt_deque.h"
#include "mlt_log.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Not nice - memalign is defined here apparently?
#ifdef linux
#include <malloc.h>
#endif

// Macros to re-assign system functions.
#ifdef _WIN32
#  define mlt_free _aligned_free
#  define mlt_alloc(X) _aligned_malloc( (X), 16 )
#else
#  define mlt_free free
#  ifdef linux
#    define mlt_alloc(X) memalign( 16, (X) )
#  else
#    define mlt_alloc(X) malloc( (X) )
#  endif
#endif

/** global singleton for tracking pools */

static mlt_properties pools = NULL;

/** \brief Pool (memory) class
 */

typedef struct mlt_pool_s
{
	pthread_mutex_t lock; ///< lock to prevent race conditions
	mlt_deque stack;      ///< a stack of addresses to memory blocks
	int size;             ///< the size of the memory block as a power of 2
	int count;            ///< the number of blocks in the pool
}
*mlt_pool;

/** \brief private to mlt_pool_s, for tracking items to release
 *
 * Aligned to 16 byte in case we toss buffers to external assembly
 * optimized libraries (sse/altivec).
 */

typedef struct __attribute__ ((aligned (16))) mlt_release_s
{
	mlt_pool pool;
	int references;
}
*mlt_release;

/** Create a pool.
 *
 * \private \memberof mlt_pool_s
 * \param size the size of the memory blocks to hold as some power of two
 * \return a new pool object
 */

static mlt_pool pool_init( int size )
{
	// Create the pool
	mlt_pool self = calloc( 1, sizeof( struct mlt_pool_s ) );

	// Initialise it
	if ( self != NULL )
	{
		// Initialise the mutex
		pthread_mutex_init( &self->lock, NULL );

		// Create the stack
		self->stack = mlt_deque_init( );

		// Assign the size
		self->size = size;
	}

	// Return it
	return self;
}

/** Get an item from the pool.
 *
 * \private \memberof mlt_pool_s
 * \param self a pool
 * \return an opaque pointer
 */

static void *pool_fetch( mlt_pool self )
{
	// We will generate a release object
	void *ptr = NULL;

	// Sanity check
	if ( self != NULL )
	{
		// Lock the pool
		pthread_mutex_lock( &self->lock );

		// Check if the stack is empty
		if ( mlt_deque_count( self->stack ) != 0 )
		{
			// Pop the top of the stack
			ptr = mlt_deque_pop_back( self->stack );

			// Assign the reference
			( ( mlt_release )ptr )->references = 1;
		}
		else
		{
			// We need to generate a release item
			mlt_release release = mlt_alloc( self->size );

			// If out of memory, log it, reclaim memory, and try again.
			if ( !release && self->size > 0 )
			{
				mlt_log_fatal( NULL, "[mlt_pool] out of memory\n" );
				mlt_pool_purge();
				release = mlt_alloc( self->size );
			}

			// Initialise it
			if ( release != NULL )
			{
				// Increment the number of items allocated to this pool
				self->count ++;

				// Assign the pool
				release->pool = self;

				// Assign the reference
				release->references = 1;

				// Determine the ptr
				ptr = ( char * )release + sizeof( struct mlt_release_s );
			}
		}

		// Unlock the pool
		pthread_mutex_unlock( &self->lock );
	}

	// Return the generated release object
	return ptr;
}

/** Return an item to the pool.
 *
 * \private \memberof mlt_pool_s
 * \param ptr an opaque pointer
 */

static void pool_return( void *ptr )
{
	// Sanity checks
	if ( ptr != NULL )
	{
		// Get the release pointer
		mlt_release that = ( void * )(( char * )ptr - sizeof( struct mlt_release_s ));

		// Get the pool
		mlt_pool self = that->pool;

		if ( self != NULL )
		{
			// Lock the pool
			pthread_mutex_lock( &self->lock );

			// Push the that back back on to the stack
			mlt_deque_push_back( self->stack, ptr );

			// Unlock the pool
			pthread_mutex_unlock( &self->lock );

			return;
		}

		// Free the release itself
		mlt_free( ( char * )ptr - sizeof( struct mlt_release_s ) );
	}
}

/** Destroy a pool.
 *
 * \private \memberof mlt_pool_s
 * \param self a pool
 */

static void pool_close( mlt_pool self )
{
	if ( self != NULL )
	{
		// We need to free up all items in the pool
		void *release = NULL;

		// Iterate through the stack until depleted
		while ( ( release = mlt_deque_pop_back( self->stack ) ) != NULL )
		{
			// We'll free this item now
			mlt_free( ( char * )release - sizeof( struct mlt_release_s ) );
		}

		// We can now close the stack
		mlt_deque_close( self->stack );

		// Destroy the mutex
		pthread_mutex_destroy( &self->lock );

		// Close the pool
		free( self );
	}
}

/** Initialise the global pool.
 *
 * \public \memberof mlt_pool_s
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
 *
 * \public \memberof mlt_pool_s
 * \param size the number of bytes
 */

void *mlt_pool_alloc( int size )
{
	// This will be used to obtain the pool to use
	mlt_pool pool = NULL;

	// Determines the index of the pool to use
	int index = 8;

	// Minimum size pooled is 256 bytes
	size += sizeof( struct mlt_release_s );
	while ( ( 1 << index ) < size )
		index ++;

	// Now get the pool at the index
	pool = mlt_properties_get_data_at( pools, index - 8, NULL );

	// Now get the real item
	return pool_fetch( pool );
}

/** Allocate size bytes from the pool.
 *
 * \public \memberof mlt_pool_s
 * \param ptr an opaque pointer - can be in the pool or a new block to allocate
 * \param size the number of bytes
 */

void *mlt_pool_realloc( void *ptr, int size )
{
	// Result to return
	void *result = NULL;

	// Check if we actually have an address
	if ( ptr != NULL )
	{
		// Get the release pointer
		mlt_release that = ( void * )(( char * )ptr - sizeof( struct mlt_release_s ));

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
 *
 * A form of garbage collection.
 * \public \memberof mlt_pool_s
 */

void mlt_pool_purge( )
{
	int i = 0;

	// For each pool
	for ( i = 0; i < mlt_properties_count( pools ); i ++ )
	{
		// Get the pool
		mlt_pool self = mlt_properties_get_data_at( pools, i, NULL );

		// Pointer to unused memory
		void *release = NULL;

		// Lock the pool
		pthread_mutex_lock( &self->lock );

		// We'll free all unused items now
		while ( ( release = mlt_deque_pop_back( self->stack ) ) != NULL )
		{
			mlt_free( ( char * )release - sizeof( struct mlt_release_s ) );
			self->count--;
		}

		// Unlock the pool
		pthread_mutex_unlock( &self->lock );
	}
}

/** Release the allocated memory.
 *
 * \public \memberof mlt_pool_s
 * \param release an opaque pointer of a block in the pool
 */

void mlt_pool_release( void *release )
{
	// Return to the pool
	pool_return( release );
}

/** Close the pool.
 *
 * \public \memberof mlt_pool_s
 */

void mlt_pool_close( )
{
#ifdef _MLT_POOL_CHECKS_
	mlt_pool_stat( );
#endif

	// Close the properties
	mlt_properties_close( pools );
}

void mlt_pool_stat( )
{
	// Stats dump
	uint64_t allocated = 0, used = 0, s;
	int i = 0, c = mlt_properties_count( pools );

	mlt_log( NULL, MLT_LOG_VERBOSE, "%s: count %d\n", __FUNCTION__, c);

	for ( i = 0; i < c; i ++ )
	{
		mlt_pool pool = mlt_properties_get_data_at( pools, i, NULL );
		if ( pool->count )
			mlt_log_verbose( NULL, "%s: size %d allocated %d returned %d %c\n", __FUNCTION__,
				pool->size, pool->count, mlt_deque_count( pool->stack ),
				pool->count !=  mlt_deque_count( pool->stack ) ? '*' : ' ' );
		s = pool->size; s *= pool->count; allocated += s;
		s = pool->count - mlt_deque_count( pool->stack ); s *= pool->size; used += s;
	}

	mlt_log_verbose( NULL, "%s: allocated %"PRIu64" bytes, used %"PRIu64" bytes \n",
		__FUNCTION__, allocated, used );
}
