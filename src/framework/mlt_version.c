/**
 * \file mlt_version.c
 * \brief contains version information
 *
 * Copyright (C) 2014 Meltytech, LLC
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

#include "mlt_version.h"

int mlt_version_get_int( )
{
	return LIBMLT_VERSION_INT;
}

char *mlt_version_get_string( )
{
	return LIBMLT_VERSION;
}

int mlt_version_get_major( )
{
	return LIBMLT_VERSION_MAJOR;
}

int mlt_version_get_minor( )
{
	return LIBMLT_VERSION_MINOR;
}

int mlt_version_get_revision( )
{
	return LIBMLT_VERSION_REVISION;
}


