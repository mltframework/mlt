/**
 * \file mlt_producer.c
 * \brief abstraction for all producer services
 * \see mlt_producer_s
 *
 * Copyright (C) 2003-2009 Ushodaya Enterprises Limited
 * \author Charles Yates <charles.yates@pandora.be>
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

#include "mlt_producer.h"
#include "mlt_factory.h"
#include "mlt_frame.h"
#include "mlt_parser.h"
#include "mlt_profile.h"
#include "mlt_log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Forward references. */

static int producer_get_frame( mlt_service self, mlt_frame_ptr frame, int index );
static void mlt_producer_property_changed( mlt_service owner, mlt_producer self, char *name );
static void mlt_producer_service_changed( mlt_service owner, mlt_producer self );

/* for debugging */
//#define _MLT_PRODUCER_CHECKS_ 1
#ifdef _MLT_PRODUCER_CHECKS_
static int producers_created = 0;
static int producers_destroyed = 0;
#endif

/** Initialize a producer service.
 *
 * \public \memberof mlt_producer_s
 * \param self the producer structure to initialize
 * \param child a pointer to the child object for the subclass
 * \return true if there was an error
 */

int mlt_producer_init( mlt_producer self, void *child )
{
	// Check that we haven't received NULL
	int error = self == NULL;

	// Continue if no error
	if ( error == 0 )
	{
#ifdef _MLT_PRODUCER_CHECKS_
		producers_created ++;
#endif

		// Initialise the producer
		memset( self, 0, sizeof( struct mlt_producer_s ) );

		// Associate with the child
		self->child = child;

		// Initialise the service
		if ( mlt_service_init( &self->parent, self ) == 0 )
		{
			// The parent is the service
			mlt_service parent = &self->parent;

			// Define the parent close
			parent->close = ( mlt_destructor )mlt_producer_close;
			parent->close_object = self;

			// For convenience, we'll assume the close_object is self
			self->close_object = self;

			// Get the properties of the parent
			mlt_properties properties = MLT_SERVICE_PROPERTIES( parent );

			// Set the default properties
			mlt_properties_set( properties, "mlt_type", "mlt_producer" );
			mlt_properties_set_position( properties, "_position", 0.0 );
			mlt_properties_set_double( properties, "_frame", 0 );
			mlt_properties_set_double( properties, "_speed", 1.0 );
			mlt_properties_set_position( properties, "in", 0 );
			mlt_properties_set_position( properties, "out", 14999 );
			mlt_properties_set_position( properties, "length", 15000 );
			mlt_properties_set( properties, "eof", "pause" );
			mlt_properties_set( properties, "resource", "<producer>" );

			// Override service get_frame
			parent->get_frame = producer_get_frame;

			mlt_events_listen( properties, self, "service-changed", ( mlt_listener )mlt_producer_service_changed );
			mlt_events_listen( properties, self, "property-changed", ( mlt_listener )mlt_producer_property_changed );
			mlt_events_register( properties, "producer-changed", NULL );
		}
	}

	return error;
}

/** Listener for property changes.
 *
 * If the in, out, or length properties changed, fire a "producer-changed" event.
 *
 * \private \memberof mlt_producer_s
 * \param owner a service (ignored)
 * \param self the producer
 * \param name the property that changed
 */

static void mlt_producer_property_changed( mlt_service owner, mlt_producer self, char *name )
{
	if ( !strcmp( name, "in" ) || !strcmp( name, "out" ) || !strcmp( name, "length" ) )
		mlt_events_fire( MLT_PRODUCER_PROPERTIES( mlt_producer_cut_parent( self ) ), "producer-changed", NULL );
}

/** Listener for service changes.
 *
 * Fires the "producer-changed" event.
 *
 * \private \memberof mlt_producer_s
 * \param owner a service (ignored)
 * \param self the producer
 */

static void mlt_producer_service_changed( mlt_service owner, mlt_producer self )
{
	mlt_events_fire( MLT_PRODUCER_PROPERTIES( mlt_producer_cut_parent( self ) ), "producer-changed", NULL );
}

