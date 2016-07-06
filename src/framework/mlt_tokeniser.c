/**
 * \file mlt_tokeniser.c
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

/* System header files */
#include <stdlib.h>
#include <string.h>

/* Application header files */
#include "mlt_tokeniser.h"

/** Initialise a tokeniser.
*/

mlt_tokeniser mlt_tokeniser_init( )
{
	mlt_tokeniser tokeniser = calloc( 1, sizeof( mlt_tokeniser_t ) );
	tokeniser->input = NULL;
	tokeniser->tokens = NULL;
	tokeniser->count = 0;
	tokeniser->size = 0;
	return tokeniser;
}

/** Clear the tokeniser.
*/

static void mlt_tokeniser_clear( mlt_tokeniser tokeniser )
{
	int index = 0;
	for ( index = 0; index < tokeniser->count; index ++ )
		free( tokeniser->tokens[ index ] );
	tokeniser->count = 0;
	free( tokeniser->input );
	tokeniser->input = NULL;
}

/** Append a string to the tokeniser.
*/

static int mlt_tokeniser_append( mlt_tokeniser tokeniser, char *token )
{
	int error = 0;

	if ( tokeniser->count == tokeniser->size )
	{
		tokeniser->size += 20;
		tokeniser->tokens = realloc( tokeniser->tokens, tokeniser->size * sizeof( char * ) );
	}

	if ( tokeniser->tokens != NULL )
	{
		tokeniser->tokens[ tokeniser->count ++ ] = strdup( token );
	}
	else
	{
		tokeniser->count = 0;
		error = -1;
	}
	return error;
}

/** Parse a string by splitting on the delimiter provided.
*/

int mlt_tokeniser_parse_new( mlt_tokeniser tokeniser, char *string, const char *delimiter )
{
	if ( !string || !delimiter ) return 0;
	int count = 0;
	int length = strlen( string );
	int delimiter_size = strlen( delimiter );
	int index = 0;
	char *token = strdup( string );

	mlt_tokeniser_clear( tokeniser );
	tokeniser->input = strdup( string );
	strcpy( token, "" );

	for ( index = 0; index < length; )
	{
		char *start = string + index;
		char *end = strstr( start, delimiter );

		if ( end == NULL )
		{
			strcat( token, start );
			mlt_tokeniser_append( tokeniser, token );
			index = length;
			count ++;
		}
		else if ( start != end )
		{
			strncat( token, start, end - start );
			index += end - start;
			if ( strchr( token, '\"' ) == NULL || token[ strlen( token ) - 1 ] == '\"' )
			{
				mlt_tokeniser_append( tokeniser, token );
				strcpy( token, "" );
				count ++;
			}
			else while ( strncmp( string + index, delimiter, delimiter_size ) == 0 )
			{
				strncat( token, delimiter, delimiter_size );
				index += delimiter_size;
			}
		}
		else
		{
			index += delimiter_size;
		}
	}

	/* Special case - malformed string condition */
	if ( !strcmp( token, "" ) )
	{
		count = 0 - ( count - 1 );
		mlt_tokeniser_append( tokeniser, token );
	}

	free( token );
	return count;
}

/** Get the original input.
*/

char *mlt_tokeniser_get_input( mlt_tokeniser tokeniser )
{
	return tokeniser->input;
}

/** Get the number of tokens.
*/

int mlt_tokeniser_count( mlt_tokeniser tokeniser )
{
	return tokeniser->count;
}

/** Get a token as a string.
*/

char *mlt_tokeniser_get_string( mlt_tokeniser tokeniser, int index )
{
	if ( index < tokeniser->count )
		return tokeniser->tokens[ index ];
	else
		return NULL;
}

/** Close the tokeniser.
*/

void mlt_tokeniser_close( mlt_tokeniser tokeniser )
{
	mlt_tokeniser_clear( tokeniser );
	free( tokeniser->tokens );
	free( tokeniser );
}
