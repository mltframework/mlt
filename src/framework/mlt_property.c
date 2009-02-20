/**
 * \file mlt_property.c
 * \brief Property class definition
 * \see mlt_property_s
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

#include "mlt_property.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/** Bit pattern used internally to indicated representations available.
*/

typedef enum
{
	mlt_prop_none = 0,    //!< not set
	mlt_prop_int = 1,     //!< set as an integer
	mlt_prop_string = 2,  //!< set as string or already converted to string
	mlt_prop_position = 4,//!< set as a position
	mlt_prop_double = 8,  //!< set as a floating point
	mlt_prop_data = 16,   //!< set as opaque binary
	mlt_prop_int64 = 32   //!< set as a 64-bit integer
}
mlt_property_type;

/** \brief Property class
 *
 * A property is like a variant or dynamic type. They are used for many things
 * in MLT, but in particular they are the parameter mechanism for the plugins.
 */

struct mlt_property_s
{
	/// Stores a bit pattern of types available for this property
	mlt_property_type types;

	/// Atomic type handling
	int prop_int;
	mlt_position prop_position;
	double prop_double;
	int64_t prop_int64;

	/// String handling
	char *prop_string;

	/// Generic type handling
	void *data;
	int length;
	mlt_destructor destructor;
	mlt_serialiser serialiser;
};

/** Construct a property and initialize it
 * \public \memberof mlt_property_s
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

/** Clear (0/null) a property.
 *
 * Frees up any associated resources in the process.
 * \private \memberof mlt_property_s
 * \param this a property
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

/** Set the property to an integer value.
 *
 * \public \memberof mlt_property_s
 * \param this a property
 * \param value an integer
 * \return false
 */

int mlt_property_set_int( mlt_property this, int value )
{
	mlt_property_clear( this );
	this->types = mlt_prop_int;
	this->prop_int = value;
	return 0;
}

/** Set the property to a floating point value.
 *
 * \public \memberof mlt_property_s
 * \param this a property
 * \param value a double precision floating point value
 * \return false
 */

int mlt_property_set_double( mlt_property this, double value )
{
	mlt_property_clear( this );
	this->types = mlt_prop_double;
	this->prop_double = value;
	return 0;
}

/** Set the property to a position value.
 *
 * Position is a relative time value in frame units.
 * \public \memberof mlt_property_s
 * \param this a property
 * \param value a position value
 * \return false
 */

int mlt_property_set_position( mlt_property this, mlt_position value )
{
	mlt_property_clear( this );
	this->types = mlt_prop_position;
	this->prop_position = value;
	return 0;
}

/** Set the property to a string value.
 *
 * This makes a copy of the string you supply so you do not need to track
 * a new reference to it.
 * \public \memberof mlt_property_s
 * \param this a property
 * \param value the string to copy to the property
 * \return true if it failed
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

/** Set the property to a 64-bit integer value.
 *
 * \public \memberof mlt_property_s
 * \param this a property
 * \param value a 64-bit integer
 * \return false
 */

int mlt_property_set_int64( mlt_property this, int64_t value )
{
	mlt_property_clear( this );
	this->types = mlt_prop_int64;
	this->prop_int64 = value;
	return 0;
}

/** Set a property to an opaque binary value.
 *
 * This does not make a copy of the data. You can use a Properties object
 * with its reference tracking and the destructor function to control
 * the lifetime of the data. Otherwise, pass NULL for the destructor
 * function and control the lifetime yourself.
 * \public \memberof mlt_property_s
 * \param this a property
 * \param value an opaque pointer
 * \param length the number of bytes pointed to by value (optional)
 * \param destructor a function to use to destroy this binary data (optional, assuming you manage the resource)
 * \param serialiser a function to use to convert this binary data to a string (optional)
 * \return false
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

/** Convert a base 10 or base 16 string to an integer.
 *
 * The string must begin with '0x' to be interpreted as hexadecimal.
 * Otherwise, it is interpreted as base 10.
 * If the string begins with '#' it is interpreted as a hexadecimal color value
 * in the form RRGGBB or AARRGGBB. Color values that begin with '0x' are
 * always in the form RRGGBBAA where the alpha components are not optional.
 * Applications and services should expect the binary color value in bytes to
 * be in the following order: RGBA. This means they will have to cast the int
 * to an unsigned int. This is especially important when they need to shift
 * right to obtain RGB without alpha in order to make it do a logical instead
 * of arithmetic shift.
 *
 * \private \memberof mlt_property_s
 * \param value a string to convert
 * \return the resultant integer
 */