/** Create and initialize a new producer.
 *
 * \public \memberof mlt_producer_s
 * \return the new producer
 */

mlt_producer mlt_producer_new( mlt_profile profile )
{
	mlt_producer self = malloc( sizeof( struct mlt_producer_s ) );
	if ( self )
	{
		if ( mlt_producer_init( self, NULL ) == 0 )
		{
			mlt_properties_set_data( MLT_PRODUCER_PROPERTIES( self ), "_profile", profile, 0, NULL, NULL );
			mlt_properties_set_double( MLT_PRODUCER_PROPERTIES( self ), "aspect_ratio", mlt_profile_sar( profile ) );
		}
	}
	return self;
}

/** Determine if producer is a cut.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return true if \p self is a "cut" producer
 * \see mlt_producer_cut
 */

int mlt_producer_is_cut( mlt_producer self )
{
	return mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( self ), "_cut" );
}

/** Determine if producer is a mix.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return true if \p self is a "mix" producer
 * \todo Define a mix producer.
 */

int mlt_producer_is_mix( mlt_producer self )
{
	mlt_properties properties = self != NULL ? MLT_PRODUCER_PROPERTIES( self ) : NULL;
	mlt_tractor tractor = properties != NULL ? mlt_properties_get_data( properties, "mlt_mix", NULL ) : NULL;
	return tractor != NULL;
}

/** Determine if the producer is a blank.
 *
 * Blank producers should only appear as an item in a playlist.
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return true if \p self is a "blank" producer
 * \see mlt_playlist_insert_blank
 */

int mlt_producer_is_blank( mlt_producer self )
{
	return self == NULL || !strcmp( mlt_properties_get( MLT_PRODUCER_PROPERTIES( mlt_producer_cut_parent( self ) ), "resource" ), "blank" );
}

/** Obtain the parent producer.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return either the parent producer if \p self is a "cut" producer or \p self otherwise.
 */

mlt_producer mlt_producer_cut_parent( mlt_producer self )
{
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( self );
	if ( mlt_producer_is_cut( self ) )
		return mlt_properties_get_data( properties, "_cut_parent", NULL );
	else
		return self;
}

/** Create a cut of this producer.
 *
 * A "cut" is a portion of another (parent) producer.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \param in the beginning
 * \param out the end
 * \return the new producer
 * \todo Expand on the value of a cut.
 */

mlt_producer mlt_producer_cut( mlt_producer self, int in, int out )
{
	mlt_producer result = mlt_producer_new( mlt_service_profile( MLT_PRODUCER_SERVICE( self ) ) );
	mlt_producer parent = mlt_producer_cut_parent( self );
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( result );
	mlt_properties parent_props = MLT_PRODUCER_PROPERTIES( parent );

	mlt_events_block( MLT_PRODUCER_PROPERTIES( result ), MLT_PRODUCER_PROPERTIES( result ) );
	// Special case - allow for a cut of the entire producer (this will squeeze all other cuts to 0)
	if ( in <= 0 )
		in = 0;
	if ( ( out < 0 || out >= mlt_producer_get_length( parent ) ) && !mlt_producer_is_blank( self ) )
		out = mlt_producer_get_length( parent ) - 1;

	mlt_properties_inc_ref( parent_props );
	mlt_properties_set_int( properties, "_cut", 1 );
	mlt_properties_set_data( properties, "_cut_parent", parent, 0, ( mlt_destructor )mlt_producer_close, NULL );
	mlt_properties_set_position( properties, "length", mlt_properties_get_position( parent_props, "length" ) );
	mlt_properties_set_double( properties, "aspect_ratio", mlt_properties_get_double( parent_props, "aspect_ratio" ) );
	mlt_producer_set_in_and_out( result, in, out );

	return result;
}

/** Get the parent service object.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return the service parent class
 * \see MLT_PRODUCER_SERVICE
 */

