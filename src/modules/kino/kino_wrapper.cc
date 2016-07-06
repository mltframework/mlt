/*
 * kino_wrapper.cc -- c wrapper for kino file handler
 * Copyright (C) 2005-2014 Meltytech, LLC
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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <cstring>
#include <cstdlib>

#include "kino_wrapper.h"
#include "filehandler.h"

extern "C" 
{

#include <framework/mlt_pool.h>

struct kino_wrapper_s
{
	FileHandler *handler;
	int is_pal;
};

kino_wrapper kino_wrapper_init( )
{
	kino_wrapper self = ( kino_wrapper )malloc( sizeof( kino_wrapper_s ) );
	if ( self != NULL )
		self->handler = NULL;
	return self;
}

int kino_wrapper_open( kino_wrapper self, char *src )
{
	if ( self != NULL )
	{
		// Rough file determination based on file type
		if ( strncasecmp( strrchr( src, '.' ), ".avi", 4 ) == 0 )
			self->handler = new AVIHandler( );
		else if ( strncasecmp( strrchr( src, '.' ), ".dv", 3 ) == 0 || strncasecmp( strrchr( src, '.' ), ".dif", 4 ) == 0 )
			self->handler = new RawHandler( );
		#ifdef HAVE_LIBQUICKTIME
		else if ( strncasecmp( strrchr( src, '.' ), ".mov", 4 ) == 0 )
			self->handler = new QtHandler( );
		#endif

		// Open the file if we have a handler
		if ( self->handler != NULL )
			if ( !self->handler->Open( src ) )
				self = NULL;

		// Check the first frame to see if it's PAL or NTSC
		if ( self != NULL && self->handler != NULL )
		{
			uint8_t *data = ( uint8_t * )mlt_pool_alloc( 144000 );
			if ( self->handler->GetFrame( data, 0 ) == 0 )
				self->is_pal = data[3] & 0x80;
			else
				self = NULL;
			mlt_pool_release( data );
		}
	}

	return kino_wrapper_is_open( self );
}

int kino_wrapper_get_frame_count( kino_wrapper self )
{
	return self != NULL && self->handler != NULL ? self->handler->GetTotalFrames( ) : 0;
}

int kino_wrapper_is_open( kino_wrapper self )
{
	return self != NULL && self->handler != NULL ? self->handler->FileIsOpen( ) : 0;
}

int kino_wrapper_is_pal( kino_wrapper self )
{
	return self != NULL ? self->is_pal : 0;
}

int kino_wrapper_get_frame( kino_wrapper self, uint8_t *data, int index )
{
	return self != NULL && self->handler != NULL ? !self->handler->GetFrame( data, index ) : 0;
}

void kino_wrapper_close( kino_wrapper self )
{
	if ( self )
		delete self->handler;
	free( self );
}

}


