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

// For strtod_l
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "mlt_property.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <pthread.h>


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

	pthread_mutex_t mutex;
};

/** Construct a property and initialize it
 * \public \memberof mlt_property_s
 */

mlt_property mlt_property_init( )
{
	mlt_property self = malloc( sizeof( struct mlt_property_s ) );
	if ( self != NULL )
	{
		self->types = 0;
		self->prop_int = 0;
		self->prop_position = 0;
		self->prop_double = 0;
		self->prop_int64 = 0;
		self->prop_string = NULL;
		self->data = NULL;
		self->length = 0;
		self->destructor = NULL;
		self->serialiser = NULL;
		pthread_mutex_init( &self->mutex, NULL );
	}
	return self;
}

/** Clear (0/null) a property.
 *
 * Frees up any associated resources in the process.
 * \private \memberof mlt_property_s
 * \param self a property
 */

static inline void mlt_property_clear( mlt_property self )
{
	// Special case data handling
	if ( self->types & mlt_prop_data && self->destructor != NULL )
		self->destructor( self->data );

	// Special case string handling
	if ( self->types & mlt_prop_string )
		free( self->prop_string );

	// Wipe stuff
	self->types = 0;
	self->prop_int = 0;
	self->prop_position = 0;
	self->prop_double = 0;
	self->prop_int64 = 0;
	self->prop_string = NULL;
	self->data = NULL;
	self->length = 0;
	self->destructor = NULL;
	self->serialiser = NULL;
}

/** Set the property to an integer value.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value an integer
 * \return false
 */

int mlt_property_set_int( mlt_property self, int value )
{
	pthread_mutex_lock( &self->mutex );
	mlt_property_clear( self );
	self->types = mlt_prop_int;
	self->prop_int = value;
	pthread_mutex_unlock( &self->mutex );
	return 0;
}

/** Set the property to a floating point value.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value a double precision floating point value
 * \return false
 */

int mlt_property_set_double( mlt_property self, double value )
{
	pthread_mutex_lock( &self->mutex );
	mlt_property_clear( self );
	self->types = mlt_prop_double;
	self->prop_double = value;
	pthread_mutex_unlock( &self->mutex );
	return 0;
}

/** Set the property to a position value.
 *
 * Position is a relative time value in frame units.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value a position value
 * \return false
 */

int mlt_property_set_position( mlt_property self, mlt_position value )
{
	pthread_mutex_lock( &self->mutex );
	mlt_property_clear( self );
	self->types = mlt_prop_position;
	self->prop_position = value;
	pthread_mutex_unlock( &self->mutex );
	return 0;
}

/** Set the property to a string value.
 *
 * This makes a copy of the string you supply so you do not need to track
 * a new reference to it.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value the string to copy to the property
 * \return true if it failed
 */

int mlt_property_set_string( mlt_property self, const char *value )
{
	pthread_mutex_lock( &self->mutex );
	if ( value != self->prop_string )
	{
		mlt_property_clear( self );
		self->types = mlt_prop_string;
		if ( value != NULL )
			self->prop_string = strdup( value );
	}
	else
	{
		self->types = mlt_prop_string;
	}
	pthread_mutex_unlock( &self->mutex );
	return self->prop_string == NULL;
}

/** Set the property to a 64-bit integer value.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value a 64-bit integer
 * \return false
 */

int mlt_property_set_int64( mlt_property self, int64_t value )
{
	pthread_mutex_lock( &self->mutex );
	mlt_property_clear( self );
	self->types = mlt_prop_int64;
	self->prop_int64 = value;
	pthread_mutex_unlock( &self->mutex );
	return 0;
}

/** Set a property to an opaque binary value.
 *
 * This does not make a copy of the data. You can use a Properties object
 * with its reference tracking and the destructor function to control
 * the lifetime of the data. Otherwise, pass NULL for the destructor
 * function and control the lifetime yourself.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param value an opaque pointer
 * \param length the number of bytes pointed to by value (optional)
 * \param destructor a function to use to destroy this binary data (optional, assuming you manage the resource)
 * \param serialiser a function to use to convert this binary data to a string (optional)
 * \return false
 */