mlt_service mlt_producer_service( mlt_producer self )
{
	return self != NULL ? &self->parent : NULL;
}

/** Get the producer properties.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return the producer's property list
 * \see MLT_PRODUCER_PROPERTIES
 */

mlt_properties mlt_producer_properties( mlt_producer self )
{
	return MLT_SERVICE_PROPERTIES( &self->parent );
}

/** Seek to a specified position.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \param position set the "play head" position of the producer
 * \return false
 * \todo Document how the properties affect behavior.
 */

int mlt_producer_seek( mlt_producer self, mlt_position position )
{
	// Determine eof handling
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( self );
	char *eof = mlt_properties_get( properties, "eof" );
	int use_points = 1 - mlt_properties_get_int( properties, "ignore_points" );

	// Recursive behaviour for cuts - repositions parent and then repositions cut
	// hence no return on this condition
	if ( mlt_producer_is_cut( self ) )
		mlt_producer_seek( mlt_producer_cut_parent( self ), position + mlt_producer_get_in( self ) );

	// Check bounds
	if ( position < 0 || mlt_producer_get_playtime( self ) == 0 )
	{
		position = 0;
	}
	else if ( use_points && ( eof == NULL || !strcmp( eof, "pause" ) ) && position >= mlt_producer_get_playtime( self ) )
	{
		mlt_producer_set_speed( self, 0 );
		position = mlt_producer_get_playtime( self ) - 1;
	}
	else if ( use_points && !strcmp( eof, "loop" ) && position >= mlt_producer_get_playtime( self ) )
	{
		position = (int)position % (int)mlt_producer_get_playtime( self );
	}

	// Set the position
	mlt_properties_set_position( MLT_PRODUCER_PROPERTIES( self ), "_position", position );

	// Calculate the absolute frame
	mlt_properties_set_position( MLT_PRODUCER_PROPERTIES( self ), "_frame", use_points * mlt_producer_get_in( self ) + position );

	return 0;
}

/** Get the current position (relative to in point).
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return the position of the "play head" relative to its beginning
 */

mlt_position mlt_producer_position( mlt_producer self )
{
	return mlt_properties_get_position( MLT_PRODUCER_PROPERTIES( self ), "_position" );
}

/** Get the current position (relative to start of producer).
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return the position of the "play head" regardless of the in point
 */

mlt_position mlt_producer_frame( mlt_producer self )
{
	return mlt_properties_get_position( MLT_PRODUCER_PROPERTIES( self ), "_frame" );
}

/** Set the playing speed.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \param speed the new speed as a relative factor (1.0 = normal)
 * \return true if error
 */

int mlt_producer_set_speed( mlt_producer self, double speed )
{
	return mlt_properties_set_double( MLT_PRODUCER_PROPERTIES( self ), "_speed", speed );
}

/** Get the playing speed.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return the speed as a relative factor (1.0 = normal)
 */

double mlt_producer_get_speed( mlt_producer self )
{
	return mlt_properties_get_double( MLT_PRODUCER_PROPERTIES( self ), "_speed" );
}

/** Get the frames per second.
 *
 * This is determined by the producer's profile.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return the video refresh rate
 */

double mlt_producer_get_fps( mlt_producer self )
{
	mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( self ) );
	return mlt_profile_fps( profile );
}

/** Set the in and out points.
 *
 * The in point is where play out should start relative to the natural start
 * of the underlying file. The out point is where play out should end, also
 * relative to the start of the underlying file. If the underlying resource is
 * a live stream, then the in point is an offset relative to first usable
 * sample.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \param in the relative starting time; a negative value is the same as 0
 * \param out the relative ending time; a negative value is the same as length - 1
 * \return false
 */

