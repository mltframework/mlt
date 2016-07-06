/*
 * producer_sdl_image.c -- Image loader which wraps SDL_image
 * Copyright (C) 2005-2014 Meltytech, LLC
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

#include <framework/mlt_producer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_pool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>

#include <SDL_image.h>

static int producer_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	SDL_Surface *surface = mlt_properties_get_data( properties, "surface", NULL );
	SDL_Surface *converted = NULL;

	*width = surface->w;
	*height = surface->h;
	int image_size = *width * *height * 3;

	if ( surface->format->BitsPerPixel != 32 && surface->format->BitsPerPixel != 24 )
	{
		SDL_PixelFormat fmt;
		fmt.BitsPerPixel = 24;
		fmt.BytesPerPixel = 3;
		fmt.Rshift = 16;
		fmt.Gshift = 8;
		fmt.Bshift = 0;
		fmt.Rmask = 0xff << 16;
		fmt.Gmask = 0xff << 8;
		fmt.Bmask = 0xff;
		converted = SDL_ConvertSurface( surface, &fmt, 0 );
	}

	switch( surface->format->BitsPerPixel )
	{
		case 32:
			*format = mlt_image_rgb24a;
			image_size = *width * *height * 4;
			*image = mlt_pool_alloc( image_size );
			memcpy( *image, surface->pixels, image_size );
			break;
		default:
			*format = mlt_image_rgb24;
			*image = mlt_pool_alloc( image_size );
			memcpy( *image, surface->pixels, image_size );
			break;
	}

	if ( converted )
		SDL_FreeSurface( converted );

	// Update the frame
	mlt_frame_set_image( frame, *image, image_size, mlt_pool_release );

	return 0;
}

static int filter_files( const struct dirent *de )
{
	return de->d_name[ 0 ] != '.';
}

static mlt_properties parse_file_names( char *resource )
{
	mlt_properties properties = mlt_properties_new( );

	if ( strstr( resource, "/.all." ) != NULL )
	{
		char *dir_name = strdup( resource );
		char *extension = strrchr( resource, '.' );
		*( strstr( dir_name, "/.all." ) + 1 ) = '\0';
		char fullname[ 1024 ];
		strcpy( fullname, dir_name );
		struct dirent **de = NULL;
		int n = scandir( fullname, &de, filter_files, alphasort );
		int i;
		struct stat info;

		for (i = 0; i < n; i++ )
		{
			snprintf( fullname, 1023, "%s%s", dir_name, de[i]->d_name );
			if ( strstr( fullname, extension ) && lstat( fullname, &info ) == 0 &&
			 	( S_ISREG( info.st_mode ) || info.st_mode | S_IXUSR ) )
			{
				char temp[ 20 ];
				sprintf( temp, "%d", i );
				mlt_properties_set( properties, temp, fullname );
			}
			free( de[ i ] );
		}

		free( de );
		free( dir_name );
	}
	else
	{
		mlt_properties_set( properties, "0", resource );
	}

	return properties;
}

static SDL_Surface *load_image( mlt_producer producer )
{
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
	char *resource = mlt_properties_get( properties, "resource" );
	char *last_resource = mlt_properties_get( properties, "_last_resource" );
	int image_idx = 0;
	char *this_resource = NULL;
	double ttl = mlt_properties_get_int( properties, "ttl" );
	mlt_position position = mlt_producer_position( producer );
	SDL_Surface *surface = mlt_properties_get_data( properties, "_surface", NULL );
	mlt_properties filenames = mlt_properties_get_data( properties, "_filenames", NULL );

	if ( filenames == NULL )
	{
		filenames = parse_file_names( resource );
		mlt_properties_set_data( properties, "_filenames", filenames, 0, ( mlt_destructor )mlt_properties_close, 0 );
		mlt_properties_set_data( properties, "_surface", surface, 0, ( mlt_destructor )SDL_FreeSurface, 0 );
	}

	if ( mlt_properties_count( filenames ) ) 
	{
		image_idx = ( int )floor( ( double )position / ttl ) % mlt_properties_count( filenames );
		this_resource = mlt_properties_get_value( filenames, image_idx );

		if ( surface == NULL || last_resource == NULL || strcmp( last_resource, this_resource ) )
		{
			surface = IMG_Load( this_resource );
			if ( surface != NULL )
			{
				surface->refcount ++;
				mlt_properties_set_data( properties, "_surface", surface, 0, ( mlt_destructor )SDL_FreeSurface, 0 );
				mlt_properties_set( properties, "_last_resource", this_resource );
				mlt_properties_set_int( properties, "meta.media.width", surface->w );
				mlt_properties_set_int( properties, "meta.media.height", surface->h );
			}
		}
		else if ( surface != NULL )
		{
			surface->refcount ++;
		}
	}

	return surface;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	if ( *frame != NULL )
	{
		// Create the surface for the current image
		SDL_Surface *surface = load_image( producer );

		if ( surface != NULL )
		{
			// Obtain properties of frame and producer
			mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

			// Obtain properties of producer
			mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

			// Update timecode on the frame we're creating
			mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

			// Set producer-specific frame properties
			mlt_properties_set_int( properties, "progressive", 1 );
			mlt_properties_set_double( properties, "aspect_ratio", mlt_properties_get_double( producer_props, "aspect_ratio" ) );
			mlt_properties_set_data( properties, "surface", surface, 0, ( mlt_destructor )SDL_FreeSurface, NULL );

			// Push the get_image method
			mlt_frame_push_get_image( *frame, producer_get_image );
		}
	}

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer producer )
{
	producer->close = NULL;
	mlt_producer_close( producer );
	free( producer );
}

mlt_producer producer_sdl_image_init( mlt_profile profile, mlt_service_type type, const char *id, char *file )
{
	mlt_producer producer = calloc( 1, sizeof( struct mlt_producer_s ) );
	if ( producer != NULL && mlt_producer_init( producer, NULL ) == 0 )
	{
		// Get the properties interface
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
	
		// Callback registration
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;

		// Set the default properties
		mlt_properties_set( properties, "resource", file );
		mlt_properties_set( properties, "_resource", "" );
		mlt_properties_set_double( properties, "aspect_ratio", 1 );
		mlt_properties_set_int( properties, "ttl", 25 );
		mlt_properties_set_int( properties, "progressive", 1 );
		
		// Validate the resource
		SDL_Surface *surface = NULL;
		if ( file && ( surface = load_image( producer ) ) )
		{
			SDL_FreeSurface( surface );
			mlt_properties_set_data( properties, "_surface", NULL, 0, NULL, NULL );
		}
		else
		{
			producer_close( producer );
			producer = NULL;
		}
		return producer;
	}
	free( producer );
	return NULL;
}