int mlt_property_set_data( mlt_property self, void *value, int length, mlt_destructor destructor, mlt_serialiser serialiser )
{
	pthread_mutex_lock( &self->mutex );
	if ( self->data == value )
		self->destructor = NULL;
	mlt_property_clear( self );
	self->types = mlt_prop_data;
	self->data = value;
	self->length = length;
	self->destructor = destructor;
	self->serialiser = serialiser;
	pthread_mutex_unlock( &self->mutex );
	return 0;
}

/** Parse a SMIL clock value.
 *
 * \private \memberof mlt_property_s
 * \param s the string to parse
 * \param fps frames per second
 * \param locale the locale to use for parsing a real number value
 * \return position in frames
 */

static int time_clock_to_frames( const char *s, double fps, locale_t locale )
{
	char *pos, *copy = strdup( s );
	int hours = 0, minutes = 0;
	double seconds;

	s = copy;
	pos = strrchr( s, ':' );
	if ( pos ) {
#if defined(__GLIBC__) || defined(__DARWIN__)
		if ( locale )
			seconds = strtod_l( pos + 1, NULL, locale );
		else
#endif
			seconds = strtod( pos + 1, NULL );
		*pos = 0;
		pos = strrchr( s, ':' );
		if ( pos ) {
			minutes = atoi( pos + 1 );
			*pos = 0;
			hours = atoi( s );
		}
		else {
			minutes = atoi( s );
		}
	}
	else {
#if defined(__GLIBC__) || defined(__DARWIN__)
		if ( locale )
			seconds = strtod_l( s, NULL, locale );
		else
#endif
			seconds = strtod( s, NULL );
	}
	free( copy );

	return fps * ( (hours * 3600) + (minutes * 60) + seconds ) + 0.5;
}

/** Parse a SMPTE timecode string.
 *
 * \private \memberof mlt_property_s
 * \param s the string to parse
 * \param fps frames per second
 * \return position in frames
 */

static int time_code_to_frames( const char *s, double fps )
{
	char *pos, *copy = strdup( s );
	int hours = 0, minutes = 0, seconds = 0, frames;

	s = copy;
	pos = strrchr( s, ';' );
	if ( !pos )
		pos = strrchr( s, ':' );
	if ( pos ) {
		frames = atoi( pos + 1 );
		*pos = 0;
		pos = strrchr( s, ':' );
		if ( pos ) {
			seconds = atoi( pos + 1 );
			*pos = 0;
			pos = strrchr( s, ':' );
			if ( pos ) {
				minutes = atoi( pos + 1 );
				*pos = 0;
				hours = atoi( s );
			}
			else {
				minutes = atoi( s );
			}
		}
		else {
			seconds = atoi( s );
		}
	}
	else {
		frames = atoi( s );
	}
	free( copy );

	return frames + ( fps * ( (hours * 3600) + (minutes * 60) + seconds ) + 0.5 );
}

/** Convert a string to an integer.
 *
 * The string must begin with '0x' to be interpreted as hexadecimal.
 * Otherwise, it is interpreted as base 10.
 *
 * If the string begins with '#' it is interpreted as a hexadecimal color value
 * in the form RRGGBB or AARRGGBB. Color values that begin with '0x' are
 * always in the form RRGGBBAA where the alpha components are not optional.
 * Applications and services should expect the binary color value in bytes to
 * be in the following order: RGBA. This means they will have to cast the int
 * to an unsigned int. This is especially important when they need to shift
 * right to obtain RGB without alpha in order to make it do a logical instead
 * of arithmetic shift.
 *
 * If the string contains a colon it is interpreted as a time value. If it also
 * contains a period or comma character, the string is parsed as a clock value:
 * HH:MM:SS. Otherwise, the time value is parsed as a SMPTE timecode: HH:MM:SS:FF.
 * \private \memberof mlt_property_s
 * \param value a string to convert
 * \param fps frames per second, used when converting from time value
 * \param locale the locale to use when converting from time clock value
 * \return the resultant integer
 */
