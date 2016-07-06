/**
 * \file mlt_geometry.c
 * \brief geometry animation API (deprecated)
 * \deprecated use mlt_animation_s instead
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

#include "mlt_geometry.h"
#include "mlt_tokeniser.h"
#include "mlt_factory.h"
#include "mlt_profile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** private part of geometry animation item (deprecated)
 * \deprecated use mlt_animation_s instead
 */

typedef struct geometry_item_s
{
	struct mlt_geometry_item_s data;
	struct geometry_item_s *next, *prev;
}
*geometry_item;

/** private part of geometry object (deprecated)
 * \deprecated use mlt_animation_s instead
 */

typedef struct
{
	char *data;
	int length;
	int nw;
	int nh;
	geometry_item item;
}
geometry_s, *geometry;

// Create a new geometry structure
mlt_geometry mlt_geometry_init( )
{
	mlt_geometry self = calloc( 1, sizeof( struct mlt_geometry_s ) );
	if ( self != NULL )
	{
		self->local = calloc( 1, sizeof( geometry_s ) );
		if ( self->local != NULL )
		{
			geometry g = self->local;
			g->nw = 720;
			g->nh = 576;
		}
		else
		{
			free( self );
			self = NULL;
		}
	}
	return self;
}

/** A linear step
*/

static inline double linearstep( double start, double end, double position, int length )
{
	double o = ( end - start ) / length;
	return start + position * o;
}

void mlt_geometry_interpolate( mlt_geometry self )
{
	geometry g = self->local;

	// Parse of all items to ensure unspecified keys are calculated correctly
	if ( g->item != NULL )
	{
		int i = 0;
		for ( i = 0; i < 5; i ++ )
		{
			geometry_item current = g->item;
			while( current != NULL )
			{
				int fixed = current->data.f[ i ];
				if ( !fixed )
				{
					geometry_item prev = current->prev;
					geometry_item next = current->next;

					double prev_value = 0;
					double next_value = 0;
					double value = 0;

					while( prev != NULL && !prev->data.f[ i ] ) prev = prev->prev;
					while( next != NULL && !next->data.f[ i ] ) next = next->next;

					switch( i )
					{
						case 0:
							if ( prev ) prev_value = prev->data.x;
							if ( next ) next_value = next->data.x;
							break;
						case 1:
							if ( prev ) prev_value = prev->data.y;
							if ( next ) next_value = next->data.y;
							break;
						case 2:
							if ( prev ) prev_value = prev->data.w;
							if ( next ) next_value = next->data.w;
							break;
						case 3:
							if ( prev ) prev_value = prev->data.h;
							if ( next ) next_value = next->data.h;
							break;
						case 4:
							if ( prev ) prev_value = prev->data.mix;
							if ( next ) next_value = next->data.mix;
							break;
					}

					// This should never happen
					if ( prev == NULL )
						current->data.f[ i ] = 1;
					else if ( next == NULL )
						value = prev_value;
					else 
						value = linearstep( prev_value, next_value, current->data.frame - prev->data.frame, next->data.frame - prev->data.frame );

					switch( i )
					{
						case 0: current->data.x = value; break;
						case 1: current->data.y = value; break;
						case 2: current->data.w = value; break;
						case 3: current->data.h = value; break;
						case 4: current->data.mix = value; break;
					}
				}

				// Move to the next item
				current = current->next;
			}
		}
	}
}

static int mlt_geometry_drop( mlt_geometry self, geometry_item item )
{
	geometry g = self->local;

	if ( item == g->item )
	{
		g->item = item->next;
		if ( g->item != NULL )
			g->item->prev = NULL;
		// To ensure correct seeding, ensure all values are fixed
		if ( g->item != NULL )
		{
			g->item->data.f[0] = 1;
			g->item->data.f[1] = 1;
			g->item->data.f[2] = 1;
			g->item->data.f[3] = 1;
			g->item->data.f[4] = 1;
		}
	}
	else if ( item->next != NULL && item->prev != NULL )
	{
		item->prev->next = item->next;
		item->next->prev = item->prev;
	}
	else if ( item->next != NULL )
	{
		item->next->prev = item->prev;
	}
	else if ( item->prev != NULL )
	{
		item->prev->next = item->next;
	}

	free( item );

	return 0;
}

static void mlt_geometry_clean( mlt_geometry self )
{
	geometry g = self->local;
	free( g->data );
	g->data = NULL;
	while( g->item )
		mlt_geometry_drop( self, g->item );
}

