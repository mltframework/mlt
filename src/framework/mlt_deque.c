/**
 * \file mlt_deque.c
 * \brief double ended queue
 * \see mlt_deque_s
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

// Local header files
#include "mlt_deque.h"

// System header files
#include <stdlib.h>
#include <string.h>

/** \brief Deque entry class
 *
 */

typedef union
{
	void *addr;
	int value;
	double floating;
}
deque_entry;

/** \brief Double-Ended Queue (deque) class
 *
 * The double-ended queue is a very versatile data structure. MLT uses it as
 * list, stack, and circular queue.
 */

struct mlt_deque_s
{
	deque_entry *list;
	int size;
	int count;
};

/** Create a deque.
 *
 * \public \memberof mlt_deque_s
 * \return a new deque
 */

mlt_deque mlt_deque_init( )
{
	mlt_deque self = calloc( 1, sizeof( struct mlt_deque_s ) );
	return self;
}

/** Return the number of items in the deque.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \return the number of items
 */

int mlt_deque_count( mlt_deque self )
{
	if ( self )
		return self->count;
	else
		return 0;
}

/** Allocate space on the deque.
 *
 * \private \memberof mlt_deque_s
 * \param self a deque
 * \return true if there was an error
 */

static int mlt_deque_allocate( mlt_deque self )
{
	if ( self->count == self->size )
	{
		self->list = realloc( self->list, sizeof( deque_entry ) * ( self->size + 20 ) );
		self->size += 20;
	}
	return self->list == NULL;
}

/** Push an item to the end.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \param item an opaque pointer
 * \return true if there was an error
 */

int mlt_deque_push_back( mlt_deque self, void *item )
{
	int error = mlt_deque_allocate( self );

	if ( error == 0 )
		self->list[ self->count ++ ].addr = item;

	return error;
}

/** Pop an item.
 *
 * \public \memberof mlt_deque_s
 * \param self a pointer
 * \return an opaque pointer
 */

void *mlt_deque_pop_back( mlt_deque self )
{
	return self->count > 0 ? self->list[ -- self->count ].addr : NULL;
}

/** Queue an item at the start.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \param item an opaque pointer
 * \return true if there was an error
 */

int mlt_deque_push_front( mlt_deque self, void *item )
{
	int error = mlt_deque_allocate( self );

	if ( error == 0 )
	{
		memmove( &self->list[ 1 ], self->list, ( self->count ++ ) * sizeof( deque_entry ) );
		self->list[ 0 ].addr = item;
	}

	return error;
}

/** Remove an item from the start.
 *
 * \public \memberof mlt_deque_s
 * \param self a pointer
 * \return an opaque pointer
 */

void *mlt_deque_pop_front( mlt_deque self )
{
	void *item = NULL;

	if ( self->count > 0 )
	{
		item = self->list[ 0 ].addr;
		memmove( self->list, &self->list[ 1 ], ( -- self->count ) * sizeof( deque_entry ) );
	}

	return item;
}

/** Inquire on item at back of deque but don't remove.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \return an opaque pointer
 */

void *mlt_deque_peek_back( mlt_deque self )
{
	return self->count > 0 ? self->list[ self->count - 1 ].addr : NULL;
}

/** Inquire on item at front of deque but don't remove.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \return an opaque pointer
 */

void *mlt_deque_peek_front( mlt_deque self )
{
	return self->count > 0 ? self->list[ 0 ].addr : NULL;
}

/** Inquire on item in deque but don't remove.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \param index the position in the deque
 * \return an opaque pointer
 */

void *mlt_deque_peek( mlt_deque self, int index )
{
	return self->count > index ? self->list[ index ].addr : NULL;
}

/** Insert an item in a sorted fashion.
 *
 * Optimized for the equivalent of \p mlt_deque_push_back.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \param item an opaque pointer
 * \param cmp a function pointer to the comparison function
 * \return true if there was an error
 */

