/**
 * \file mlt_log.c
 * \brief logging functions
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

#include "mlt_log.h"
#include "mlt_service.h"

#include <string.h>

static int log_level = MLT_LOG_WARNING;

void default_callback( void* ptr, int level, const char* fmt, va_list vl )
{
	static int print_prefix = 1;
	mlt_properties properties = ptr ? MLT_SERVICE_PROPERTIES( ( mlt_service )ptr ) : NULL;
	
	if ( level > log_level )
		return;
	if ( print_prefix && properties )
	{
		char *mlt_type = mlt_properties_get( properties, "mlt_type" );
		char *mlt_service = mlt_properties_get( properties, "mlt_service" );
		char *resource = mlt_properties_get( properties, "resource" );
	
		if ( !( resource && *resource && resource[0] == '<' && resource[ strlen(resource) - 1 ] == '>' ) )
			mlt_type = mlt_properties_get( properties, "mlt_type" );
		if ( mlt_service )
			fprintf( stderr, "[%s %s] ", mlt_type, mlt_service );
		else
			fprintf( stderr, "[%s %p] ", mlt_type, ptr );
		if ( resource )
			fprintf( stderr, "%s\n    ", resource );
	}
	print_prefix = strstr( fmt, "\n" ) != NULL;
	vfprintf( stderr, fmt, vl );
}

static void ( *callback )( void*, int, const char*, va_list ) = default_callback;

void mlt_log( void* service, int level, const char *fmt, ...)
{
	va_list vl;
	
	va_start( vl, fmt );
	mlt_vlog( service, level, fmt, vl );
	va_end( vl );
}

void mlt_vlog( void* service, int level, const char *fmt, va_list vl )
{
	if ( callback ) callback( service, level, fmt, vl );
}

int mlt_log_get_level( void )
{
	return log_level;
}

void mlt_log_set_level( int level )
{
	log_level = level;
}

void mlt_log_set_callback( void (*new_callback)( void*, int, const char*, va_list ) )
{
	callback = new_callback;
}
