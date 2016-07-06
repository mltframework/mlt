/*
 * Copyright (C) 2016 Meltytech, LLC
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

#include "common.h"

#include <string.h>
#include <ctype.h>

// Returns string length of "plain:" prefix if webvx or speed parameter if timewarp.
// Otherwise, returns 0.
size_t mlt_xml_prefix_size( mlt_properties properties, const char *name, const char *value )
{
	size_t result = 0;

	if ( !strcmp( "resource", name ) ) {
		const char *mlt_service = mlt_properties_get( properties, "mlt_service" );
		const char *plain = "plain:";
		size_t plain_len = strlen( plain );

		if ( mlt_service && !strcmp( "timewarp", mlt_service ) ) {
			const char *delimiter = strchr( value, ':' );
			if ( delimiter )
				result = delimiter - value;
			if ( result && ( value[result - 1] == '.' || value[result - 1] == ',' || isdigit(value[result - 1]) ) )
				++result; // include the delimiter
			else
				result = 0; // invalid
		} else if ( !strncmp( value, plain, plain_len ) ) {
			result = plain_len;
		}
	}
	return result;
}


