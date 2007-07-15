/*
 * mlt_producer.c -- abstraction for all producer services
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
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

#include "config.h"
#include "mlt_producer.h"
#include "mlt_factory.h"
#include "mlt_frame.h"
#include "mlt_parser.h"
#include "mlt_profile.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/** Forward references.
*/

static int producer_get_frame( mlt_service this, mlt_frame_ptr frame, int index );
static void mlt_producer_property_changed( mlt_service owner, mlt_producer this, char *name );
static void mlt_producer_service_changed( mlt_service owner, mlt_producer this );

//#define _MLT_PRODUCER_CHECKS_ 1

#ifdef _MLT_PRODUCER_CHECKS_
static int producers_created = 0;
static int producers_destroyed = 0;
#endif

/** Constructor
*/

int mlt_producer_init( mlt_producer this, void *child )
{
	// Check that we haven't received NULL
	int error = this == NULL;

	// Continue if no error
	if ( error == 0 )
	{
#ifdef _MLT_PRODUCER_CHECKS_
		producers_created ++;
#endif

		// Initialise the producer
		memset( this, 0, sizeof( struct mlt_producer_s ) );
	
		// Associate with the child
		this->child = child;

		// Initialise the service
		if ( mlt_service_init( &this->parent, this ) == 0 )
		{
			// The parent is the service
			mlt_service parent = &this->parent;
	
			// Define the parent close
			parent->close = ( mlt_destructor )mlt_producer_close;
			parent->close_object = this;

			// For convenience, we'll assume the close_object is this
			this->close_object = this;

			// Get the properties of the parent
			mlt_properties properties = MLT_SERVICE_PROPERTIES( parent );
	
			// Set the default properties
			mlt_properties_set( properties, "mlt_type", "mlt_producer" );
			mlt_properties_set_position( properties, "_position", 0.0 );
			mlt_properties_set_double( properties, "_frame", 0 );
			mlt_properties_set_double( properties, "fps", mlt_profile_fps( NULL ) );
			mlt_properties_set_int( properties, "frame_rate_num", mlt_profile_get()->frame_rate_num );
			mlt_properties_set_int( properties, "frame_rate_den", mlt_profile_get()->frame_rate_den );
			mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( NULL ) );
			mlt_properties_set_double( properties, "_speed", 1.0 );
			mlt_properties_set_position( properties, "in", 0 );
			mlt_properties_set_position( properties, "out", 14999 );
			mlt_properties_set_position( properties, "length", 15000 );
			mlt_properties_set( properties, "eof", "pause" );
			mlt_properties_set( properties, "resource", "<producer>" );

			// Override service get_frame
			parent->get_frame = producer_get_frame;

			mlt_events_listen( properties, this, "service-changed", ( mlt_listener )mlt_producer_service_changed );
			mlt_events_listen( properties, this, "property-changed", ( mlt_listener )mlt_producer_property_changed );
			mlt_events_register( properties, "producer-changed", NULL );
		}
	}

	return error;
}

/** Listener for property changes.
*/

static void mlt_producer_property_changed( mlt_service owner, mlt_producer this, char *name )
{
	if ( !strcmp( name, "in" ) || !strcmp( name, "out" ) || !strcmp( name, "length" ) )
		mlt_events_fire( MLT_PRODUCER_PROPERTIES( mlt_producer_cut_parent( this ) ), "producer-changed", NULL );
}

/** Listener for service changes.
*/

static void mlt_producer_service_changed( mlt_service owner, mlt_producer this )
{
	mlt_events_fire( MLT_PRODUCER_PROPERTIES( mlt_producer_cut_parent( this ) ), "producer-changed", NULL );
}

/** Create a new producer.
*/

mlt_producer mlt_producer_new( )
{
	mlt_producer this = malloc( sizeof( struct mlt_producer_s ) );
	mlt_producer_init( this, NULL );
	return this;
}

/** Determine if producer is a cut.
*/

int mlt_producer_is_cut( mlt_producer this )
{
	return mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( this ), "_cut" );
}

/** Determine if producer is a mix.
*/

int mlt_producer_is_mix( mlt_producer this )
{
	mlt_properties properties = this != NULL ? MLT_PRODUCER_PROPERTIES( this ) : NULL;
	mlt_tractor tractor = properties != NULL ? mlt_properties_get_data( properties, "mlt_mix", NULL ) : NULL;
	return tractor != NULL;
}