int mlt_producer_set_in_and_out( mlt_producer self, mlt_position in, mlt_position out )
{
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( self );

	// Correct ins and outs if necessary
	if ( in < 0 )
		in = 0;
	else if ( in >= mlt_producer_get_length( self ) )
		in = mlt_producer_get_length( self ) - 1;

	if ( ( out < 0 || out >= mlt_producer_get_length( self ) ) && !mlt_producer_is_blank( self ) )
		out = mlt_producer_get_length( self ) - 1;
	else if ( ( out < 0 || out >= mlt_producer_get_length( self ) ) && mlt_producer_is_blank( self ) )
		mlt_properties_set_position( MLT_PRODUCER_PROPERTIES( self ), "length", out + 1 );
	else if ( out < 0 )
		out = 0;

	// Swap ins and outs if wrong
	if ( out < in )
	{
		mlt_position t = in;
		in = out;
		out = t;
	}

	// Set the values
	mlt_events_block( properties, properties );
	mlt_properties_set_position( properties, "in", in );
	mlt_events_unblock( properties, properties );
	mlt_properties_set_position( properties, "out", out );

	return 0;
}

/** Physically reduce the producer (typically a cut) to a 0 length.
 *  Essentially, all 0 length cuts should be immediately removed by containers.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return false
 */

int mlt_producer_clear( mlt_producer self )
{
	if ( self != NULL )
	{
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( self );
		mlt_events_block( properties, properties );
		mlt_properties_set_position( properties, "in", 0 );
		mlt_events_unblock( properties, properties );
		mlt_properties_set_position( properties, "out", -1 );
	}
	return 0;
}

/** Get the in point.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return the in point
 */

mlt_position mlt_producer_get_in( mlt_producer self )
{
	return mlt_properties_get_position( MLT_PRODUCER_PROPERTIES( self ), "in" );
}

/** Get the out point.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return the out point
 */

mlt_position mlt_producer_get_out( mlt_producer self )
{
	return mlt_properties_get_position( MLT_PRODUCER_PROPERTIES( self ), "out" );
}

/** Get the total play time.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return the playable (based on in and out points) duration
 */

mlt_position mlt_producer_get_playtime( mlt_producer self )
{
	return mlt_producer_get_out( self ) - mlt_producer_get_in( self ) + 1;
}

/** Get the total, unedited length of the producer.
 *
 * The value returned by a live streaming producer is unknown.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return the duration of the producer regardless of in and out points
 */

mlt_position mlt_producer_get_length( mlt_producer self )
{
	return mlt_properties_get_position( MLT_PRODUCER_PROPERTIES( self ), "length" );
}

/** Prepare for next frame.
 *
 * Advance the play out position. If the speed is less than zero, it will
 * move the play out position in the reverse direction.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 */

void mlt_producer_prepare_next( mlt_producer self )
{
	if ( mlt_producer_get_speed( self ) != 0 )
		mlt_producer_seek( self, mlt_producer_position( self ) + mlt_producer_get_speed( self ) );
}

/** Get a frame.
 *
 * This is the implementation of the \p get_frame virtual function.
 * It requests a new frame object from the actual producer for the current
 * play out position. The producer and its filters can add information and
 * operations to the frame object in their get_frame handlers.
 *
 * \private \memberof mlt_producer_s
 * \param service a service
 * \param[out] frame a frame by reference
 * \param index as determined by the actual producer
 * \return true if there was an error
 * \todo Learn more about the details and document how certain properties affect
 * its behavior.
 */