static int mlt_property_atoi( const char *value, double fps, locale_t locale )
{
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
	else if ( fps > 0 && strchr( value, ':' ) )
	{
		if ( strchr( value, '.' ) || strchr( value, ',' ) )
			return time_clock_to_frames( value, fps, locale );
		else
			return time_code_to_frames( value, fps );
	}
	else
	{
		return strtol( value, NULL, 10 );
	}
}

/** Get the property as an integer.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps frames per second, used when converting from time value
 * \param locale the locale to use when converting from time clock value
 * \return an integer value
 */

int mlt_property_get_int( mlt_property self, double fps, locale_t locale )
{
	if ( self->types & mlt_prop_int )
		return self->prop_int;
	else if ( self->types & mlt_prop_double )
		return ( int )self->prop_double;
	else if ( self->types & mlt_prop_position )
		return ( int )self->prop_position;
	else if ( self->types & mlt_prop_int64 )
		return ( int )self->prop_int64;
	else if ( ( self->types & mlt_prop_string ) && self->prop_string )
		return mlt_property_atoi( self->prop_string, fps, locale );
	return 0;
}

/** Convert a string to a floating point number.
 *
 * If the string contains a colon it is interpreted as a time value. If it also
 * contains a period or comma character, the string is parsed as a clock value:
 * HH:MM:SS. Otherwise, the time value is parsed as a SMPTE timecode: HH:MM:SS:FF.
 * \private \memberof mlt_property_s
 * \param value the string to convert
 * \param fps frames per second, used when converting from time value
 * \param locale the locale to use when converting from time clock value
 * \return the resultant real number
 */
static double mlt_property_atof( const char *value, double fps, locale_t locale )
{
	if ( fps > 0 && strchr( value, ':' ) )
	{
		if ( strchr( value, '.' ) || strchr( value, ',' ) )
			return time_clock_to_frames( value, fps, locale );
		else
			return time_code_to_frames( value, fps );
	}
	else
	{
#if defined(__GLIBC__) || defined(__DARWIN__)
		if ( locale )
			return strtod_l( value, NULL, locale );
#endif
		return strtod( value, NULL );
	}
}

/** Get the property as a floating point.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps frames per second, used when converting from time value
 * \param locale the locale to use for this conversion
 * \return a floating point value
 */

double mlt_property_get_double( mlt_property self, double fps, locale_t locale )
{
	if ( self->types & mlt_prop_double )
		return self->prop_double;
	else if ( self->types & mlt_prop_int )
		return ( double )self->prop_int;
	else if ( self->types & mlt_prop_position )
		return ( double )self->prop_position;
	else if ( self->types & mlt_prop_int64 )
		return ( double )self->prop_int64;
	else if ( ( self->types & mlt_prop_string ) && self->prop_string )
		return mlt_property_atof( self->prop_string, fps, locale );
	return 0;
}

/** Get the property as a position.
 *
 * A position is an offset time in terms of frame units.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param fps frames per second, used when converting from time value
 * \param locale the locale to use when converting from time clock value
 * \return the position in frames
 */

