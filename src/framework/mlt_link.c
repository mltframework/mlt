/**
 * \file mlt_link.c
 * \brief link service class
 * \see mlt_link_s
 *
 * Copyright (C) 2020 Meltytech, LLC
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

#include "mlt_link.h"
#include "mlt_frame.h"
#include "mlt_log.h"

#include <stdio.h>
#include <stdlib.h>

/* Forward references to static methods.
*/

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int track );
static int producer_seek( mlt_producer parent, mlt_position position );
static int producer_set_in_and_out( mlt_producer, mlt_position, mlt_position );

/** Construct a link.
 *
 * Sets the mlt_type to "link"
 *
 * \public \memberof mlt_link_s
 * \return the new link
 */

mlt_link mlt_link_init( )
{
	mlt_link self = calloc( 1, sizeof( struct mlt_link_s ) );
	if ( self != NULL )
	{
		mlt_producer producer = &self->parent;
		if ( mlt_producer_init( producer, self ) == 0 )
		{
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

			mlt_properties_set( properties, "mlt_type", "link" );
			mlt_properties_clear( properties, "mlt_service");
			mlt_properties_clear( properties, "resource");
			mlt_properties_clear( properties, "in");
			mlt_properties_clear( properties, "out");
			mlt_properties_clear( properties, "length");
			mlt_properties_clear( properties, "eof");
			producer->get_frame = producer_get_frame;
			producer->seek = producer_seek;
			producer->set_in_and_out = producer_set_in_and_out;
			producer->close = ( mlt_destructor )mlt_link_close;
			producer->close_object = self;
		}
		else
		{
			free( self );
			self = NULL;
		}
	}
	return self;
}

/** Connect this link to the next producer.
 *
 * \public \memberof mlt_link_s
 * \param self a link
 * \param next the producer to get frames from
 * \param chain_profile a profile to use if needed (some links derive their frame rate from the next producer)
 * \return true on error
 */

int mlt_link_connect_next( mlt_link self, mlt_producer next, mlt_profile chain_profile )
{
	self->next = next;
	if ( self->configure )
	{
		self->configure( self, chain_profile );
	}
	return 0;
}

/** Close the link and free its resources.
 *
 * \public \memberof mlt_link_s
 * \param self a link
 */

void mlt_link_close( mlt_link self )
{
	if ( self != NULL && mlt_properties_dec_ref( MLT_LINK_PROPERTIES( self ) ) <= 0 )
	{
		if( self->close )
		{
			self->close( self );
		}
		else
		{
			self->parent.close = NULL;
			mlt_producer_close( &self->parent );
		}
	}
}

static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index )
{
	if ( parent && parent->child )
	{
		mlt_link self = parent->child;
		if ( self->get_frame != NULL )
		{
			return self->get_frame( self, frame, index );
		}
		else
		{
			/* Default implementation: get a frame from the next producer */
			return mlt_service_get_frame( MLT_PRODUCER_SERVICE( self->next ), frame, index );
		}
	}
	return 1;
}

int producer_seek( mlt_producer parent, mlt_position position )
{
	// Unlike mlt_producer_seek(), a link does not do bounds checking when seeking
	if ( parent && parent->child )
	{
		mlt_link self = parent->child;
		mlt_properties properties = MLT_LINK_PROPERTIES( self );
		int use_points = 1 - mlt_properties_get_int( properties, "ignore_points" );

		// Set the position
		mlt_properties_set_position( properties, "_position", position );

		// Calculate the absolute frame
		mlt_properties_set_position( properties, "_frame", use_points * mlt_producer_get_in( parent ) + position );
	}
	return 0;
}


int producer_set_in_and_out( mlt_producer parent, mlt_position in, mlt_position out )
{
	// Unlike mlt_producer_set_in_and_out(), a link does not do bounds checking against length
	if ( parent && parent->child )
	{
		mlt_link self = parent->child;
		mlt_properties properties = MLT_LINK_PROPERTIES( self );
		mlt_events_block( properties, properties );
		mlt_properties_set_position( properties, "in", in );
		mlt_events_unblock( properties, properties );
		mlt_properties_set_position( properties, "out", out );
	}
	return 0;
}
