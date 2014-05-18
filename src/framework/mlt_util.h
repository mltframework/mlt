/**
 * \file mlt_util.h
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

#ifndef MLT_UTIL_H
#define MLT_UTIL_H

/** Convert UTF-8 string to the locale-defined encoding.
 *
 * MLT uses UTF-8 for strings, but Windows cannot accept UTF-8 for a filename.
 * Windows uses code pages for the locale encoding.
 * \param self a properties list
 * \param name the property to set
 * \param value the property's new value
 * \return true if error
 */

extern int mlt_util_from_utf8( mlt_properties properties, const char *prop_name, const char *prop_name_out );

#endif // MLT_UTIL_H