/** Determine if the producer is a blank [from a playlist].
*/

int mlt_producer_is_blank( mlt_producer this )
{
	return this == NULL || !strcmp( mlt_properties_get( MLT_PRODUCER_PROPERTIES( mlt_producer_cut_parent( this ) ), "resource" ), "blank" );
}

/** Obtain the parent producer.
*/

mlt_producer mlt_producer_cut_parent( mlt_producer this )
{
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );
	if ( mlt_producer_is_cut( this ) )
		return mlt_properties_get_data( properties, "_cut_parent", NULL );
	else
		return this;
}

/** Create a cut of this producer
*/

mlt_producer mlt_producer_cut( mlt_producer this, int in, int out )
{
	mlt_producer result = mlt_producer_new( );
	mlt_producer parent = mlt_producer_cut_parent( this );
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( result );
	mlt_properties parent_props = MLT_PRODUCER_PROPERTIES( parent );

	mlt_events_block( MLT_PRODUCER_PROPERTIES( result ), MLT_PRODUCER_PROPERTIES( result ) );
	// Special case - allow for a cut of the entire producer (this will squeeze all other cuts to 0)
	if ( in <= 0 )
		in = 0;
	if ( ( out < 0 || out >= mlt_producer_get_length( parent ) ) && !mlt_producer_is_blank( this ) )
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
*/

mlt_service mlt_producer_service( mlt_producer this )
{
	return this != NULL ? &this->parent : NULL;
}

/** Get the producer properties.
*/

mlt_properties mlt_producer_properties( mlt_producer this )
{
	return MLT_SERVICE_PROPERTIES( &this->parent );
}

/** Seek to a specified position.
*/

int mlt_producer_seek( mlt_producer this, mlt_position position )
{
	// Determine eof handling
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );
	char *eof = mlt_properties_get( properties, "eof" );
	int use_points = 1 - mlt_properties_get_int( properties, "ignore_points" );

	// Recursive behaviour for cuts - repositions parent and then repositions cut
	// hence no return on this condition
	if ( mlt_producer_is_cut( this ) )
		mlt_producer_seek( mlt_producer_cut_parent( this ), position + mlt_producer_get_in( this ) );

	// Check bounds
	if ( position < 0 || mlt_producer_get_playtime( this ) == 0 )
	{
		position = 0;
	}
	else if ( use_points && ( eof == NULL || !strcmp( eof, "pause" ) ) && position >= mlt_producer_get_playtime( this ) )
	{
		mlt_producer_set_speed( this, 0 );
		position = mlt_producer_get_playtime( this ) - 1;
	}
	else if ( use_points && !strcmp( eof, "loop" ) && position >= mlt_producer_get_playtime( this ) )
	{
		position = (int)position % (int)mlt_producer_get_playtime( this );
	}

	// Set the position
	mlt_properties_set_position( MLT_PRODUCER_PROPERTIES( this ), "_position", position );

	// Calculate the absolute frame
	mlt_properties_set_position( MLT_PRODUCER_PROPERTIES( this ), "_frame", use_points * mlt_producer_get_in( this ) + position );

	return 0;
}

/** Get the current position (relative to in point).
*/

mlt_position mlt_producer_position( mlt_producer this )
{
	return mlt_properties_get_position( MLT_PRODUCER_PROPERTIES( this ), "_position" );
}

/** Get the current position (relative to start of producer).
*/

mlt_position mlt_producer_frame( mlt_producer this )
{
	return mlt_properties_get_position( MLT_PRODUCER_PROPERTIES( this ), "_frame" );
}

/** Set the playing speed.
*/

int mlt_producer_set_speed( mlt_producer this, double speed )
{
	return mlt_properties_set_double( MLT_PRODUCER_PROPERTIES( this ), "_speed", speed );
}

/** Get the playing speed.
*/

double mlt_producer_get_speed( mlt_producer this )
{
	return mlt_properties_get_double( MLT_PRODUCER_PROPERTIES( this ), "_speed" );
}

/** Get the frames per second.
*/

double mlt_producer_get_fps( mlt_producer this )
{
	return mlt_properties_get_double( MLT_PRODUCER_PROPERTIES( this ), "fps" );
}

/** Set the in and out points.
*/