static inline int mlt_property_atoi( const char *value )
{
	if ( value == NULL )
		return 0;
	// Parse a hex color value as #RRGGBB or #AARRGGBB.
	if ( value[0] == '#' )
	{
		unsigned int rgb = strtoul( value + 1, NULL, 16 );
		unsigned int alpha = ( strlen( value ) > 7 ) ? ( rgb >> 24 ) : 0xff;
		return ( rgb << 8 ) | alpha;
	}
	// Do hex and decimal explicitly to avoid decimal value with leading zeros
	// interpreted as octal.
	else if ( value[0] == '0' && value[1] == 'x' )
	{
		return strtoul( value + 2, NULL, 16 );
	}
	else
	{
		return strtol( value, NULL, 10 );
	}
}

/** Get the property as an integer.
 *
 * \public \memberof mlt_property_s
 * \param this a property
 * \return an integer value
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

/** Get the property as a floating point.
 *
 * \public \memberof mlt_property_s
 * \param this a property
 * \return a floating point value
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

/** Get the property as a position.
 *
 * A position is an offset time in terms of frame units.
 * \public \memberof mlt_property_s
 * \param this a property
 * \return the position in frames
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

/** Convert a string to a 64-bit integer.
 *
 * If the string begins with '0x' it is interpreted as a hexadecimal value.
 * \private \memberof mlt_property_s
 * \param value a string
 * \return a 64-bit integer
 */

static inline int64_t mlt_property_atoll( const char *value )
{
	if ( value == NULL )
		return 0;
	else if ( value[0] == '0' && value[1] == 'x' )
		return strtoll( value + 2, NULL, 16 );
	else
		return strtoll( value, NULL, 10 );
}

/** Get the property as a signed integer.
 *
 * \public \memberof mlt_property_s
 * \param this a property
 * \return a 64-bit integer
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

/** Get the property as a string.
 *
 * The caller is not responsible for deallocating the returned string!
 * The string is deallocated when the Property is closed.
 * This tries its hardest to convert the property to string including using
 * a serialization function for binary data, if supplied.
 * \public \memberof mlt_property_s
 * \param this a property
 * \return a string representation of the property or NULL if failed
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

/** Get the binary data from a property.
 *
 * This only works if you previously put binary data into the property.
 * This does not return a copy of the data; it returns a pointer to it.
 * If you supplied a destructor function when setting the binary data,
 * the destructor is used when the Property is closed to free the memory.
 * Therefore, only free the returned pointer if you did not supply a
 * destructor function.
 * \public \memberof mlt_property_s
 * \param this a property
 * \param[out] length the size of the binary object in bytes (optional)
 * \return an opaque data pointer or NULL if not available
 */

void *mlt_property_get_data( mlt_property this, int *length )
{
	// Assign length if not NULL
	if ( length != NULL )
		*length = this->length;

	// Return the data (note: there is no conversion here)
	return this->data;
}

/** Destroy a property and free all related resources.
 *
 * \public \memberof mlt_property_s
 * \param this a property
 */

void mlt_property_close( mlt_property this )
{
	mlt_property_clear( this );
	free( this );
}

/** Copy a property.
 *
 * A Property holding binary data only copies the data if a serialiser
 * function was supplied when you set the Property.
 * \public \memberof mlt_property_s
 * \author Zach <zachary.drew@gmail.com>
 * \param this a property
 * \param that another property
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
