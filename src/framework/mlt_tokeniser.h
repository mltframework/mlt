/**
 * \file mlt_tokeniser.h
 * \brief string tokeniser
 * \see mlt_tokeniser_s
 *
 * Copyright (C) 2002-2014 Meltytech, LLC
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

#ifndef MLT_TOKENISER_H
#define MLT_TOKENISER_H

/** \brief Tokeniser class
 *
 */

typedef struct
{
	char *input;
	char **tokens;
	int count;
	int size;
}
*mlt_tokeniser, mlt_tokeniser_t;

/* Remote parser API.
*/

extern mlt_tokeniser mlt_tokeniser_init( );
extern int mlt_tokeniser_parse_new( mlt_tokeniser tokeniser, char *text, const char *delimiter );
extern char *mlt_tokeniser_get_input( mlt_tokeniser tokeniser );
extern int mlt_tokeniser_count( mlt_tokeniser tokeniser );
extern char *mlt_tokeniser_get_string( mlt_tokeniser tokeniser, int index );
extern void mlt_tokeniser_close( mlt_tokeniser tokeniser );

#endif