static int producer_get_frame( mlt_service service, mlt_frame_ptr frame, int index )
{
	int result = 1;
	mlt_producer self = service != NULL ? service->child : NULL;

	if ( self != NULL && !mlt_producer_is_cut( self ) )
	{
		// Get the properties of this producer
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( self );

		// Determine eof handling
		char *eof = mlt_properties_get( MLT_PRODUCER_PROPERTIES( self ), "eof" );

		// Get the speed of the producer
		double speed = mlt_producer_get_speed( self );

		// We need to use the clone if it's specified
		mlt_producer clone = mlt_properties_get_data( properties, "use_clone", NULL );

		// If no clone is specified, use self
		clone = clone == NULL ? self : clone;

		// A properly instatiated producer will have a get_frame method...
		if ( self->get_frame == NULL || ( !strcmp( eof, "continue" ) && mlt_producer_position( self ) > mlt_producer_get_out( self ) ) )
		{
			// Generate a test frame
			*frame = mlt_frame_init( service );

			// Set the position
			result = mlt_frame_set_position( *frame, mlt_producer_position( self ) );

			// Mark as a test card
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( *frame ), "test_image", 1 );
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( *frame ), "test_audio", 1 );

			// Calculate the next position
			mlt_producer_prepare_next( self );
		}
		else
		{
			// Get the frame from the implementation
			result = self->get_frame( clone, frame, index );
		}

		// Copy the fps and speed of the producer onto the frame
		properties = MLT_FRAME_PROPERTIES( *frame );
		mlt_properties_set_double( properties, "_speed", speed );
		mlt_properties_set_int( properties, "test_audio", mlt_frame_is_test_audio( *frame ) );
		mlt_properties_set_int( properties, "test_image", mlt_frame_is_test_card( *frame ) );
		if ( mlt_properties_get_data( properties, "_producer", NULL ) == NULL )
			mlt_properties_set_data( properties, "_producer", service, 0, NULL, NULL );
	}
	else if ( self != NULL )
	{
		// Get the speed of the cut
		double speed = mlt_producer_get_speed( self );

		// Get the parent of the cut
		mlt_producer parent = mlt_producer_cut_parent( self );

		// Get the properties of the parent
		mlt_properties parent_properties = MLT_PRODUCER_PROPERTIES( parent );

		// Get the properties of the cut
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( self );

		// Determine the clone index
		int clone_index = mlt_properties_get_int( properties, "_clone" );

		// Determine the clone to use
		mlt_producer clone = self;

		if ( clone_index > 0 )
		{
			char key[ 25 ];
			sprintf( key, "_clone.%d", clone_index - 1 );
			clone = mlt_properties_get_data( MLT_PRODUCER_PROPERTIES( mlt_producer_cut_parent( self ) ), key, NULL );
			if ( clone == NULL ) mlt_log( service, MLT_LOG_ERROR, "requested clone doesn't exist %d\n", clone_index );
			clone = clone == NULL ? self : clone;
		}
		else
		{
			clone = parent;
		}

		// We need to seek to the correct position in the clone
		mlt_producer_seek( clone, mlt_producer_get_in( self ) + mlt_properties_get_int( properties, "_position" ) );

		// Assign the clone property to the parent
		mlt_properties_set_data( parent_properties, "use_clone", clone, 0, NULL, NULL );

		// Now get the frame from the parents service
		result = mlt_service_get_frame( MLT_PRODUCER_SERVICE( parent ), frame, index );

		// We're done with the clone now
		mlt_properties_set_data( parent_properties, "use_clone", NULL, 0, NULL, NULL );

		// This is useful and required by always_active transitions to determine in/out points of the cut
		if ( mlt_properties_get_data( MLT_FRAME_PROPERTIES( *frame ), "_producer", NULL ) == MLT_PRODUCER_SERVICE( parent ) )
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( *frame ), "_producer", self, 0, NULL, NULL );

		mlt_properties_set_double( MLT_FRAME_PROPERTIES( *frame ), "_speed", speed );
		mlt_producer_prepare_next( self );
	}
	else
	{
		*frame = mlt_frame_init( service );
		result = 0;
	}

	// Pass on all meta properties from the producer/cut on to the frame
	if ( *frame != NULL && self != NULL )
	{
		int i = 0;
		mlt_properties p_props = MLT_PRODUCER_PROPERTIES( self );
		mlt_properties f_props = MLT_FRAME_PROPERTIES( *frame );
		mlt_properties_lock( p_props );
		int count = mlt_properties_count( p_props );
		for ( i = 0; i < count; i ++ )
		{
			char *name = mlt_properties_get_name( p_props, i );
			if ( !strncmp( name, "meta.", 5 ) )
				mlt_properties_set( f_props, name, mlt_properties_get_value( p_props, i ) );
			else if ( !strncmp( name, "set.", 4 ) )
				mlt_properties_set( f_props, name + 4, mlt_properties_get_value( p_props, i ) );
		}
		mlt_properties_unlock( p_props );
	}

	return result;
}