// Parse the geometry specification for a given length and normalised width/height (-1 for default)
// data is constructed as: [frame=]X/Y:WxH[:mix][!][;[frame=]X/Y:WxH[:mix][!]]*
// and X, Y, W and H can have trailing % chars to indicate percentage of normalised size
// Append a pair's value with ! to enable distort.
int mlt_geometry_parse( mlt_geometry self, char *data, int length, int nw, int nh )
{
	int i = 0;

	// Create a tokeniser
	mlt_tokeniser tokens = mlt_tokeniser_init( );

	// Get the local/private structure
	geometry g = self->local;

	// Clean the existing geometry
	mlt_geometry_clean( self );

	// Update the info on the data
	if ( length != -1 )
		g->length = length;
	if ( nw != -1 )
		g->nw = nw;
	if ( nh != -1 )
		g->nh = nh;
	if ( data != NULL )
		g->data = strdup( data );

	// Tokenise
	if ( data != NULL )
		mlt_tokeniser_parse_new( tokens, data, ";" );

	// Iterate through each token
	for ( i = 0; i < mlt_tokeniser_count( tokens ); i ++ )
	{
		struct mlt_geometry_item_s item;
		char *value = mlt_tokeniser_get_string( tokens, i );

		// If no data in keyframe, drop it (trailing semicolon)
		if ( value == NULL || !strcmp( value, "" ) )
			continue;

		// Set item to 0
		memset( &item, 0, sizeof( struct mlt_geometry_item_s ) );

		// Now parse the item
		mlt_geometry_parse_item( self, &item, value );

		// Now insert into place
		mlt_geometry_insert( self, &item );
	}
	mlt_geometry_interpolate( self );

	// Remove the tokeniser
	mlt_tokeniser_close( tokens );

	// ???
	return 0;
}

// Conditionally refresh in case of a change
int mlt_geometry_refresh( mlt_geometry self, char *data, int length, int nw, int nh )
{
	geometry g = self->local;
	int changed = ( length != -1 && length != g->length );
	changed = changed || ( nw != -1 && nw != g->nw );
	changed = changed || ( nh != -1 && nh != g->nh );
	changed = changed || ( data != NULL && ( g->data == NULL || strcmp( data, g->data ) ) );
	if ( changed )
		return mlt_geometry_parse( self, data, length, nw, nh );
	return -1;
}

int mlt_geometry_get_length( mlt_geometry self )
{
	// Get the local/private structure
	geometry g = self->local;

	// return the length
	return g->length;
}

void mlt_geometry_set_length( mlt_geometry self, int length )
{
	// Get the local/private structure
	geometry g = self->local;

	// set the length
	g->length = length;
}

int mlt_geometry_parse_item( mlt_geometry self, mlt_geometry_item item, char *value )
{
	int ret = 0;

	// Get the local/private structure
	geometry g = self->local;

	if ( value != NULL && strcmp( value, "" ) )
	{
		char *p = strchr( value, '=' );
		int count = 0;
		double temp;

		// Determine if a position has been specified
		if ( p != NULL )
		{
			temp = atof( value );
			if ( temp > -1 && temp < 1 )
				item->frame = temp * g->length;
			else
				item->frame = temp;
			value = p + 1;
		}

		// Special case - frame < 0
		if ( item->frame < 0 )
			item->frame += g->length;

		// Obtain the current value at this position - self allows new
		// frames to be created which don't specify all values
		mlt_geometry_fetch( self, item, item->frame );

		// Special case - when an empty string is specified, all values are fixed
		// TODO: Check if this is logical - it's convenient, but it's also odd...
		if ( !*value )
		{
			item->f[0] = 1;
			item->f[1] = 1;
			item->f[2] = 1;
			item->f[3] = 1;
			item->f[4] = 1;
		}

		// Iterate through the remainder of value
		while( *value )
		{
			// Get the value
			temp = strtod( value, &p );

			// Check if a value was specified
			if ( p != value )
			{
				// Handle the % case
				if ( *p == '%' )
				{
					if ( count == 0 || count == 2 )
						temp *= g->nw / 100.0;
					else if ( count == 1 || count == 3 )
						temp *= g->nh / 100.0;
					p ++;
				}

				// Special case - distort token
				if ( *p == '!' || *p == '*' )
				{
					p ++;
					item->distort = 1;
				}

				// Actually, we don't care about the delimiter at all..
				if ( *p ) p ++;

				// Assign to the item
				switch( count )
				{
					case 0: item->x = temp; item->f[0] = 1; break;
					case 1: item->y = temp; item->f[1] = 1; break;
					case 2: item->w = temp; item->f[2] = 1; break;
					case 3: item->h = temp; item->f[3] = 1; break;
					case 4: item->mix = temp; item->f[4] = 1; break;
				}
			}
			else
			{
				p ++;
			}

			// Update the value pointer
			value = p;
			count ++;
		}
	}
	else
	{
		ret = 1;
	}

	return ret;
}

