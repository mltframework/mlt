/**
 * \file mlt_profile.c
 * \brief least recently used cache
 * \see mlt_profile_s
 *
 * Copyright (C) 2007-2009 Ushodaya Enterprises Limited
 * \author Dan Dennedy <dan@dennedy.org>
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

#include "mlt_types.h"
#include "mlt_log.h"
#include "mlt_properties.h"
#include "mlt_cache.h"

#include <stdlib.h>
#include <pthread.h>

/** the maximum number of data objects to cache per line */
#define MAX_CACHE_SIZE (10)

/** \brief Cache item class
 *
 * A cache item is a structure holding information about a data object including
 * a reference count that is used to control its lifetime. When you get a
 * a cache item from the cache, you hold a reference that prevents the data
 * from being released when the cache is full and something new is added.
 * When you close the cache item, the reference count is decremented.
 * The data object is destroyed when all cache items are closed and the cache
 * releases its reference.
 */

typedef struct mlt_cache_item_s
{
	mlt_cache cache;           /**< a reference to the cache to which this belongs */
	void *object;              /**< a parent object to the cache data that uniquely identifies this cached item */
	void *data;                /**< the opaque pointer to the cached data */
	int size;                  /**< the size of the cached data */
	int refcount;              /**< a reference counter to control when destructor is called */
	mlt_destructor destructor; /**< a function to release or destroy the cached data */
} mlt_cache_item_s;

/** \brief Cache class
 *
 * This is a utility class for implementing a Least Recently Used (LRU) cache
 * of data blobs indexed by the address of some other object (e.g., a service).
 * Instead of sorting and manipulating linked lists, it tries to be simple and
 * elegant by copying pointers between two arrays of fixed size to shuffle the
 * order of elements.
 *
 * This class is useful if you have a service that wants to cache something
 * somewhat large, but will not scale if there are many instances of the service.
 * Of course, the service will need to know how to recreate the cached element
 * if it gets flushed from the cache,
 *
 * The most obvious examples are the pixbuf and qimage producers that cache their
 * respective objects representing a picture read from a file. If the picture
 * is no longer in the cache, it can simply re-read it from file. However, a
 * picture is often repeated over many frames and makes sense to cache instead
 * of continually reading, parsing, and decoding. On the other hand, you might
 * want to load hundreds of pictures as individual producers, which would use
 * a lot of memory if every picture is held in memory!
 */

struct mlt_cache_s
{
	int count;             /**< the number of items currently in the cache */
	int size;              /**< the maximum number of items permitted in the cache <= \p MAX_CACHE_SIZE */
	void* *current;        /**< pointer to the current array of pointers */
	void* A[ MAX_CACHE_SIZE ];
	void* B[ MAX_CACHE_SIZE ];
	pthread_mutex_t mutex; /**< a mutex to prevent multi-threaded race conditions */
	mlt_properties active; /**< a list of cache items some of which may no longer
	                            be in \p current but to which there are
	                            outstanding references */
	mlt_properties garbage;/**< a list cache items pending release. A cache item
	                            is copied to this list when it is updated but there
	                            are outstanding references to the old data object. */
};

/** Get the data pointer from the cache item.
 *
 * \public \memberof mlt_cache_s
 * \param item a cache item
 * \param[out] size the number of bytes pointed at, if supplied when putting the data into the cache
 * \return the data pointer
 */

void *mlt_cache_item_data( mlt_cache_item item, int *size )
{
	if ( size && item )
		*size = item->size;
	return item? item->data : NULL;
}

/** Close a cache item given its parent object pointer.
 *
 * \private \memberof mlt_cache_s
 * \param cache a cache
 * \param object the object to which the data object belongs
 * \param data the data object, which might be in the garbage list (optional)
 */

