/*
 * mlt_property.c -- property class
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

#include "config.h"

#include "mlt_property.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Construct and uninitialised property.
*/

mlt_property mlt_property_init( )
{
	return calloc( sizeof( struct mlt_property_s ), 1 );
}

/** Clear a property.
*/

void mlt_property_clear( mlt_property this )
{
	// Special case data handling
	if ( this->types & mlt_prop_data && this->destructor != NULL )
		this->destructor( this->data );

	// Special case string handling
	if ( this->types & mlt_prop_string )
		free( this->prop_string );

	// We can wipe it now.
	memset( this, 0, sizeof( struct mlt_property_s ) );
}

/** Set an int on this property.
*/

int mlt_property_set_int( mlt_property this, int value )
{
	mlt_property_clear( this );
	this->types = mlt_prop_int;
	this->prop_int = value;
	return 0;
}

/** Set a double on this property.
*/

int mlt_property_set_double( mlt_property this, double value )
{
	mlt_property_clear( this );
	this->types = mlt_prop_double;
	this->prop_double = value;
	return 0;
}

/** Set a timecode on this property.
*/

int mlt_property_set_timecode( mlt_property this, mlt_timecode value )
{
	mlt_property_clear( this );
	this->types = mlt_prop_timecode;
	this->prop_timecode = value;
	return 0;
}

/** Set a string on this property.
*/

int mlt_property_set_string( mlt_property this, char *value )
{
	mlt_property_clear( this );
	this->types = mlt_prop_string;
	if ( value != NULL )
		this->prop_string = strdup( value );
	return this->prop_string != NULL;
}

/** Set a data on this property.
*/

int mlt_property_set_data( mlt_property this, void *value, int length, mlt_destructor destructor, mlt_serialiser serialiser )
{
	mlt_property_clear( this );
	this->types = mlt_prop_data;
	this->data = value;
	this->length = length;
	this->destructor = destructor;
	this->serialiser = serialiser;
	return 0;
}

/** Get an int from this property.
*/

int mlt_property_get_int( mlt_property this )
{
	if ( this->types & mlt_prop_int )
		return this->prop_int;
	else if ( this->types & mlt_prop_double )
		return ( int )this->prop_double;
	else if ( this->types & mlt_prop_timecode )
		return ( int )this->prop_timecode;
	else if ( this->types & mlt_prop_string )
		return atoi( this->prop_string );
	return 0;
}

/** Get a double from this property.
*/

double mlt_property_get_double( mlt_property this )
{
	if ( this->types & mlt_prop_double )
		return this->prop_double;
	else if ( this->types & mlt_prop_int )
		return ( double )this->prop_int;
	else if ( this->types & mlt_prop_timecode )
		return ( double )this->prop_timecode;
	else if ( this->types & mlt_prop_string )
		return atof( this->prop_string );
	return 0;
}

/** Get a timecode from this property.
*/

mlt_timecode mlt_property_get_timecode( mlt_property this )
{
	if ( this->types & mlt_prop_timecode )
		return this->prop_timecode;
	else if ( this->types & mlt_prop_int )
		return ( mlt_timecode )this->prop_int;
	else if ( this->types & mlt_prop_double )
		return ( mlt_timecode )this->prop_double;
	else if ( this->types & mlt_prop_string )
		return ( mlt_timecode )atof( this->prop_string );
	return 0;
}

/** Get a string from this property.
*/

char *mlt_property_get_string( mlt_property this )
{
	// Construct a string if need be
	if ( ! ( this->types & mlt_prop_string ) )
	{
		if ( this->types & mlt_prop_int )
		{
			this->types |= mlt_prop_string;
			this->prop_string = malloc( 32 );
			sprintf( this->prop_string, "%d", this->prop_int );
		}
		else if ( this->types & mlt_prop_double )
		{
			this->types |= mlt_prop_string;
			this->prop_string = malloc( 32 );
			sprintf( this->prop_string, "%e", this->prop_double );
		}
		else if ( this->types & mlt_prop_timecode )
		{
			this->types |= mlt_prop_string;
			this->prop_string = malloc( 32 );
			sprintf( this->prop_string, "%e", this->prop_timecode );
		}
		else if ( this->types & mlt_prop_data && this->serialiser != NULL )
		{
			this->types |= mlt_prop_string;
			this->prop_string = this->serialiser( this->data, this->length );
		}
	}

	// Return the string (may be NULL)
	return this->prop_string;
}

/** Get a data and associated length.
*/

void *mlt_property_get_data( mlt_property this, int *length )
{
	// Assign length if not NULL
	if ( length != NULL )
		*length = this->length;

	// Return the data (note: there is no conversion here)
	return this->data;
}

/** Close this property.
*/

void mlt_property_close( mlt_property this )
{
	mlt_property_clear( this );
	free( this );
}