// Fetch a geometry item for an absolute position
int mlt_geometry_fetch( mlt_geometry self, mlt_geometry_item item, float position )
{
	// Get the local geometry
	geometry g = self->local;

	// Need to find the nearest key to the position specifed
	geometry_item key = g->item;

	// Iterate through the keys until we reach last or have 
	while( key != NULL && key->next != NULL && position >= key->next->data.frame )
		key = key->next;

	if ( key != NULL )
	{
		// Position is situated before the first key - all zeroes
		if ( position < key->data.frame )
		{
			memset( item, 0, sizeof( struct mlt_geometry_item_s ) );
			item->mix = 100;
		}
		// Position is a key itself - no iterpolation need
		else if ( position == key->data.frame )
		{
			memcpy( item, &key->data, sizeof( struct mlt_geometry_item_s ) );
		}
		// Position is after the last key - no interpolation, but not a key frame
		else if ( key->next == NULL )
		{
			memcpy( item, &key->data, sizeof( struct mlt_geometry_item_s ) );
			item->key = 0;
			item->f[ 0 ] = 0;
			item->f[ 1 ] = 0;
			item->f[ 2 ] = 0;
			item->f[ 3 ] = 0;
			item->f[ 4 ] = 0;
		}
		// Interpolation is needed - position > key and there is a following key
		else
		{
			item->key = 0;
			item->frame = position;
			position -= key->data.frame;
			item->x = linearstep( key->data.x, key->next->data.x, position, key->next->data.frame - key->data.frame );
			item->y = linearstep( key->data.y, key->next->data.y, position, key->next->data.frame - key->data.frame );
			item->w = linearstep( key->data.w, key->next->data.w, position, key->next->data.frame - key->data.frame );
			item->h = linearstep( key->data.h, key->next->data.h, position, key->next->data.frame - key->data.frame );
			item->mix = linearstep( key->data.mix, key->next->data.mix, position, key->next->data.frame - key->data.frame );
			item->distort = key->data.distort;
			position += key->data.frame;
		}

		item->frame = position;
	}
	else
	{
		memset( item, 0, sizeof( struct mlt_geometry_item_s ) );
		item->frame = position;
		item->mix = 100;
	}

	return key == NULL;
}

// Specify a geometry item at an absolute position
int mlt_geometry_insert( mlt_geometry self, mlt_geometry_item item )
{
	// Get the local/private geometry structure
	geometry g = self->local;

	// Create a new local item (this may be removed if a key already exists at self position)
	geometry_item gi = calloc( 1, sizeof( struct geometry_item_s ) );
	memcpy( &gi->data, item, sizeof( struct mlt_geometry_item_s ) );
	gi->data.key = 1;

	// Determine if we need to insert or append to the list, or if it's a new list
	if ( g->item != NULL )
	{
		// Get the first item
		geometry_item place = g->item;

		// Locate an existing nearby item
		while ( place->next != NULL && item->frame > place->data.frame )
			place = place->next;

		if ( item->frame < place->data.frame )
		{
			if ( place == g->item )
				g->item = gi;
			if ( place->prev )
				place->prev->next = gi;
			gi->next = place;
			gi->prev = place->prev;
			place->prev = gi;
		}
		else if ( item->frame > place->data.frame )
		{
			if ( place->next )
				place->next->prev = gi;
			gi->next = place->next;
			gi->prev = place;
			place->next = gi;
		}
		else
		{
			memcpy( &place->data, &gi->data, sizeof( struct mlt_geometry_item_s ) );
			free( gi );
		}
	}
	else
	{
		// Set the first item
		g->item = gi;

		// To ensure correct seeding, ensure all values are fixed
		g->item->data.f[0] = 1;
		g->item->data.f[1] = 1;
		g->item->data.f[2] = 1;
		g->item->data.f[3] = 1;
		g->item->data.f[4] = 1;
	}

	// TODO: Error checking
	return 0;
}

