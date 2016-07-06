/**
 * \file mlt_parser.h
 * \brief service parsing functionality
 * \see mlt_parser_s
 *
 * Copyright (C) 2003-2014 Meltytech, LLC
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

#ifndef MLT_PARSER_H
#define MLT_PARSER_H

#include "mlt_types.h"

/** \brief Parser class
 *
 * \extends mlt_properties_s
 */

struct mlt_parser_s
{
	struct mlt_properties_s parent;
	int ( *on_invalid )( mlt_parser self, mlt_service object );
	int ( *on_unknown )( mlt_parser self, mlt_service object );
	int ( *on_start_producer )( mlt_parser self, mlt_producer object );
	int ( *on_end_producer )( mlt_parser self, mlt_producer object );
	int ( *on_start_playlist )( mlt_parser self, mlt_playlist object );
	int ( *on_end_playlist )( mlt_parser self, mlt_playlist object );
	int ( *on_start_tractor )( mlt_parser self, mlt_tractor object );
	int ( *on_end_tractor )( mlt_parser self, mlt_tractor object );
	int ( *on_start_multitrack )( mlt_parser self, mlt_multitrack object );
	int ( *on_end_multitrack )( mlt_parser self, mlt_multitrack object );
	int ( *on_start_track )( mlt_parser self );
	int ( *on_end_track )( mlt_parser self );
	int ( *on_start_filter )( mlt_parser self, mlt_filter object );
	int ( *on_end_filter )( mlt_parser self, mlt_filter object );
	int ( *on_start_transition )( mlt_parser self, mlt_transition object );
	int ( *on_end_transition )( mlt_parser self, mlt_transition object );
};

extern mlt_parser mlt_parser_new( );
extern mlt_properties mlt_parser_properties( mlt_parser self );
extern int mlt_parser_start( mlt_parser self, mlt_service object );
extern void mlt_parser_close( mlt_parser self );

#endif
