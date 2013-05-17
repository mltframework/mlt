/*
 * mlt_animation.c -- provides the property animation API
 * Copyright (C) 2004-2013 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include "mlt_animation.h"
#include "mlt_tokeniser.h"
#include "mlt_profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct animation_node_s *animation_node;
struct animation_node_s
{
	struct mlt_animation_item_s item;
	animation_node next, prev;
};

struct mlt_animation_s
{
	char *data;
	int length;
	double fps;
	locale_t locale;
	animation_node nodes;
};

// Create a new geometry structure
mlt_animation mlt_animation_new( )
{
	mlt_animation self = calloc( 1, sizeof( *self ) );
	return self;
}

void mlt_animation_interpolate( mlt_animation self )
{
	// Parse all items to ensure non-keyframes are calculated correctly.
	if ( self->nodes )
	{
		animation_node current = self->nodes;
		while ( current )
		{
			if ( !current->item.is_key )
			{
				animation_node prev = current->prev;
				animation_node next = current->next;

				while ( prev && !prev->item.is_key ) prev = prev->prev;
				while ( next && !next->item.is_key ) next = next->next;

				if ( !prev )
					current->item.is_key = 1;
				mlt_property_interpolate( current->item.property,
					prev->item.property, next->item.property,
					current->item.frame - prev->item.frame,
					next->item.frame - prev->item.frame,
					self->fps, self->locale );
			}

			// Move to the next item
			current = current->next;
		}
	}
}

static int mlt_animation_drop( mlt_animation self, animation_node node )
{
	if ( node == self->nodes )
	{
		self->nodes = node->next;
		if ( self->nodes ) {
			self->nodes->prev = NULL;
			self->nodes->item.is_key = 1;
		}
	}
	else if ( node->next && node->prev )
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	else if ( node->next )
	{
		node->next->prev = node->prev;
	}
	else if ( node->prev )
	{
		node->prev->next = node->next;
	}
	mlt_property_close( node->item.property );
	free( node );

	return 0;
}

static void mlt_animation_clean( mlt_animation self )
{
	if ( self->data )
		free( self->data );
	self->data = NULL;
	while ( self->nodes )
		mlt_animation_drop( self, self->nodes );
}

int mlt_animation_parse(mlt_animation self, const char *data, int length, double fps, locale_t locale )
{
	int error = 0;
	int i = 0;
	struct mlt_animation_item_s item;
	mlt_tokeniser tokens = mlt_tokeniser_init( );

	// Clean the existing geometry
	mlt_animation_clean( self );

	// Update the info on the data
	if ( data )
		self->data = strdup( data );
	self->length = length;
	self->fps = fps;
	self->locale = locale;
	item.property = mlt_property_init();

	// Tokenise
	if ( data )
		mlt_tokeniser_parse_new( tokens, (char*) data, ";" );

	// Iterate through each token
	for ( i = 0; i < mlt_tokeniser_count( tokens ); i++ )
	{
		char *value = mlt_tokeniser_get_string( tokens, i );

		// If no data in keyframe, drop it (trailing semicolon)
		if ( !value || !strcmp( value, "" ) )
			continue;

		// Reset item
		item.frame = item.is_key = 0;

		// Now parse the item
		mlt_animation_parse_item( self, &item, value );

		// Now insert into place
		mlt_animation_insert( self, &item );
	}
	mlt_animation_interpolate( self );

	// Cleanup
	mlt_tokeniser_close( tokens );
	mlt_property_close( item.property );

	return error;
}

// Conditionally refresh in case of a change
int mlt_animation_refresh(mlt_animation self, const char *data, int length)
{
	if ( ( length != self->length )|| ( data && ( !self->data || strcmp( data, self->data ) ) ) )
		return mlt_animation_parse( self, data, length, self->fps, self->locale );
	return 0;
}

int mlt_animation_get_length( mlt_animation self )
{
	if ( self )
		return self->length;
	else
		return 0;
}

void mlt_animation_set_length( mlt_animation self, int length )
{
	if ( self )
		self->length = length;
}

int mlt_animation_parse_item( mlt_animation self, mlt_animation_item item, const char *value )
{
	int error = 0;

	if ( value && strcmp( value, "" ) )
	{
		// Determine if a position has been specified
		if ( strchr( value, '=' ) )
		{
			double temp;
			char *p = NULL;
#if defined(__GLIBC__) || defined(__DARWIN__)
			if ( self->locale )
				temp = strtod_l( value, &p, self->locale );
			else
#endif
			temp = strtod( value, &p );
			// If p == value then it is likely a time clock or time code.
			if ( temp > -1.0 && temp < 1.0 && p != value )
			{
				// Parse a relative time (-1, 1).
				item->frame = temp * self->length;
			}
			else
			{
				// Parse an absolute time value.
				mlt_property_set_string( item->property, value );
				item->frame = mlt_property_get_int( item->property, self->fps, self->locale );
			}
			value = strchr( value, '=' ) + 1;

			// TODO the character preceeding the equal sign indicates method of interpolation.
		}

		// Special case - frame < 0
		if ( item->frame < 0 )
			item->frame += self->length;

		// Obtain the current value at this position - this allows new
		// frames to be created which don't specify all values
		mlt_animation_get_item( self, item, item->frame );

		// Set remainder of string as item value.
		mlt_property_set_string( item->property, value );
		item->is_key = 1;
	}
	else
	{
		error = 1;
	}

	return error;
}

// Fetch a geometry item for an absolute position
int mlt_animation_get_item( mlt_animation self, mlt_animation_item item, int position )
{
	int error = 0;
	// Need to find the nearest keyframe to the position specifed
	animation_node node = self->nodes;

	// Iterate through the keyframes until we reach last or have
	while ( node && node->next && position >= node->next->item.frame )
		node = node->next;

	if ( node )
	{
		// Position is before the first keyframe.
		if ( position < node->item.frame )
		{
			item->is_key = 0;
			mlt_property_pass( item->property, node->item.property );
		}
		// Item exists.
		else if ( position == node->item.frame )
		{
			item->is_key = node->item.is_key;
			mlt_property_pass( item->property, node->item.property );
		}
		// Position is after the last keyframe.
		else if ( !node->next )
		{
			item->is_key = 0;
			mlt_property_pass( item->property, node->item.property );
		}
		// Interpolation needed.
		else
		{
			item->is_key = 0;
			mlt_property_interpolate( item->property, node->item.property, node->next->item.property,
				position - node->item.frame, node->next->item.frame - node->item.frame,
				self->fps, self->locale );
		}
	}
	else
	{
		item->frame = item->is_key = 0;
		error = 1;
	}
	item->frame = position;

	return error;
}

// Specify an animation item at an absolute position
int mlt_animation_insert( mlt_animation self, mlt_animation_item item )
{
	int error = 0;
	animation_node node = calloc( 1, sizeof( *node ) );
	node->item.frame = item->frame;
	node->item.is_key = 1;
	node->item.property = mlt_property_init();
	mlt_property_pass( node->item.property, item->property );

	// Determine if we need to insert or append to the list, or if it's a new list
	if ( self->nodes )
	{
		// Get the first item
		animation_node current = self->nodes;

		// Locate an existing nearby item
		while ( current->next && item->frame > current->item.frame )
			current = current->next;

		if ( item->frame < current->item.frame )
		{
			if ( current == self->nodes )
				self->nodes = node;
			if ( current->prev )
				current->prev->next = node;
			node->next = current;
			node->prev = current->prev;
			current->prev = node;
		}
		else if ( item->frame > current->item.frame )
		{
			if ( current->next )
				current->next->prev = node;
			node->next = current->next;
			node->prev = current;
			current->next = node;
		}
		else
		{
			// Update matching node.
			current->item.frame = item->frame;
			current->item.is_key = 1;
			mlt_property_close( current->item.property );
			current->item.property = node->item.property;
			free( node );
		}
	}
	else
	{
		// Set the first item
		self->nodes = node;
	}

	return error;
}

// Remove the keyframe at the specified position
int mlt_animation_remove( mlt_animation self, int position )
{
	int error = 1;
	animation_node node = self->nodes;

	while ( node && position != node->item.frame )
		node = node->next;

	if ( node && position == node->item.frame )
		error = mlt_animation_drop( self, node );

	return error;
}

// Get the keyfame at the position or the next following
int mlt_animation_next_key( mlt_animation self, mlt_animation_item item, int position )
{
	animation_node node = self->nodes;

	while ( node && position > node->item.frame )
		node = node->next;

	if ( node )
	{
		item->frame = node->item.frame;
		item->is_key = node->item.is_key;
		mlt_property_pass( item->property, node->item.property );
	}

	return ( node == NULL );
}

// Get the keyframe at the position or the previous key
int mlt_animation_prev_key( mlt_animation self, mlt_animation_item item, int position )
{
	animation_node node = self->nodes;

	while ( node && node->next && position >= node->next->item.frame )
		node = node->next;

	if ( node )
	{
		item->frame = node->item.frame;
		item->is_key = node->item.is_key;
		mlt_property_pass( item->property, node->item.property );
	}

	return ( node == NULL );
}

char *mlt_animation_serialize_cut( mlt_animation self, int in, int out )
{
	struct mlt_animation_item_s item;
	char *ret = malloc( 1000 );
	size_t used = 0;
	size_t size = 1000;

	item.property = mlt_property_init();
	if ( in == -1 )
		in = 0;
	if ( out == -1 )
		out = mlt_animation_get_length( self );

	if ( ret )
	{
		strcpy( ret, "" );

		item.frame = in;

		while ( 1 )
		{
			size_t item_len = 0;

			// If it's the first frame, then it's not necessarily a key
			if ( item.frame == in )
			{
				if ( mlt_animation_get_item( self, &item, item.frame ) )
					break;

				// If the first keyframe is larger than the current position
				// then do nothing here
				if ( self->nodes->item.frame > item.frame )
				{
					item.frame ++;
					continue;
				}

				// To ensure correct seeding
				item.is_key = 1;
			}
			// Typically, we move from keyframe to keyframe
			else if ( item.frame < out )
			{
				if ( mlt_animation_next_key( self, &item, item.frame ) )
					break;

				// Special case - crop at the out point
				if ( item.frame > out )
					mlt_animation_get_item( self, &item, out );
			}
			// We've handled the last keyframe
			else
			{
				break;
			}

			// Determine length of string to be appended.
			if ( item.frame - in != 0 )
				item_len += 20;
			if ( item.is_key )
				item_len += strlen( mlt_property_get_string_l( item.property, self->locale ) );

			// Reallocate return string to be long enough.
			while ( used + item_len + 2 > size ) // +2 for ';' and NULL
			{
				size += 1000;
				ret = realloc( ret, size );
			}

			// Append item delimiter (;) if needed.
			if ( ret && used > 0 )
			{
				used ++;
				strcat( ret, ";" );
			}
			if ( ret )
			{
				// Append keyframe time and keyframe/value delimiter (=).
				if ( item.frame - in != 0 )
					sprintf( ret + used, "%d=", item.frame - in );
				// Append item value.
				if ( item.is_key )
					strcat( ret, mlt_property_get_string_l( item.property, self->locale ) );
				used = strlen( ret );
			}
			item.frame ++;
		}
	}
	mlt_property_close( item.property );

	return ret;
}

// Serialise the current geometry
char *mlt_animation_serialize( mlt_animation self )
{
	char *ret = mlt_animation_serialize_cut( self, 0, self->length );
	if ( ret )
	{
		if ( self->data )
			free( self->data );
		self->data = ret;
	}
	return strdup( ret );
}

// Close the geometry
void mlt_animation_close( mlt_animation self )
{
	if ( self )
	{
		mlt_animation_clean( self );
		free( self );
	}
}
