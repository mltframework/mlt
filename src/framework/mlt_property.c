/*
 * mlt_property.c -- property class
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

#include "mlt_property.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Construct and uninitialised property.
*/

mlt_property mlt_property_init( )
{
	mlt_property this = malloc( sizeof( struct mlt_property_s ) );
	if ( this != NULL )
	{
		this->types = 0;
		this->prop_int = 0;
		this->prop_position = 0;
		this->prop_double = 0;
		this->prop_int64 = 0;
		this->prop_string = NULL;
		this->data = NULL;
		this->length = 0;
		this->destructor = NULL;
		this->serialiser = NULL;
	}
	return this;
}

/** Clear a property.
*/

static inline void mlt_property_clear( mlt_property this )
{
	// Special case data handling
	if ( this->types & mlt_prop_data && this->destructor != NULL )
		this->destructor( this->data );

	// Special case string handling
	if ( this->types & mlt_prop_string )
		free( this->prop_string );

	// Wipe stuff
	this->types = 0;
	this->prop_int = 0;
	this->prop_position = 0;
	this->prop_double = 0;
	this->prop_int64 = 0;
	this->prop_string = NULL;
	this->data = NULL;
	this->length = 0;
	this->destructor = NULL;
	this->serialiser = NULL;
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

/** Set a position on this property.
*/

int mlt_property_set_position( mlt_property this, mlt_position value )
{
	mlt_property_clear( this );
	this->types = mlt_prop_position;
	this->prop_position = value;
	return 0;
}

/** Set a string on this property.
*/

int mlt_property_set_string( mlt_property this, const char *value )
{
	if ( value != this->prop_string )
	{
		mlt_property_clear( this );
		this->types = mlt_prop_string;
		if ( value != NULL )
			this->prop_string = strdup( value );
	}
	else
	{
		this->types = mlt_prop_string;
	}
	return this->prop_string == NULL;
}

/** Set an int64 on this property.
*/

int mlt_property_set_int64( mlt_property this, int64_t value )
{
	mlt_property_clear( this );
	this->types = mlt_prop_int64;
	this->prop_int64 = value;
	return 0;
}

/** Set a data on this property.
*/

int mlt_property_set_data( mlt_property this, void *value, int length, mlt_destructor destructor, mlt_serialiser serialiser )
{
	if ( this->data == value )
		this->destructor = NULL;
	mlt_property_clear( this );
	this->types = mlt_prop_data;
	this->data = value;
	this->length = length;
	this->destructor = destructor;
	this->serialiser = serialiser;
	return 0;
}

static inline int mlt_property_atoi( const char *value )
{
	if ( value == NULL )
		return 0;
	else if ( value[0] == '0' && value[1] == 'x' )
		return strtol( value + 2, NULL, 16 );
	else 
		return strtol( value, NULL, 10 );
}

/** Get an int from this property.
*/

int mlt_property_get_int( mlt_property this )
{
	if ( this->types & mlt_prop_int )
		return this->prop_int;
	else if ( this->types & mlt_prop_double )
		return ( int )this->prop_double;
	else if ( this->types & mlt_prop_position )
		return ( int )this->prop_position;
	else if ( this->types & mlt_prop_int64 )
		return ( int )this->prop_int64;
	else if ( this->types & mlt_prop_string )
		return mlt_property_atoi( this->prop_string );
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
	else if ( this->types & mlt_prop_position )
		return ( double )this->prop_position;
	else if ( this->types & mlt_prop_int64 )
		return ( double )this->prop_int64;
	else if ( this->types & mlt_prop_string )
		return atof( this->prop_string );
	return 0;
}

/** Get a position from this property.
*/

mlt_position mlt_property_get_position( mlt_property this )
{
	if ( this->types & mlt_prop_position )
		return this->prop_position;
	else if ( this->types & mlt_prop_int )
		return ( mlt_position )this->prop_int;
	else if ( this->types & mlt_prop_double )
		return ( mlt_position )this->prop_double;
	else if ( this->types & mlt_prop_int64 )
		return ( mlt_position )this->prop_int64;
	else if ( this->types & mlt_prop_string )
		return ( mlt_position )atol( this->prop_string );
	return 0;
}

static inline int64_t mlt_property_atoll( const char *value )
{
	if ( value == NULL )
		return 0;
	else if ( value[0] == '0' && value[1] == 'x' )
		return strtoll( value + 2, NULL, 16 );
	else 
		return strtoll( value, NULL, 10 );
}

/** Get an int64 from this property.
*/

int64_t mlt_property_get_int64( mlt_property this )
{
	if ( this->types & mlt_prop_int64 )
		return this->prop_int64;
	else if ( this->types & mlt_prop_int )
		return ( int64_t )this->prop_int;
	else if ( this->types & mlt_prop_double )
		return ( int64_t )this->prop_double;
	else if ( this->types & mlt_prop_position )
		return ( int64_t )this->prop_position;
	else if ( this->types & mlt_prop_string )
		return mlt_property_atoll( this->prop_string );
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
			sprintf( this->prop_string, "%f", this->prop_double );
		}
		else if ( this->types & mlt_prop_position )
		{
			this->types |= mlt_prop_string;
			this->prop_string = malloc( 32 );
			sprintf( this->prop_string, "%d", (int)this->prop_position ); /* I don't know if this is wanted. -Zach */
		}
		else if ( this->types & mlt_prop_int64 )
		{
			this->types |= mlt_prop_string;
			this->prop_string = malloc( 32 );
			sprintf( this->prop_string, "%lld", this->prop_int64 );
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

/** Pass the property 'that' to 'this'.
* Who to blame: Zach <zachary.drew@gmail.com>
*/
void mlt_property_pass( mlt_property this, mlt_property that )
{
	mlt_property_clear( this );

	this->types = that->types;

	if ( this->types & mlt_prop_int64 )
		this->prop_int64 = that->prop_int64;
	else if ( this->types & mlt_prop_int )
		this->prop_int = that->prop_int;
	else if ( this->types & mlt_prop_double )
		this->prop_double = that->prop_double;
	else if ( this->types & mlt_prop_position )
		this->prop_position = that->prop_position;
	else if ( this->types & mlt_prop_string )
	{
		if ( that->prop_string != NULL )
			this->prop_string = strdup( that->prop_string );
	}
	else if ( this->types & mlt_prop_data && this->serialiser != NULL )
	{
		this->types = mlt_prop_string;
		this->prop_string = this->serialiser( this->data, this->length );	
	}
}