// Remove the key at the specified position
int mlt_geometry_remove( mlt_geometry self, int position )
{
	int ret = 1;

	// Get the local/private geometry structure
	geometry g = self->local;

	// Get the first item
	geometry_item place = g->item;

	while( place != NULL && position != place->data.frame )
		place = place->next;

	if ( place != NULL && position == place->data.frame )
		ret = mlt_geometry_drop( self, place );

	return ret;
}

// Get the key at the position or the next following
int mlt_geometry_next_key( mlt_geometry self, mlt_geometry_item item, int position )
{
	// Get the local/private geometry structure
	geometry g = self->local;

	// Get the first item
	geometry_item place = g->item;

	while( place != NULL && position > place->data.frame )
		place = place->next;

	if ( place != NULL )
		memcpy( item, &place->data, sizeof( struct mlt_geometry_item_s ) );

	return place == NULL;
}

// Get the key at the position or the previous key
int mlt_geometry_prev_key( mlt_geometry self, mlt_geometry_item item, int position )
{
	// Get the local/private geometry structure
	geometry g = self->local;

	// Get the first item
	geometry_item place = g->item;

	while( place != NULL && place->next != NULL && position >= place->next->data.frame )
		place = place->next;

	if ( place != NULL )
		memcpy( item, &place->data, sizeof( struct mlt_geometry_item_s ) );

	return place == NULL;
}

char *mlt_geometry_serialise_cut( mlt_geometry self, int in, int out )
{
	geometry g = self->local;
	struct mlt_geometry_item_s item;
	char *ret = malloc( 1000 );
	int used = 0;
	int size = 1000;

	if ( in == -1 )
		in = 0;
	if ( out == -1 )
		out = mlt_geometry_get_length( self );

	if ( ret != NULL )
	{
		char temp[ 100 ];

		strcpy( ret, "" );

		item.frame = in;

		while( 1 )
		{
			strcpy( temp, "" );

			// If it's the first frame, then it's not necessarily a key
			if ( item.frame == in )
			{
				if ( mlt_geometry_fetch( self, &item, item.frame ) )
					break;

				// If the first key is larger than the current position
				// then do nothing here
				if ( g->item->data.frame > item.frame )
				{
					item.frame ++;
					continue;
				}

				// To ensure correct seeding, ensure all values are fixed
				item.f[0] = 1;
				item.f[1] = 1;
				item.f[2] = 1;
				item.f[3] = 1;
				item.f[4] = 1;
			}
			// Typically, we move from key to key
			else if ( item.frame < out )
			{
				if ( mlt_geometry_next_key( self, &item, item.frame ) )
					break;

				// Special case - crop at the out point
				if ( item.frame > out )
					mlt_geometry_fetch( self, &item, out );
			}
			// We've handled the last key
			else
			{
				break;
			}

			if ( item.frame - in != 0 )
				sprintf( temp, "%d=", item.frame - in );

			if ( item.f[0] )
				sprintf( temp + strlen( temp ), "%g", item.x );
			if ( item.f[1] ) {
				strcat( temp, "/" );
				sprintf( temp + strlen( temp ), "%g", item.y );
			}
			if ( item.f[2] ) {
				strcat( temp, ":" );
				sprintf( temp + strlen( temp ), "%g", item.w );
			}
			if ( item.f[3] ) {
				strcat( temp, "x" );
				sprintf( temp + strlen( temp ), "%g", item.h );
			}
			if ( item.f[4] ) {
				strcat( temp, ":" );
				sprintf( temp + strlen( temp ), "%g", item.mix );
			}

			if ( used + strlen( temp ) + 2 > size ) // +2 for ';' and NULL
			{
				size += 1000;
				ret = realloc( ret, size );
			}

			if ( ret != NULL && used != 0 )
			{
				used ++;
				strcat( ret, ";" );
			}
			if ( ret != NULL )
			{
				used += strlen( temp );
				strcat( ret, temp );
			}

			item.frame ++;
		}
	}

	return ret;
}

// Serialise the current geometry
char *mlt_geometry_serialise( mlt_geometry self )
{
	geometry g = self->local;
	char *ret = mlt_geometry_serialise_cut( self, 0, g->length );
	if ( ret )
	{
		free( g->data );
		g->data = ret;
	}
	return strdup( ret );
}

// Close the geometry
void mlt_geometry_close( mlt_geometry self )
{
	if ( self != NULL )
	{
		mlt_geometry_clean( self );
		free( self->local );
		free( self );
	}
}