/** Attach a filter.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \param filter the filter to attach
 * \return true if there was an error
 */

int mlt_producer_attach( mlt_producer self, mlt_filter filter )
{
	return mlt_service_attach( MLT_PRODUCER_SERVICE( self ), filter );
}

/** Detach a filter.
 *
 * \public \memberof mlt_producer_s
 * \param self a service
 * \param filter the filter to detach
 * \return true if there was an error
 */

int mlt_producer_detach( mlt_producer self, mlt_filter filter )
{
	return mlt_service_detach( MLT_PRODUCER_SERVICE( self ), filter );
}

/** Retrieve a filter.
 *
 * \public \memberof mlt_producer_s
 * \param self a service
 * \param index which filter to retrieve
 * \return the filter or null if there was an error
 */

mlt_filter mlt_producer_filter( mlt_producer self, int index )
{
	return mlt_service_filter( MLT_PRODUCER_SERVICE( self ), index );
}

/** Clone self producer.
 *
 * \private \memberof mlt_producer_s
 * \param self a producer
 * \return a new producer that is a copy of \p self
 * \see mlt_producer_set_clones
 */

static mlt_producer mlt_producer_clone( mlt_producer self )
{
	mlt_producer clone = NULL;
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( self );
	char *resource = mlt_properties_get( properties, "resource" );
	char *service = mlt_properties_get( properties, "mlt_service" );
	mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( self ) );

	mlt_events_block( mlt_factory_event_object( ), mlt_factory_event_object( ) );

	if ( service != NULL )
		clone = mlt_factory_producer( profile, service, resource );

	if ( clone == NULL && resource != NULL )
		clone = mlt_factory_producer( profile, NULL, resource );

	if ( clone != NULL )
		mlt_properties_inherit( MLT_PRODUCER_PROPERTIES( clone ), properties );

	mlt_events_unblock( mlt_factory_event_object( ), mlt_factory_event_object( ) );

	return clone;
}

/** Create clones.
 *
 * \private \memberof mlt_producer_s
 * \param self a producer
 * \param clones the number of copies to make
 * \see mlt_producer_optimise
 */

static void mlt_producer_set_clones( mlt_producer self, int clones )
{
	mlt_producer parent = mlt_producer_cut_parent( self );
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( parent );
	int existing = mlt_properties_get_int( properties, "_clones" );
	int i = 0;
	char key[ 25 ];

	// If the number of existing clones is different, then create/remove as necessary
	if ( existing != clones )
	{
		if ( existing < clones )
		{
			for ( i = existing; i < clones; i ++ )
			{
				mlt_producer clone = mlt_producer_clone( parent );
				sprintf( key, "_clone.%d", i );
				mlt_properties_set_data( properties, key, clone, 0, ( mlt_destructor )mlt_producer_close, NULL );
			}
		}
		else
		{
			for ( i = clones; i < existing; i ++ )
			{
				sprintf( key, "_clone.%d", i );
				mlt_properties_set_data( properties, key, NULL, 0, NULL, NULL );
			}
		}
	}

	// Ensure all properties on the parent are passed to the clones
	for ( i = 0; i < clones; i ++ )
	{
		mlt_producer clone = NULL;
		sprintf( key, "_clone.%d", i );
		clone = mlt_properties_get_data( properties, key, NULL );
		if ( clone != NULL )
			mlt_properties_pass( MLT_PRODUCER_PROPERTIES( clone ), properties, "" );
	}

	// Update the number of clones on the properties
	mlt_properties_set_int( properties, "_clones", clones );
}

/** \brief private to mlt_producer_s, used by mlt_producer_optimise() */

typedef struct
{
	int multitrack;
	int track;
	int position;
	int length;
	int offset;
}
track_info;

