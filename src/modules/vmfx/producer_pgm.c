/*
 * producer_pgm.c -- PGM producer
 * Copyright (C) 2005 Visual Media Fx Inc.
 * Author: Charles Yates <charles.yates@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_producer.h>
#include <framework/mlt_frame.h>
#include <stdlib.h>
#include <string.h>

static int read_pgm( char *name, uint8_t **image, int *width, int *height, int *maxval );
static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );

mlt_producer producer_pgm_init( mlt_profile profile, mlt_service_type type, const char *id, char *resource )
{
	mlt_producer this = NULL;
	uint8_t *image = NULL;
	int width = 0;
	int height = 0;
	int maxval = 0;

	// Convert file name string encoding.
	mlt_properties tmp_properties = mlt_properties_new();
	mlt_properties_set( tmp_properties, "utf8", resource );
	mlt_properties_from_utf8( tmp_properties, "utf8", "local8" );
	resource = mlt_properties_get( tmp_properties, "local8" );

	if ( read_pgm( resource, &image, &width, &height, &maxval ) == 0 )
	{
		this = calloc( 1, sizeof( struct mlt_producer_s ) );
		if ( this != NULL && mlt_producer_init( this, NULL ) == 0 )
		{
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );
			this->get_frame = producer_get_frame;
			this->close = ( mlt_destructor )producer_close;
			mlt_properties_set( properties, "resource", resource );
			mlt_properties_set_data( properties, "image", image, 0, mlt_pool_release, NULL );
			mlt_properties_set_int( properties, "meta.media.width", width );
			mlt_properties_set_int( properties, "meta.media.height", height );
		}
		else
		{
			mlt_pool_release( image );
			free( this );
			this = NULL;
		}
	}
	mlt_properties_close( tmp_properties );

	return this;
}

/** Load the PGM file.
*/

static int read_pgm( char *name, uint8_t **image, int *width, int *height, int *maxval )
{
	uint8_t *input = NULL;
	int error = 0;
	FILE *f = fopen( name, "rb" );
	char data[ 512 ];

	// Initialise
	*image = NULL;
	*width = 0;
	*height = 0;
	*maxval = 0;

	// Get the magic code
	if ( f != NULL && fgets( data, 511, f ) != NULL && data[ 0 ] == 'P' && data[ 1 ] == '5' )
	{
		char *p = data + 2;
		int i = 0;
		int val = 0;

		// PGM Header parser (probably needs to be strengthened)
		for ( i = 0; !error && i < 3; i ++ )
		{
			if ( *p != '\0' && *p != '\n' )
				val = strtol( p, &p, 10 );
			else
				p = NULL;

			while ( error == 0 && p == NULL )
			{
				if ( fgets( data, 511, f ) == NULL )
					error = 1;
				else if ( data[ 0 ] != '#' )
					val = strtol( data, &p, 10 );
			}

			switch( i )
			{
				case 0: *width = val; break;
				case 1: *height = val; break;
				case 2: *maxval = val; break;
			}
		}
		
		if ( !error )
		{
			// Determine if this is one or two bytes per pixel
			int bpp = *maxval > 255 ? 2 : 1;
			int size = *width * *height * bpp;
			uint8_t *p;

			// Allocate temporary storage for the data and the image
			input = mlt_pool_alloc( *width * *height * bpp );
			*image = mlt_pool_alloc( *width * *height * sizeof( uint8_t ) * 2 );
			p = *image;

			error = *image == NULL || input == NULL;

			if ( !error )
			{
				// Read the raw data
				error = fread( input, *width * *height * bpp, 1, f ) != 1;

				if ( !error )
				{
					// Convert to yuv422 (very lossy - need to extend this to allow 16 bit alpha out)
					for ( i = 0; i < size; i += bpp )
					{
						*p ++ = 16 + ( input[ i ] * 219 ) / 255;
						*p ++ = 128;
					}
				}
			}
		}

		if ( error )
			mlt_pool_release( *image );
		mlt_pool_release( input );
	}
	else
	{
		error = 1;
	}

	if ( f != NULL )
		fclose( f );

	return error;
}

static int producer_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_producer producer = mlt_frame_pop_service( this );
	int real_width = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "meta.media.width" );
	int real_height = mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "meta.media.height" );
	int size = real_width * real_height;
	uint8_t *image = mlt_pool_alloc( size * 2 );
	uint8_t *source = mlt_properties_get_data( MLT_PRODUCER_PROPERTIES( producer ), "image", NULL );

	mlt_frame_set_image( this, image, size * 2, mlt_pool_release );

	*width = real_width;
	*height = real_height;
	*format = mlt_image_yuv422;
	*buffer = image;

	if ( image != NULL && source != NULL )
		memcpy( image, source, size * 2 );

	return 0;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Construct a test frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	// Get the frames properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

	// Pass the data on the frame properties
	mlt_properties_set_int( properties, "has_image", 1 );
	mlt_properties_set_int( properties, "progressive", 1 );
	mlt_properties_set_double( properties, "aspect_ratio", 1 );

	// Push the image callback
	mlt_frame_push_service( *frame, producer );
	mlt_frame_push_get_image( *frame, producer_get_image );

	// Update timecode on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	parent->close = NULL;
	mlt_producer_close( parent );
	free( parent );
}