static void cache_object_close( mlt_cache cache, void *object, void* data )
{
	char key[19];

	// Fetch the cache item from the active list by its owner's address
	sprintf( key, "%p", object );
	mlt_cache_item item = mlt_properties_get_data( cache->active, key, NULL );
	if ( item )
	{
		mlt_log( NULL, MLT_LOG_DEBUG, "%s: item %p object %p data %p refcount %d\n", __FUNCTION__,
			item, item->object, item->data, item->refcount );
		if ( item->destructor && --item->refcount <= 0 )
		{
			// Destroy the data object
			item->destructor( item->data );
			item->data = NULL;
			item->destructor = NULL;
			// Do not dispose of the cache item because it could likely be used
			// again.
		}
	}

	// Fetch the cache item from the garbage collection by its data address
	if ( data )
	{
		sprintf( key, "%p", data );
		item = mlt_properties_get_data( cache->garbage, key, NULL );
		if ( item )
		{
			mlt_log( NULL, MLT_LOG_DEBUG, "collecting garbage item %p object %p data %p refcount %d\n",
				item, item->object, item->data, item->refcount );
			if ( item->destructor && --item->refcount <= 0 )
			{
				item->destructor( item->data );
				item->data = NULL;
				item->destructor = NULL;
				// We do not need the garbage-collected cache item
				mlt_properties_set_data( cache->garbage, key, NULL, 0, NULL, NULL );
			}
		}
	}
}

/** Close a cache item.
 *
 * Release a reference and call the destructor on the data object when all
 * references are released.
 *
 * \public \memberof mlt_cache_item_s
 * \param item a cache item
 */

void mlt_cache_item_close( mlt_cache_item item )
{
	if ( item )
	{
		pthread_mutex_lock( &item->cache->mutex );
		cache_object_close( item->cache, item->object, item->data );
		pthread_mutex_unlock( &item->cache->mutex );
	}
}

/** Create a new cache.
 *
 * The default size is \p MAX_CACHE_SIZE.
 * \public \memberof mlt_cache_s
 * \return a new cache or NULL if there was an error
 */

mlt_cache mlt_cache_init()
{
	mlt_cache result = calloc( 1, sizeof( struct mlt_cache_s ) );
	if ( result )
	{
		result->size = MAX_CACHE_SIZE;
		result->current = result->A;
		pthread_mutex_init( &result->mutex, NULL );
		result->active = mlt_properties_new();
		result->garbage = mlt_properties_new();
	}
	return result;
}

/** Set the numer of items to cache.
 *
 * This must be called before using the cache. The size can not be more
 * than \p MAX_CACHE_SIZE.
 * \public \memberof mlt_cache_s
 * \param cache the cache to adjust
 * \param size the new size of the cache
 */

void mlt_cache_set_size( mlt_cache cache, int size )
{
	if ( size <= MAX_CACHE_SIZE )
		cache->size = size;
}

/** Destroy a cache.
 *
 * \public \memberof mlt_cache_s
 * \param cache the cache to destroy
 */

void mlt_cache_close( mlt_cache cache )
{
	if ( cache )
	{
		while ( cache->count-- )
		{
			void *object = cache->current[ cache->count ];
			mlt_log( NULL, MLT_LOG_DEBUG, "%s: %d = %p\n", __FUNCTION__, cache->count, object );
			cache_object_close( cache, object, NULL );
		}
		mlt_properties_close( cache->active );
		mlt_properties_close( cache->garbage );
		pthread_mutex_destroy( &cache->mutex );
		free( cache );
	}
}

/** Remove cache entries for an object.
 *
 * \public \memberof mlt_cache_s
 * \param cache a cache
 * \param object the object that owns the cached data
 */

void mlt_cache_purge( mlt_cache cache, void *object )
{
	pthread_mutex_lock( &cache->mutex );
	if ( cache && object )
	{
		int i, j;
		void **alt = cache->current == cache->A ? cache->B : cache->A;

		for ( i = 0, j = 0; i < cache->count; i++ )
		{
			void *o = cache->current[ i ];

			if ( o == object )
			{
				cache_object_close( cache, o, NULL );
			}
			else
			{
				alt[ j++ ] = o;
			}
		}
		cache->count = j;
		cache->current = alt;
	}
	pthread_mutex_unlock( &cache->mutex );
}

/** Shuffle the cache entries between the two arrays and return the cache entry for an object.
 *
 * \private \memberof mlt_cache_s
 * \param cache a cache object
 * \param object the object that owns the cached data
 * \return a cache entry if there was a hit or NULL for a miss
 */

