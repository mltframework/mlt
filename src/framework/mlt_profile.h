/**
 * \file mlt_profile.h
 * \brief video output definition
 *
 * Copyright (C) 2007-2008 Ushodaya Enterprises Limited
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

#ifndef _MLT_PROFILE_H
#define _MLT_PROFILE_H

#include "mlt_types.h"

/** \brief Profile class
 *
 */

struct mlt_profile_s
{
	char* description;
	int frame_rate_num;
	int frame_rate_den;
	int width;
	int height;
	int progressive;
	int sample_aspect_num;
	int sample_aspect_den;
	int display_aspect_num;
	int display_aspect_den;
};

extern mlt_profile mlt_profile_init( const char *name );
extern mlt_profile mlt_profile_load_file( const char *file );
extern mlt_profile mlt_profile_load_properties( mlt_properties properties );
extern mlt_profile mlt_profile_load_string( const char *string );
extern double mlt_profile_fps( mlt_profile profile );
extern double mlt_profile_sar( mlt_profile profile );
extern double mlt_profile_dar( mlt_profile profile );
extern void mlt_profile_close( mlt_profile profile );
#endif
