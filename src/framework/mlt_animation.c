/**
 * \file mlt_animation.c
 * \brief Property Animation class definition
 * \see mlt_animation_s
 *
 * Copyright (C) 2004-2014 Meltytech, LLC
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

#include "mlt_animation.h"
#include "mlt_tokeniser.h"
#include "mlt_profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** \brief animation list node pointer */
typedef struct animation_node_s *animation_node;
/** \brief private animation list node */
struct animation_node_s
{
	struct mlt_animation_item_s item;
	animation_node next, prev;
};

/** \brief Property Animation class
 *
 * This is the animation engine for a Property object. It is dependent upon
 * the mlt_property API and used by the various mlt_property_anim_* functions.
 */

struct mlt_animation_s
{
	char *data;           /**< the string representing the animation */
	int length;           /**< the maximum number of frames to use when interpreting negative keyframe positions */
	double fps;           /**< framerate to use when converting time clock strings to frame units */
	locale_t locale;      /**< pointer to a locale to use when converting strings to numeric values */
	animation_node nodes; /**< a linked list of keyframes (and possibly non-keyframe values) */
};

/** Create a new animation object.
 *
 * \public \memberof mlt_animation_s
 * \return an animation object
 */

mlt_animation mlt_animation_new( )
{
	mlt_animation self = calloc( 1, sizeof( *self ) );
	return self;
}

/** Re-interpolate non-keyframe nodess after a series of insertions or removals.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 */

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
				double progress;
				mlt_property points[4];
				animation_node prev = current->prev;
				animation_node next = current->next;

				while ( prev && !prev->item.is_key ) prev = prev->prev;
				while ( next && !next->item.is_key ) next = next->next;

				if ( !prev ) {
					current->item.is_key = 1;
					prev = current;
				}
				if ( !next ) {
					next = current;
				}
				points[0] = prev->prev? prev->prev->item.property : prev->item.property;
				points[1] = prev->item.property;
				points[2] = next->item.property;
				points[3] = next->next? next->next->item.property : next->item.property;
				progress = current->item.frame - prev->item.frame;
				progress /= next->item.frame - prev->item.frame;
				mlt_property_interpolate( current->item.property, points, progress,
					self->fps, self->locale, current->item.keyframe_type );
			}

			// Move to the next item
			current = current->next;
		}
	}
}

/** Remove a node from the linked list.
 *
 * \private \memberof mlt_animation_s
 * \param self an animation
 * \param node the node to remove
 * \return false
 */

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

/** Reset an animation and free all strings and properties.
 *
 * \private \memberof mlt_animation_s
 * \param self an animation
 */

static void mlt_animation_clean( mlt_animation self )
{
	free( self->data );
	self->data = NULL;
	while ( self->nodes )
		mlt_animation_drop( self, self->nodes );
}

/** Parse a string representing an animation.
 *
 * A semicolon is the delimiter between keyframe=value items in the string.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param data the string representing an animation
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \param fps the framerate to use when evaluating time strings
 * \param locale the locale to use when converting strings to numbers
 * \return true if there was an error
 */

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

/** Conditionally refresh the animation if it is modified.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param data the string representing an animation
 * \param length the maximum number of frames when interpreting negative keyframe times,
 *  <=0 if you don't care or need that
 * \return true if there was an error
 */

int mlt_animation_refresh( mlt_animation self, const char *data, int length )
{
	if ( ( length != self->length )|| ( data && ( !self->data || strcmp( data, self->data ) ) ) )
		return mlt_animation_parse( self, data, length, self->fps, self->locale );
	return 0;
}

/** Get the length of the animation.
 *
 * If the animation was initialized with a zero or negative value, then this
 * gets the maximum frame number from animation's list of nodes.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \return the number of frames
 */

int mlt_animation_get_length( mlt_animation self )
{
	int length = 0;
	if ( self ) {
		if ( self->length > 0 ) {
			length = self->length;
		}
		else if ( self->nodes ) {
			animation_node node = self->nodes;
			while ( node ) {
				if ( node->item.frame > length )
					length = node->item.frame;
				node = node->next;
			}
		}
	}
	return length;
}