int mlt_producer_set_in_and_out( mlt_producer this, mlt_position in, mlt_position out )
{
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

	// Correct ins and outs if necessary
	if ( in < 0 )
		in = 0;
	else if ( in >= mlt_producer_get_length( this ) )
		in = mlt_producer_get_length( this ) - 1;

	if ( out < 0 )
		out = 0;
	else if ( out >= mlt_producer_get_length( this ) && !mlt_producer_is_blank( this ) )
		out = mlt_producer_get_length( this ) - 1;
	else if ( out >= mlt_producer_get_length( this ) && mlt_producer_is_blank( this ) )
		mlt_properties_set_position( MLT_PRODUCER_PROPERTIES( this ), "length", out + 1 );

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
  	Essentially, all 0 length cuts should be immediately removed by containers.
*/

int mlt_producer_clear( mlt_producer this )
{
	if ( this != NULL )
	{
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );
		mlt_events_block( properties, properties );
		mlt_properties_set_position( properties, "in", 0 );
		mlt_events_unblock( properties, properties );
		mlt_properties_set_position( properties, "out", -1 );
	}
	return 0;
}

/** Get the in point.
*/

mlt_position mlt_producer_get_in( mlt_producer this )
{
	return mlt_properties_get_position( MLT_PRODUCER_PROPERTIES( this ), "in" );
}

/** Get the out point.
*/

mlt_position mlt_producer_get_out( mlt_producer this )
{
	return mlt_properties_get_position( MLT_PRODUCER_PROPERTIES( this ), "out" );
}

/** Get the total play time.
*/

mlt_position mlt_producer_get_playtime( mlt_producer this )
{
	return mlt_producer_get_out( this ) - mlt_producer_get_in( this ) + 1;
}

/** Get the total length of the producer.
*/

mlt_position mlt_producer_get_length( mlt_producer this )
{
	return mlt_properties_get_position( MLT_PRODUCER_PROPERTIES( this ), "length" );
}

/** Prepare for next frame.
*/

void mlt_producer_prepare_next( mlt_producer this )
{
	if ( mlt_producer_get_speed( this ) != 0 )
		mlt_producer_seek( this, mlt_producer_position( this ) + mlt_producer_get_speed( this ) );
}

/** Get a frame.
*/