static void** shuffle_get_hit( mlt_cache cache, void *object )
{
	int i = cache->count;
	int j = cache->count - 1;
	void **hit = NULL;
	void **alt = cache->current == cache->A ? cache->B : cache->A;

	if ( cache->count > 0 && cache->count < cache->size )
	{
		// first determine if we have a hit
		while ( i-- && !hit )
		{
			void **o = &cache->current[ i ];
			if ( *o == object )
				hit = o;
		}
		// if there was no hit, we will not be shuffling out an entry
		// and are still filling the cache
		if ( !hit )
			++j;
		// reset these
		i = cache->count;
		hit = NULL;
	}

	// shuffle the existing entries to the alternate array
	while ( i-- )
	{
		void **o = &cache->current[ i ];

		if ( !hit && *o == object )
		{
			hit = o;
		}
		else if ( j > 0 )
		{
			alt[ --j ] = *o;
// 			mlt_log( NULL, MLT_LOG_DEBUG, "%s: shuffle %d = %p\n", __FUNCTION__, j, alt[j] );
		}
	}
	return hit;
}

/** Put a chunk of data in the cache.
 *
 * \public \memberof mlt_cache_s
 * \param cache a cache object
 * \param object the object to which this data belongs
 * \param data an opaque pointer to the data to cache
 * \param size the size of the data in bytes
 * \param destructor a pointer to a function that can destroy or release a reference to the data.
 */

void mlt_cache_put( mlt_cache cache, void *object, void* data, int size, mlt_destructor destructor )
{
	pthread_mutex_lock( &cache->mutex );
	void **hit = shuffle_get_hit( cache, object );
	void **alt = cache->current == cache->A ? cache->B : cache->A;

	// add the object to the cache
	if ( hit )
	{
		// release the old data
		cache_object_close( cache, *hit, NULL );
		// the MRU end gets the updated data
		hit = &alt[ cache->count - 1 ];
	}
	else if ( cache->count < cache->size )
	{
		// more room in cache, add it to MRU end
		hit = &alt[ cache->count++ ];
	}
	else
	{
		// release the entry at the LRU end
		cache_object_close( cache, cache->current[0], NULL );

		// The MRU end gets the new item
		hit = &alt[ cache->count - 1 ];
	}
	*hit = object;
	mlt_log( NULL, MLT_LOG_DEBUG, "%s: put %d = %p, %p\n", __FUNCTION__, cache->count - 1, object, data );

	// Fetch the cache item
	char key[19];
	sprintf( key, "%p", object );
	mlt_cache_item item = mlt_properties_get_data( cache->active, key, NULL );
	if ( !item )
	{
		item = calloc( 1, sizeof( mlt_cache_item_s ) );
		if ( item )
			mlt_properties_set_data( cache->active, key, item, 0, free, NULL );
	}
	if ( item )
	{
		// If updating the cache item but not all references are released
		// copy the item to the garbage collection.
		if ( item->refcount > 0 && item->data )
		{
			mlt_cache_item orphan = calloc( 1, sizeof( mlt_cache_item_s ) );
			if ( orphan )
			{
				mlt_log( NULL, MLT_LOG_DEBUG, "adding to garbage collection object %p data %p\n", item->object, item->data );
				*orphan = *item;
				sprintf( key, "%p", orphan->data );
				// We store in the garbage collection by data address, not the owner's!
				mlt_properties_set_data( cache->garbage, key, orphan, 0, free, NULL );
			}
		}

		// Set/update the cache item
		item->cache = cache;
		item->object = object;
		item->data = data;
		item->size = size;
		item->destructor = destructor;
		item->refcount = 1;
	}
	
	// swap the current array
	cache->current = alt;
	pthread_mutex_unlock( &cache->mutex );
}

/** Get a chunk of data from the cache.
 *
 * \public \memberof mlt_cache_s
 * \param cache a cache object
 * \param object the object for which you are trying to locate the data
 * \return a mlt_cache_item if found or NULL if not found or has been flushed from the cache
 */

mlt_cache_item mlt_cache_get( mlt_cache cache, void *object )
{
	mlt_cache_item result = NULL;
	pthread_mutex_lock( &cache->mutex );
	void **hit = shuffle_get_hit( cache, object );
	void **alt = cache->current == cache->A ? cache->B : cache->A;

	if ( hit )
	{
		// copy the hit to the MRU end
		alt[ cache->count - 1 ] = *hit;
		hit = &alt[ cache->count - 1 ];

		char key[19];
		sprintf( key, "%p", *hit );
		result = mlt_properties_get_data( cache->active, key, NULL );
		if ( result && result->data )
			result->refcount++;
		mlt_log( NULL, MLT_LOG_DEBUG, "%s: get %d = %p, %p\n", __FUNCTION__, cache->count - 1, *hit, result->data );

		// swap the current array
		cache->current = alt;
	}
	pthread_mutex_unlock( &cache->mutex );
	
	return result;
}