/** \brief private to mlt_producer_s, used by mlt_producer_optimise() */

typedef struct
{
	mlt_producer cut;
	int start;
	int end;
}
clip_references;

static int intersect( clip_references *a, clip_references *b )
{
	int diff = ( a->start - b->start ) + ( a->end - b->end );
	return diff >= 0 && diff < ( a->end - a->start + 1 );
}

static int push( mlt_parser self, int multitrack, int track, int position )
{
	mlt_properties properties = mlt_parser_properties( self );
	mlt_deque stack = mlt_properties_get_data( properties, "stack", NULL );
	track_info *info = malloc( sizeof( track_info ) );
	info->multitrack = multitrack;
	info->track = track;
	info->position = position;
	info->length = 0;
	info->offset = 0;
	return mlt_deque_push_back( stack, info );
}

static track_info *pop( mlt_parser self )
{
	mlt_properties properties = mlt_parser_properties( self );
	mlt_deque stack = mlt_properties_get_data( properties, "stack", NULL );
	return mlt_deque_pop_back( stack );
}

static track_info *peek( mlt_parser self )
{
	mlt_properties properties = mlt_parser_properties( self );
	mlt_deque stack = mlt_properties_get_data( properties, "stack", NULL );
	return mlt_deque_peek_back( stack );
}

static int on_start_multitrack( mlt_parser self, mlt_multitrack object )
{
	track_info *info = peek( self );
	return push( self, info->multitrack ++, info->track, info->position );
}

static int on_start_track( mlt_parser self )
{
	track_info *info = peek( self );
	info->position -= info->offset;
	info->length -= info->offset;
	return push( self, info->multitrack, info->track ++, info->position );
}

static int on_start_producer( mlt_parser self, mlt_producer object )
{
	mlt_properties properties = mlt_parser_properties( self );
	mlt_properties producers = mlt_properties_get_data( properties, "producers", NULL );
	mlt_producer parent = mlt_producer_cut_parent( object );
	if ( mlt_service_identify( ( mlt_service )mlt_producer_cut_parent( object ) ) == producer_type && mlt_producer_is_cut( object ) )
	{
		int ref_count = 0;
		clip_references *old_refs = NULL;
		clip_references *refs = NULL;
		char key[ 50 ];
		int count = 0;
		track_info *info = peek( self );
		sprintf( key, "%p", parent );
		mlt_properties_get_data( producers, key, &count );
		mlt_properties_set_data( producers, key, parent, ++ count, NULL, NULL );
		old_refs = mlt_properties_get_data( properties, key, &ref_count );
		refs = malloc( ( ref_count + 1 ) * sizeof( clip_references ) );
		if ( old_refs != NULL )
			memcpy( refs, old_refs, ref_count * sizeof( clip_references ) );
		mlt_properties_set_int( MLT_PRODUCER_PROPERTIES( object ), "_clone", -1 );
		refs[ ref_count ].cut = object;
		refs[ ref_count ].start = info->position;
		refs[ ref_count ].end = info->position + mlt_producer_get_playtime( object ) - 1;
		mlt_properties_set_data( properties, key, refs, ++ ref_count, free, NULL );
		info->position += mlt_producer_get_playtime( object );
		info->length += mlt_producer_get_playtime( object );
	}
	return 0;
}

static int on_end_track( mlt_parser self )
{
	track_info *track = pop( self );
	track_info *multi = peek( self );
	multi->length += track->length;
	multi->position += track->length;
	multi->offset = track->length;
	free( track );
	return 0;
}

static int on_end_multitrack( mlt_parser self, mlt_multitrack object )
{
	track_info *multi = pop( self );
	track_info *track = peek( self );
	track->position += multi->length;
	track->length += multi->length;
	free( multi );
	return 0;
}

/** Optimise for overlapping cuts from the same clip.
 *
 * \todo learn more about this
 * \public \memberof mlt_producer_s
 * \param self a producer
 * \return true if there was an error
 */