static int producer_get_frame( mlt_service service, mlt_frame_ptr frame, int index )
{
	int result = 1;
	mlt_producer this = service != NULL ? service->child : NULL;

	if ( this != NULL && !mlt_producer_is_cut( this ) )
	{
		// Get the properties of this producer
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

		// Determine eof handling
		char *eof = mlt_properties_get( MLT_PRODUCER_PROPERTIES( this ), "eof" );

		// Get the speed of the producer
		double speed = mlt_producer_get_speed( this );

		// We need to use the clone if it's specified
		mlt_producer clone = mlt_properties_get_data( properties, "use_clone", NULL );

		// If no clone is specified, use this
		clone = clone == NULL ? this : clone;

		// A properly instatiated producer will have a get_frame method...
		if ( this->get_frame == NULL || ( !strcmp( eof, "continue" ) && mlt_producer_position( this ) > mlt_producer_get_out( this ) ) )
		{
			// Generate a test frame
			*frame = mlt_frame_init( );

			// Set the position
			result = mlt_frame_set_position( *frame, mlt_producer_position( this ) );

			// Mark as a test card
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( *frame ), "test_image", 1 );
			mlt_properties_set_int( MLT_FRAME_PROPERTIES( *frame ), "test_audio", 1 );

			// Calculate the next position
			mlt_producer_prepare_next( this );
		}
		else
		{
			// Get the frame from the implementation
			result = this->get_frame( clone, frame, index );
		}

		// Copy the fps and speed of the producer onto the frame
		properties = MLT_FRAME_PROPERTIES( *frame );
		mlt_properties_set_double( properties, "_speed", speed );
		mlt_properties_set_double( properties, "fps", mlt_producer_get_fps( this ) );
		mlt_properties_set_int( properties, "test_audio", mlt_frame_is_test_audio( *frame ) );
		mlt_properties_set_int( properties, "test_image", mlt_frame_is_test_card( *frame ) );
		if ( mlt_properties_get_data( properties, "_producer", NULL ) == NULL )
			mlt_properties_set_data( properties, "_producer", service, 0, NULL, NULL );
	}
	else if ( this != NULL )
	{
		// Get the speed of the cut
		double speed = mlt_producer_get_speed( this );

		// Get the parent of this cut
		mlt_producer parent = mlt_producer_cut_parent( this );

		// Get the properties of the parent
		mlt_properties parent_properties = MLT_PRODUCER_PROPERTIES( parent );

		// Get the properties of the cut
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

		// Determine the clone index
		int clone_index = mlt_properties_get_int( properties, "_clone" );

		// Determine the clone to use
		mlt_producer clone = this;

		if ( clone_index > 0 )
		{
			char key[ 25 ];
			sprintf( key, "_clone.%d", clone_index - 1 );
			clone = mlt_properties_get_data( MLT_PRODUCER_PROPERTIES( mlt_producer_cut_parent( this ) ), key, NULL );
			if ( clone == NULL ) fprintf( stderr, "requested clone doesn't exist %d\n", clone_index );
			clone = clone == NULL ? this : clone;
		}
		else
		{
			clone = parent;
		}

		// We need to seek to the correct position in the clone
		mlt_producer_seek( clone, mlt_producer_get_in( this ) + mlt_properties_get_int( properties, "_position" ) );

		// Assign the clone property to the parent
		mlt_properties_set_data( parent_properties, "use_clone", clone, 0, NULL, NULL );

		// Now get the frame from the parents service
		result = mlt_service_get_frame( MLT_PRODUCER_SERVICE( parent ), frame, index );

		// We're done with the clone now
		mlt_properties_set_data( parent_properties, "use_clone", NULL, 0, NULL, NULL );

		// This is useful and required by always_active transitions to determine in/out points of the cut
		if ( mlt_properties_get_data( MLT_FRAME_PROPERTIES( *frame ), "_producer", NULL ) == MLT_PRODUCER_SERVICE( parent ) )
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( *frame ), "_producer", this, 0, NULL, NULL );

		mlt_properties_set_double( MLT_FRAME_PROPERTIES( *frame ), "_speed", speed );
		mlt_producer_prepare_next( this );
	}
	else
	{
		*frame = mlt_frame_init( );
		result = 0;
	}

	// Pass on all meta properties from the producer/cut on to the frame
	if ( *frame != NULL && this != NULL )
	{
		int i = 0;
		mlt_properties p_props = MLT_PRODUCER_PROPERTIES( this );
		mlt_properties f_props = MLT_FRAME_PROPERTIES( *frame );
		int count = mlt_properties_count( p_props );
		for ( i = 0; i < count; i ++ )
		{
			char *name = mlt_properties_get_name( p_props, i );
			if ( !strncmp( name, "meta.", 5 ) )
				mlt_properties_set( f_props, name, mlt_properties_get( p_props, name ) );
			else if ( !strncmp( name, "set.", 4 ) )
				mlt_properties_set( f_props, name + 4, mlt_properties_get( p_props, name ) );
		}
	}

	return result;
}

/** Attach a filter.
*/

int mlt_producer_attach( mlt_producer this, mlt_filter filter )
{
	return mlt_service_attach( MLT_PRODUCER_SERVICE( this ), filter );
}

/** Detach a filter.
*/

int mlt_producer_detach( mlt_producer this, mlt_filter filter )
{
	return mlt_service_detach( MLT_PRODUCER_SERVICE( this ), filter );
}

/** Retrieve a filter.
*/

mlt_filter mlt_producer_filter( mlt_producer this, int index )
{
	return mlt_service_filter( MLT_PRODUCER_SERVICE( this ), index );
}

/** Clone this producer.
*/

static mlt_producer mlt_producer_clone( mlt_producer this )
{
	mlt_producer clone = NULL;
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );
	char *resource = mlt_properties_get( properties, "resource" );
	char *service = mlt_properties_get( properties, "mlt_service" );

	mlt_events_block( mlt_factory_event_object( ), mlt_factory_event_object( ) );

	if ( service != NULL )
		clone = mlt_factory_producer( service, resource );

	if ( clone == NULL && resource != NULL )
		clone = mlt_factory_producer( "fezzik", resource );

	if ( clone != NULL )
		mlt_properties_inherit( MLT_PRODUCER_PROPERTIES( clone ), properties );

	mlt_events_unblock( mlt_factory_event_object( ), mlt_factory_event_object( ) );

	return clone;
}

