/*
 * miracle_log.c -- logging facility implementation
 * Copyright (C) 2002-2003 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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

#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>

#include "miracle_log.h"

static int log_output = log_stderr;
static int threshold = LOG_DEBUG;

void miracle_log_init( enum log_output method, int new_threshold )
{
	log_output = method;
	threshold = new_threshold;
	if (method == log_syslog)
		openlog( "miracle", LOG_CONS, LOG_DAEMON );

}

void miracle_log( int priority, const char *format, ... )
{
	va_list list;
	va_start( list, format );
	if ( LOG_PRI(priority) <= threshold )
	{
		if ( log_output == log_syslog )
		{
				vsyslog( priority, format, list );
		}
		else
		{
			char line[1024];
			if ( snprintf( line, 1024, "(%d) %s\n", priority, format ) != 0 )
				vfprintf( stderr, line, list );
		}
	}
	va_end( list );
}