mlt_position mlt_property_get_position( mlt_property self, double fps, locale_t locale )
{
	if ( self->types & mlt_prop_position )
		return self->prop_position;
	else if ( self->types & mlt_prop_int )
		return ( mlt_position )self->prop_int;
	else if ( self->types & mlt_prop_double )
		return ( mlt_position )self->prop_double;
	else if ( self->types & mlt_prop_int64 )
		return ( mlt_position )self->prop_int64;
	else if ( ( self->types & mlt_prop_string ) && self->prop_string )
		return ( mlt_position )mlt_property_atoi( self->prop_string, fps, locale );
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
 * \param self a property
 * \return a 64-bit integer
 */

int64_t mlt_property_get_int64( mlt_property self )
{
	if ( self->types & mlt_prop_int64 )
		return self->prop_int64;
	else if ( self->types & mlt_prop_int )
		return ( int64_t )self->prop_int;
	else if ( self->types & mlt_prop_double )
		return ( int64_t )self->prop_double;
	else if ( self->types & mlt_prop_position )
		return ( int64_t )self->prop_position;
	else if ( ( self->types & mlt_prop_string ) && self->prop_string )
		return mlt_property_atoll( self->prop_string );
	return 0;
}

/** Get the property as a string.
 *
 * The caller is not responsible for deallocating the returned string!
 * The string is deallocated when the Property is closed.
 * This tries its hardest to convert the property to string including using
 * a serialization function for binary data, if supplied.
 * \public \memberof mlt_property_s
 * \param self a property
 * \return a string representation of the property or NULL if failed
 */

char *mlt_property_get_string( mlt_property self )
{
	// Construct a string if need be
	if ( ! ( self->types & mlt_prop_string ) )
	{
		pthread_mutex_lock( &self->mutex );
		if ( self->types & mlt_prop_int )
		{
			self->types |= mlt_prop_string;
			self->prop_string = malloc( 32 );
			sprintf( self->prop_string, "%d", self->prop_int );
		}
		else if ( self->types & mlt_prop_double )
		{
			self->types |= mlt_prop_string;
			self->prop_string = malloc( 32 );
			sprintf( self->prop_string, "%f", self->prop_double );
		}
		else if ( self->types & mlt_prop_position )
		{
			self->types |= mlt_prop_string;
			self->prop_string = malloc( 32 );
			sprintf( self->prop_string, "%d", (int)self->prop_position );
		}
		else if ( self->types & mlt_prop_int64 )
		{
			self->types |= mlt_prop_string;
			self->prop_string = malloc( 32 );
			sprintf( self->prop_string, "%"PRId64, self->prop_int64 );
		}
		else if ( self->types & mlt_prop_data && self->serialiser != NULL )
		{
			self->types |= mlt_prop_string;
			self->prop_string = self->serialiser( self->data, self->length );
		}
		pthread_mutex_unlock( &self->mutex );
	}

	// Return the string (may be NULL)
	return self->prop_string;
}

/** Get the property as a string (with locale).
 *
 * The caller is not responsible for deallocating the returned string!
 * The string is deallocated when the Property is closed.
 * This tries its hardest to convert the property to string including using
 * a serialization function for binary data, if supplied.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param locale the locale to use for this conversion
 * \return a string representation of the property or NULL if failed
 */

char *mlt_property_get_string_l( mlt_property self, locale_t locale )
{
	// Optimization for no locale
	if ( !locale )
		return mlt_property_get_string( self );

	// Construct a string if need be
	if ( ! ( self->types & mlt_prop_string ) )
	{
		// TODO: when glibc gets sprintf_l, start using it! For now, hack on setlocale.
		// Save the current locale
#if defined(__DARWIN__)
		const char *localename = querylocale( LC_NUMERIC, locale );
#elif defined(__GLIBC__)
		const char *localename = locale->__names[ LC_NUMERIC ];
#else
		// TODO: not yet sure what to do on other platforms
		const char *localename = "";
#endif
		// Protect damaging the global locale from a temporary locale on another thread.
		pthread_mutex_lock( &self->mutex );

		// Get the current locale
		char *orig_localename = strdup( setlocale( LC_NUMERIC, NULL ) );

		// Set the new locale
		setlocale( LC_NUMERIC, localename );

		if ( self->types & mlt_prop_int )
		{
			self->types |= mlt_prop_string;
			self->prop_string = malloc( 32 );
			sprintf( self->prop_string, "%d", self->prop_int );
		}
		else if ( self->types & mlt_prop_double )
		{
			self->types |= mlt_prop_string;
			self->prop_string = malloc( 32 );
			sprintf( self->prop_string, "%f", self->prop_double );
		}
		else if ( self->types & mlt_prop_position )
		{
			self->types |= mlt_prop_string;
			self->prop_string = malloc( 32 );
			sprintf( self->prop_string, "%d", (int)self->prop_position );
		}
		else if ( self->types & mlt_prop_int64 )
		{
			self->types |= mlt_prop_string;
			self->prop_string = malloc( 32 );
			sprintf( self->prop_string, "%"PRId64, self->prop_int64 );
		}
		else if ( self->types & mlt_prop_data && self->serialiser != NULL )
		{
			self->types |= mlt_prop_string;
			self->prop_string = self->serialiser( self->data, self->length );
		}
		// Restore the current locale
		setlocale( LC_NUMERIC, orig_localename );
		free( orig_localename );
		pthread_mutex_unlock( &self->mutex );
	}

	// Return the string (may be NULL)
	return self->prop_string;
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
 * \param self a property
 * \param[out] length the size of the binary object in bytes (optional)
 * \return an opaque data pointer or NULL if not available
 */

void *mlt_property_get_data( mlt_property self, int *length )
{
	// Assign length if not NULL
	if ( length != NULL )
		*length = self->length;

	// Return the data (note: there is no conversion here)
	return self->data;
}

/** Destroy a property and free all related resources.
 *
 * \public \memberof mlt_property_s
 * \param self a property
 */

void mlt_property_close( mlt_property self )
{
	mlt_property_clear( self );
	pthread_mutex_destroy( &self->mutex );
	free( self );
}

/** Copy a property.
 *
 * A Property holding binary data only copies the data if a serialiser
 * function was supplied when you set the Property.
 * \public \memberof mlt_property_s
 * \author Zach <zachary.drew@gmail.com>
 * \param self a property
 * \param that another property
 */
void mlt_property_pass( mlt_property self, mlt_property that )
{
	pthread_mutex_lock( &self->mutex );
	mlt_property_clear( self );

	self->types = that->types;

	if ( self->types & mlt_prop_int64 )
		self->prop_int64 = that->prop_int64;
	else if ( self->types & mlt_prop_int )
		self->prop_int = that->prop_int;
	else if ( self->types & mlt_prop_double )
		self->prop_double = that->prop_double;
	else if ( self->types & mlt_prop_position )
		self->prop_position = that->prop_position;
	if ( self->types & mlt_prop_string )
	{
		if ( that->prop_string != NULL )
			self->prop_string = strdup( that->prop_string );
	}
	else if ( self->types & mlt_prop_data && self->serialiser != NULL )
	{
		self->types = mlt_prop_string;
		self->prop_string = self->serialiser( self->data, self->length );
	}
	pthread_mutex_unlock( &self->mutex );
}

/** Convert frame count to a SMPTE timecode string.
 *
 * \private \memberof mlt_property_s
 * \param frames a frame count
 * \param fps frames per second
 * \param[out] s the string to write into - must have enough space to hold largest time string
 */

static void time_smpte_from_frames( int frames, double fps, char *s )
{
	int hours, mins, secs;
	char frame_sep = ':';

	if ( fps == 30000.0/1001.0 )
	{
		fps = 30.0;
		int i, max_frames = frames;
		for ( i = 1800; i <= max_frames; i += 1800 )
		{
			if ( i % 18000 )
			{
				max_frames += 2;
				frames += 2;
			}
		}
		frame_sep = ';';
	}
	hours = frames / ( fps * 3600 );
	frames -= hours * ( fps * 3600 );
	mins = frames / ( fps * 60 );
	frames -= mins * ( fps * 60 );
	secs = frames / fps;
	frames -= secs * fps;

	sprintf( s, "%02d:%02d:%02d%c%02d", hours, mins, secs, frame_sep, frames );
}

/** Convert frame count to a SMIL clock value string.
 *
 * \private \memberof mlt_property_s
 * \param frames a frame count
 * \param fps frames per second
 * \param[out] s the string to write into - must have enough space to hold largest time string
 */

static void time_clock_from_frames( int frames, double fps, char *s )
{
	int hours, mins;
	double secs;

	hours = frames / ( fps * 3600 );
	frames -= hours * ( fps * 3600 );
	mins = frames / ( fps * 60 );
	frames -= mins * ( fps * 60 );
	secs = (double) frames / fps;

	sprintf( s, "%02d:%02d:%06.3f", hours, mins, secs );
}

/** Get the property as a time string.
 *
 * The time value can be either a SMPTE timecode or SMIL clock value.
 * The caller is not responsible for deallocating the returned string!
 * The string is deallocated when the property is closed.
 * \public \memberof mlt_property_s
 * \param self a property
 * \param format the time format that you want
 * \param fps frames per second
 * \param locale the locale to use for this conversion
 * \return a string representation of the property or NULL if failed
 */

char *mlt_property_get_time( mlt_property self, mlt_time_format format, double fps, locale_t locale )
{
	char *orig_localename = NULL;
	const char *localename = "";

	// Optimization for mlt_time_frames
	if ( format == mlt_time_frames )
		return mlt_property_get_string_l( self, locale );

	// Remove existing string
	if ( self->prop_string )
		mlt_property_set_int( self, mlt_property_get_int( self, fps, locale ) );

	// Use the specified locale
	if ( locale )
	{
		// TODO: when glibc gets sprintf_l, start using it! For now, hack on setlocale.
		// Save the current locale
#if defined(__DARWIN__)
		localename = querylocale( LC_NUMERIC, locale );
#elif defined(__GLIBC__)
		localename = locale->__names[ LC_NUMERIC ];
#else
		// TODO: not yet sure what to do on other platforms
#endif
		// Protect damaging the global locale from a temporary locale on another thread.
		pthread_mutex_lock( &self->mutex );

		// Get the current locale
		orig_localename = strdup( setlocale( LC_NUMERIC, NULL ) );

		// Set the new locale
		setlocale( LC_NUMERIC, localename );
	}
	else
	{
		// Make sure we have a lock before accessing self->types
		pthread_mutex_lock( &self->mutex );
	}

	// Convert number to string
	if ( self->types & mlt_prop_int )
	{
		self->types |= mlt_prop_string;
		self->prop_string = malloc( 32 );
		if ( format == mlt_time_clock )
			time_clock_from_frames( self->prop_int, fps, self->prop_string );
		else
			time_smpte_from_frames( self->prop_int, fps, self->prop_string );
	}
	else if ( self->types & mlt_prop_position )
	{
		self->types |= mlt_prop_string;
		self->prop_string = malloc( 32 );
		if ( format == mlt_time_clock )
			time_clock_from_frames( (int) self->prop_position, fps, self->prop_string );
		else
			time_smpte_from_frames( (int) self->prop_position, fps, self->prop_string );
	}
	else if ( self->types & mlt_prop_double )
	{
		self->types |= mlt_prop_string;
		self->prop_string = malloc( 32 );
		if ( format == mlt_time_clock )
			time_clock_from_frames( self->prop_double, fps, self->prop_string );
		else
			time_smpte_from_frames( self->prop_double, fps, self->prop_string );
	}
	else if ( self->types & mlt_prop_int64 )
	{
		self->types |= mlt_prop_string;
		self->prop_string = malloc( 32 );
		if ( format == mlt_time_clock )
			time_clock_from_frames( (int) self->prop_int64, fps, self->prop_string );
		else
			time_smpte_from_frames( (int) self->prop_int64, fps, self->prop_string );
	}

	// Restore the current locale
	if ( locale )
	{
		setlocale( LC_NUMERIC, orig_localename );
		free( orig_localename );
		pthread_mutex_unlock( &self->mutex );
	}
	else
	{
		// Make sure we have a lock before accessing self->types
		pthread_mutex_unlock( &self->mutex );
	}

	// Return the string (may be NULL)
	return self->prop_string;
}