/** Create clones.
*/

static void mlt_producer_set_clones( mlt_producer this, int clones )
{
	mlt_producer parent = mlt_producer_cut_parent( this );
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

/** Optimise for overlapping cuts from the same clip.
*/

typedef struct 
{
	int multitrack;
	int track;
	int position;
	int length;
	int offset;
}
track_info;

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

static int push( mlt_parser this, int multitrack, int track, int position )
{
	mlt_properties properties = mlt_parser_properties( this );
	mlt_deque stack = mlt_properties_get_data( properties, "stack", NULL );
	track_info *info = malloc( sizeof( track_info ) );
	info->multitrack = multitrack;
	info->track = track;
	info->position = position;
	info->length = 0;
	info->offset = 0;
	return mlt_deque_push_back( stack, info );
}

static track_info *pop( mlt_parser this )
{
	mlt_properties properties = mlt_parser_properties( this );
	mlt_deque stack = mlt_properties_get_data( properties, "stack", NULL );
	return mlt_deque_pop_back( stack );
}

static track_info *peek( mlt_parser this )
{
	mlt_properties properties = mlt_parser_properties( this );
	mlt_deque stack = mlt_properties_get_data( properties, "stack", NULL );
	return mlt_deque_peek_back( stack );
}

static int on_start_multitrack( mlt_parser this, mlt_multitrack object )
{
	track_info *info = peek( this );
	return push( this, info->multitrack ++, info->track, info->position );
}

static int on_start_track( mlt_parser this )
{
	track_info *info = peek( this );
	info->position -= info->offset;
	info->length -= info->offset;
	return push( this, info->multitrack, info->track ++, info->position );
}

static int on_start_producer( mlt_parser this, mlt_producer object )
{
	mlt_properties properties = mlt_parser_properties( this );
	mlt_properties producers = mlt_properties_get_data( properties, "producers", NULL );
	mlt_producer parent = mlt_producer_cut_parent( object );
	if ( mlt_service_identify( ( mlt_service )mlt_producer_cut_parent( object ) ) == producer_type && mlt_producer_is_cut( object ) )
	{
		int ref_count = 0;
		clip_references *old_refs = NULL;
		clip_references *refs = NULL;
		char key[ 50 ];
		int count = 0;
		track_info *info = peek( this );
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

static int on_end_track( mlt_parser this )
{
	track_info *track = pop( this );
	track_info *multi = peek( this );
	multi->length += track->length;
	multi->position += track->length;
	multi->offset = track->length;
	free( track );
	return 0;
}

static int on_end_multitrack( mlt_parser this, mlt_multitrack object )
{
	track_info *multi = pop( this );
	track_info *track = peek( this );
	track->position += multi->length;
	track->length += multi->length;
	free( multi );
	return 0;
}

int mlt_producer_optimise( mlt_producer this )
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
		mlt_parser_start( parser, MLT_PRODUCER_SERVICE( this ) );
		free( pop( parser ) );
		for ( k = 0; k < mlt_properties_count( producers ); k ++ )
		{
			char *name = mlt_properties_get_name( producers, k );
			int count = 0;
			int clones = 0;
			int max_clones = 0;
			mlt_producer producer = mlt_properties_get_data( producers, name, &count );
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
*/

void mlt_producer_close( mlt_producer this )
{
	if ( this != NULL && mlt_properties_dec_ref( MLT_PRODUCER_PROPERTIES( this ) ) <= 0 )
	{
		this->parent.close = NULL;

		if ( this->close != NULL )
		{
			this->close( this->close_object );
		}
		else
		{
			int destroy = mlt_producer_is_cut( this );

#if _MLT_PRODUCER_CHECKS_ == 1
			// Show debug info
			mlt_properties_debug( MLT_PRODUCER_PROPERTIES( this ), "Producer closing", stderr );
#endif

#ifdef _MLT_PRODUCER_CHECKS_
			// Increment destroyed count
			producers_destroyed ++;

			// Show current stats - these should match when the app is closed
			fprintf( stderr, "Producers created %d, destroyed %d\n", producers_created, producers_destroyed );
#endif

			mlt_service_close( &this->parent );

			if ( destroy )
				free( this );
		}
	}
}
