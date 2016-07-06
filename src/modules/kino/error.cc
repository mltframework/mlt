/*
* error.cc Error handling
* Copyright (C) 2000 Arne Schirmacher <arne@schirmacher.de>
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
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// C++ includes

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

using std::ostringstream;
using std::string;
using std::endl;
using std::ends;
using std::cerr;

// C includes

#include <errno.h>
#include <string.h>

// local includes

#include "error.h"

void real_fail_neg( int eval, const char *eval_str, const char *func, const char *file, int line )
{
	if ( eval < 0 )
	{
		string exc;
		ostringstream sb;

		sb << file << ":" << line << ": In function \"" << func << "\": \"" << eval_str << "\" evaluated to " << eval;
		if ( errno != 0 )
			sb << endl << file << ":" << line << ": errno: " << errno << " (" << strerror( errno ) << ")";
		sb << ends;
		exc = sb.str();
		cerr << exc << endl;
		throw exc;
	}
}


/** error handler for NULL result codes
 
    Whenever this is called with a NULL argument, it will throw an
    exception. Typically used with functions like malloc() and new().
 
*/

void real_fail_null( const void *eval, const char *eval_str, const char *func, const char *file, int line )
{
	if ( eval == NULL )
	{

		string exc;
		ostringstream sb;

		sb << file << ":" << line << ": In function \"" << func << "\": " << eval_str << " is NULL" << ends;
		exc = sb.str();
		cerr << exc << endl;
		throw exc;
	}
}


void real_fail_if( bool eval, const char *eval_str, const char *func, const char *file, int line )
{
	if ( eval == true )
	{

		string exc;
		ostringstream sb;

		sb << file << ":" << line << ": In function \"" << func << "\": condition \"" << eval_str << "\" is true";
		if ( errno != 0 )
			sb << endl << file << ":" << line << ": errno: " << errno << " (" << strerror( errno ) << ")";
		sb << ends;
		exc = sb.str();
		cerr << exc << endl;
		throw exc;
	}
}