/** Set the length of the animation.
 *
 * The length is used for interpreting negative keyframe positions as relative
 * to the length. It is also used when serializing an animation as a string.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param length the length of the animation in frame units
 */

void mlt_animation_set_length( mlt_animation self, int length )
{
	if ( self )
		self->length = length;
}

/** Parse a string representing an animation keyframe=value.
 *
 * This function does not affect the animation itself! But it will use some state
 * of the animation for the parsing (e.g. fps, locale).
 * It parses into a mlt_animation_item that you provide.
 * \p item->frame should be specified if the string does not have an equal sign and time field.
 * If an exclamation point (!) or vertical bar (|) character preceeds the equal sign, then
 * the keyframe interpolation is set to discrete. If a tilde (~) preceeds the equal sign,
 * then the keyframe interpolation is set to smooth (spline).
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an already allocated animation item
 * \param value the string representing an animation
 * \return true if there was an error
 */

int mlt_animation_parse_item( mlt_animation self, mlt_animation_item item, const char *value )
{
	int error = 0;

	if ( value && strcmp( value, "" ) )
	{
		// Determine if a position has been specified
		if ( strchr( value, '=' ) )
		{
			// Parse an absolute time value.
			// Null terminate the string at the equal sign to prevent interpreting
			// a colon in the part to the right of the equal sign as indicative of a
			// a time value string.
			char *s = strdup( value );
			char *p = strchr( s, '=' );
			p[0] = '\0';
			mlt_property_set_string( item->property, s );
			item->frame = mlt_property_get_int( item->property, self->fps, self->locale );
			free( s );

			// The character preceeding the equal sign indicates interpolation method.
			p = strchr( value, '=' ) - 1;
			if ( p[0] == '|' || p[0] == '!' )
				item->keyframe_type = mlt_keyframe_discrete;
			else if ( p[0] == '~' )
				item->keyframe_type = mlt_keyframe_smooth;
			else
				item->keyframe_type = mlt_keyframe_linear;
			value = &p[2];
		}

		// Special case - frame < 0
		if ( item->frame < 0 )
			item->frame += mlt_animation_get_length( self );

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

/** Load an animation item for an absolute position.
 *
 * This performs interpolation if there is no keyframe at the \p position.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an already allocated animation item that will be filled in
 * \param position the frame number for the point in time
 * \return true if there was an error
 */

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
		item->keyframe_type = node->item.keyframe_type;

		// Position is before the first keyframe.
		if ( position < node->item.frame )
		{
			item->is_key = 0;
			if ( item->property )
				mlt_property_pass( item->property, node->item.property );
		}
		// Item exists.
		else if ( position == node->item.frame )
		{
			item->is_key = node->item.is_key;
			if ( item->property )
				mlt_property_pass( item->property, node->item.property );
		}
		// Position is after the last keyframe.
		else if ( !node->next )
		{
			item->is_key = 0;
			if ( item->property )
				mlt_property_pass( item->property, node->item.property );
		}
		// Interpolation needed.
		else
		{
			if ( item->property )
			{
				double progress;
				mlt_property points[4];
				points[0] = node->prev? node->prev->item.property : node->item.property;
				points[1] = node->item.property;
				points[2] = node->next->item.property;
				points[3] = node->next->next? node->next->next->item.property : node->next->item.property;
				progress = position - node->item.frame;
				progress /= node->next->item.frame - node->item.frame;
				mlt_property_interpolate( item->property, points, progress,
					self->fps, self->locale, item->keyframe_type );
			}
			item->is_key = 0;
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

/** Insert an animation item.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an animation item
 * \return true if there was an error
 * \see mlt_animation_parse_item
 */

int mlt_animation_insert( mlt_animation self, mlt_animation_item item )
{
	int error = 0;
	animation_node node = calloc( 1, sizeof( *node ) );
	node->item.frame = item->frame;
	node->item.is_key = 1;
	node->item.keyframe_type = item->keyframe_type;
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
			current->item.keyframe_type = item->keyframe_type;
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

/** Remove the keyframe at the specified position.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param position the frame number of the animation node to remove
 * \return true if there was an error
 */

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

/** Get the keyfame at the position or the next following.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an already allocated animation item which will be updated
 * \param position the frame number at which to start looking for the next animation node
 * \return true if there was an error
 */

int mlt_animation_next_key( mlt_animation self, mlt_animation_item item, int position )
{
	animation_node node = self->nodes;

	while ( node && position > node->item.frame )
		node = node->next;

	if ( node )
	{
		item->frame = node->item.frame;
		item->is_key = node->item.is_key;
		item->keyframe_type = node->item.keyframe_type;
		if ( item->property )
			mlt_property_pass( item->property, node->item.property );
	}

	return ( node == NULL );
}

/** Get the keyfame at the position or the next preceeding.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an already allocated animation item which will be updated
 * \param position the frame number at which to start looking for the previous animation node
 * \return true if there was an error
 */

int mlt_animation_prev_key( mlt_animation self, mlt_animation_item item, int position )
{
	animation_node node = self->nodes;

	while ( node && node->next && position >= node->next->item.frame )
		node = node->next;

	if ( node )
	{
		item->frame = node->item.frame;
		item->is_key = node->item.is_key;
		item->keyframe_type = node->item.keyframe_type;
		if ( item->property )
			mlt_property_pass( item->property, node->item.property );
	}

	return ( node == NULL );
}

/** Serialize a cut of the animation.
 *
 * The caller is responsible for free-ing the returned string.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param in the frame at which to start serializing animation nodes
 * \param out the frame at which to stop serializing nodes
 * \return a string representing the animation
 */

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
			else if ( item.frame <= out )
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
				const char *s;
				switch (item.keyframe_type) {
				case mlt_keyframe_discrete:
					s = "|";
					break;
				case mlt_keyframe_smooth:
					s = "~";
					break;
				default:
					s = "";
					break;
				}
				sprintf( ret + used, "%d%s=", item.frame - in, s );

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

/** Serialize the animation.
 *
 * The caller is responsible for free-ing the returned string.
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \return a string representing the animation
 */

char *mlt_animation_serialize( mlt_animation self )
{
	char *ret = mlt_animation_serialize_cut( self, -1, -1 );
	if ( ret )
	{
		free( self->data );
		self->data = ret;
		ret = strdup( ret );
	}
	return ret;
}

/** Get the number of keyframes.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \return the number of keyframes or -1 on error
 */

int mlt_animation_key_count( mlt_animation self )
{
	int count = -1;
	if ( self )
	{
		animation_node node = self->nodes;
		for ( count = 0; node; ++count )
			node = node->next;
	}
	return count;
}

/** Get an animation item for the N-th keyframe.
 *
 * \public \memberof mlt_animation_s
 * \param self an animation
 * \param item an already allocated animation item that will be filled in
 * \param position the frame number for the point in time
 * \return true if there was an error
 */

int mlt_animation_key_get( mlt_animation self, mlt_animation_item item, int index )
{
	int error = 0;
	animation_node node = self->nodes;

	// Iterate through the keyframes.
	int i = index;
	while ( i-- && node )
		node = node->next;

	if ( node )
	{
		item->is_key = node->item.is_key;
		item->frame = node->item.frame;
		item->keyframe_type = node->item.keyframe_type;
		if ( item->property )
			mlt_property_pass( item->property, node->item.property );
	}
	else
	{
		item->frame = item->is_key = 0;
		error = 1;
	}

	return error;
}

/** Close the animation and deallocate all of its resources.
 *
 * \public \memberof mlt_animation_s
 * \param self the animation to destroy
 */

void mlt_animation_close( mlt_animation self )
{
	if ( self )
	{
		mlt_animation_clean( self );
		free( self );
	}
}
