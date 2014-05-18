/**
 * \file mlt_util.c
 * \brief MLT utility functions
 *
 * Copyright (C) 2014 Ushodaya Enterprises Limited
 * \author Dan Dennedy <dan@dennedy.org>
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

#include "mlt_util.h"
#include "mlt_properties.h"

#ifndef WIN32
// On non-Windows platforms, assume UTF-8 will always work and does not need conversion.
// This function just becomes a pass-through operation.
// This was largely chosen to prevent adding a libiconv dependency to the framework per policy.
// However, for file open operations on Windows, especially when processing XML, a text codec
// dependency is hardly avoidable.
int mlt_util_from_utf8( mlt_properties properties, const char *prop_name, const char *prop_name_out )
{
	return mlt_properties_set( properties, prop_name_out, mlt_properties_get( properties, prop_name ) );
}
#endif