int mlt_deque_insert( mlt_deque self, void *item, mlt_deque_compare cmp )
{
	int error = mlt_deque_allocate( self );

	if ( error == 0 )
	{
		int n = self->count + 1;
		while ( --n )
			if ( cmp( item, self->list[ n - 1 ].addr ) >= 0 )
				break;
		memmove( &self->list[ n + 1 ], &self->list[ n ], ( self->count - n ) * sizeof( deque_entry ) );
		self->list[ n ].addr = item;
		self->count++;
	}
	return error;
}

/** Push an integer to the end.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \param item an integer
 * \return true if there was an error
 */

int mlt_deque_push_back_int( mlt_deque self, int item )
{
	int error = mlt_deque_allocate( self );

	if ( error == 0 )
		self->list[ self->count ++ ].value = item;

	return error;
}

/** Pop an integer.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \return an integer
 */

int mlt_deque_pop_back_int( mlt_deque self )
{
	return self->count > 0 ? self->list[ -- self->count ].value : 0;
}

/** Queue an integer at the start.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \param item an integer
 * \return true if there was an error
 */

int mlt_deque_push_front_int( mlt_deque self, int item )
{
	int error = mlt_deque_allocate( self );

	if ( error == 0 )
	{
		memmove( &self->list[ 1 ], self->list, ( self->count ++ ) * sizeof( deque_entry ) );
		self->list[ 0 ].value = item;
	}

	return error;
}

/** Remove an integer from the start.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \return an integer
 */

int mlt_deque_pop_front_int( mlt_deque self )
{
	int item = 0;

	if ( self->count > 0 )
	{
		item = self->list[ 0 ].value;
		memmove( self->list, &self->list[ 1 ], ( -- self->count ) * sizeof( deque_entry ) );
	}

	return item;
}

/** Inquire on an integer at back of deque but don't remove.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \return an integer
 */

int mlt_deque_peek_back_int( mlt_deque self )
{
	return self->count > 0 ? self->list[ self->count - 1 ].value : 0;
}

/** Inquire on an integer at front of deque but don't remove.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \return an integer
 */

int mlt_deque_peek_front_int( mlt_deque self )
{
	return self->count > 0 ? self->list[ 0 ].value : 0;
}

/** Push a double float to the end.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \param item a double float
 * \return true if there was an error
 */

int mlt_deque_push_back_double( mlt_deque self, double item )
{
	int error = mlt_deque_allocate( self );

	if ( error == 0 )
		self->list[ self->count ++ ].floating = item;

	return error;
}

/** Pop a double float.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \return a double float
 */

double mlt_deque_pop_back_double( mlt_deque self )
{
	return self->count > 0 ? self->list[ -- self->count ].floating : 0;
}

/** Queue a double float at the start.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \param item a double float
 * \return true if there was an error
 */

int mlt_deque_push_front_double( mlt_deque self, double item )
{
	int error = mlt_deque_allocate( self );

	if ( error == 0 )
	{
		memmove( &self->list[ 1 ], self->list, ( self->count ++ ) * sizeof( deque_entry ) );
		self->list[ 0 ].floating = item;
	}

	return error;
}

/** Remove a double float from the start.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \return a double float
 */

double mlt_deque_pop_front_double( mlt_deque self )
{
	double item = 0;

	if ( self->count > 0 )
	{
		item = self->list[ 0 ].floating;
		memmove( self->list, &self->list[ 1 ], ( -- self->count ) * sizeof( deque_entry ) );
	}

	return item;
}

/** Inquire on a double float at back of deque but don't remove.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \return a double float
 */

double mlt_deque_peek_back_double( mlt_deque self )
{
	return self->count > 0 ? self->list[ self->count - 1 ].floating : 0;
}

/** Inquire on a double float at front of deque but don't remove.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 * \return a double float
 */

double mlt_deque_peek_front_double( mlt_deque self )
{
	return self->count > 0 ? self->list[ 0 ].floating : 0;
}

/** Destroy the queue.
 *
 * \public \memberof mlt_deque_s
 * \param self a deque
 */

void mlt_deque_close( mlt_deque self )
{
	free( self->list );
	free( self );
}
