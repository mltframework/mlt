/**
 * \file mlt_version.h
 * \brief contains version information
 *
 * Copyright (C) 2010 Ushodaya Enterprises Limited
 * \author Jonathan Thomas <Jonathan.Oomph@gmail.com>
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

#ifndef _MLT_VERSION_H_
#define _MLT_VERSION_H_

// Add quotes around any #define variables
#define STRINGIZE2(s)           #s
#define STRINGIZE(s)            STRINGIZE2(s)

#define LIBMLT_VERSION_MAJOR    0
#define LIBMLT_VERSION_MINOR    7
#define LIBMLT_VERSION_REVISION 2
#define LIBMLT_VERSION_INT      ((LIBMLT_VERSION_MAJOR<<16)+(LIBMLT_VERSION_MINOR<<8)+LIBMLT_VERSION_REVISION)
#define LIBMLT_VERSION          STRINGIZE(LIBMLT_VERSION_MAJOR.LIBMLT_VERSION_MINOR.LIBMLT_VERSION_REVISION)

extern int mlt_version_get_int( );
extern int mlt_version_get_major( );
extern int mlt_version_get_minor( );
extern int mlt_version_get_revision( );
extern char *mlt_version_get_string( );

#endif