int mlt_producer_optimise( mlt_producer self )
{
	int error = 1;
	mlt_parser parser = mlt_parser_new( );
	if ( parser != NULL )
	{
		int i = 0, j = 0, k = 0;
		mlt_properties properties = mlt_parser_properties( parser );
		mlt_properties producers = mlt_properties_new( );
		mlt_deque stack = mlt_deque_init( );
		mlt_properties_set_data( properties, "producers", producers, 0, ( mlt_destructor )mlt_properties_close, NULL );
		mlt_properties_set_data( properties, "stack", stack, 0, ( mlt_destructor )mlt_deque_close, NULL );
		parser->on_start_producer = on_start_producer;
		parser->on_start_track = on_start_track;
		parser->on_end_track = on_end_track;
		parser->on_start_multitrack = on_start_multitrack;
		parser->on_end_multitrack = on_end_multitrack;
		push( parser, 0, 0, 0 );
		mlt_parser_start( parser, MLT_PRODUCER_SERVICE( self ) );
		free( pop( parser ) );
		for ( k = 0; k < mlt_properties_count( producers ); k ++ )
		{
			char *name = mlt_properties_get_name( producers, k );
			int count = 0;
			int clones = 0;
			int max_clones = 0;
			mlt_producer producer = mlt_properties_get_data_at( producers, k, &count );
			if ( producer != NULL && count > 1 )
			{
				clip_references *refs = mlt_properties_get_data( properties, name, &count );
				for ( i = 0; i < count; i ++ )
				{
					clones = 0;
					for ( j = i + 1; j < count; j ++ )
					{
						if ( intersect( &refs[ i ], &refs[ j ] ) )
						{
							clones ++;
							mlt_properties_set_int( MLT_PRODUCER_PROPERTIES( refs[ j ].cut ), "_clone", clones );
						}
					}
					if ( clones > max_clones )
						max_clones = clones;
				}

				for ( i = 0; i < count; i ++ )
				{
					mlt_producer cut = refs[ i ].cut;
					if ( mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( cut ), "_clone" ) == -1 )
						mlt_properties_set_int( MLT_PRODUCER_PROPERTIES( cut ), "_clone", 0 );
				}

				mlt_producer_set_clones( producer, max_clones );
			}
			else if ( producer != NULL )
			{
				clip_references *refs = mlt_properties_get_data( properties, name, &count );
				for ( i = 0; i < count; i ++ )
				{
					mlt_producer cut = refs[ i ].cut;
					mlt_properties_set_int( MLT_PRODUCER_PROPERTIES( cut ), "_clone", 0 );
				}
				mlt_producer_set_clones( producer, 0 );
			}
		}
		mlt_parser_close( parser );
	}
	return error;
}

/** Close the producer.
 *
 * Destroys the producer and deallocates its resources managed by its
 * properties list. This will call the close virtual function. Therefore, a
 * subclass that defines its own close function should set its virtual close
 * function to NULL prior to calling this to avoid circular calls.
 *
 * \public \memberof mlt_producer_s
 * \param self a producer
 */

void mlt_producer_close( mlt_producer self )
{
	if ( self != NULL && mlt_properties_dec_ref( MLT_PRODUCER_PROPERTIES( self ) ) <= 0 )
	{
		self->parent.close = NULL;

		if ( self->close != NULL )
		{
			self->close( self->close_object );
		}
		else
		{
			int destroy = mlt_producer_is_cut( self );

#if _MLT_PRODUCER_CHECKS_ == 1
			// Show debug info
			mlt_properties_debug( MLT_PRODUCER_PROPERTIES( self ), "Producer closing", stderr );
#endif

#ifdef _MLT_PRODUCER_CHECKS_
			// Show current stats - these should match when the app is closed
			mlt_log( MLT_PRODUCER_SERVICE( self ), MLT_LOG_DEBUG, "Producers created %d, destroyed %d\n", producers_created, ++producers_destroyed );
#endif

			mlt_service_close( &self->parent );

			if ( destroy )
				free( self );
		}
	}
}
