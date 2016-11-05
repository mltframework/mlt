/*
 * producer_kdenlivetitle.c -- kdenlive producer
 * Copyright (c) 2009 Marco Gittler <g.marco@freenet.de>
 * Copyright (c) 2009 Jean-Baptiste Mardelle <jb@kdenlive.org>
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

#include "kdenlivetitle_wrapper.h"

#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>


void read_xml(mlt_properties properties)
{
	// Convert file name string encoding.
	const char *resource = mlt_properties_get( properties, "resource" );
	mlt_properties_set( properties, "_resource_utf8", resource );
	mlt_properties_from_utf8( properties, "_resource_utf8", "_resource_local8" );
	resource = mlt_properties_get( properties, "_resource_local8" );

	FILE *f = fopen( resource, "r" );
	if ( f != NULL )
	{
		int size = 0;
		long lSize;
 
		if ( fseek (f , 0 , SEEK_END) < 0 )
			goto error;
		lSize = ftell (f);
		if ( lSize <= 0 )
			goto error;
		rewind (f);

		char *infile = (char*) mlt_pool_alloc(lSize);
		if ( infile )
		{
			size = fread(infile,1,lSize,f);
			if ( size )
			{
				infile[size] = '\0';
				mlt_properties_set(properties, "_xmldata", infile);
			}
			mlt_pool_release( infile );
		}
error:
		fclose(f);
	}
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )

{
        int error = 0;
	/* Obtain properties of frame */
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	/* Obtain the producer for this frame */
	producer_ktitle self = mlt_properties_get_data( properties, "producer_kdenlivetitle", NULL );
	
	/* Obtain properties of producer */
	mlt_producer producer = &self->parent;
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );
	
	if ( mlt_properties_get_int( properties, "rescale_width" ) > 0 )
		*width = mlt_properties_get_int( properties, "rescale_width" );
	if ( mlt_properties_get_int( properties, "rescale_height" ) > 0 )
		*height = mlt_properties_get_int( properties, "rescale_height" );
	
	mlt_service_lock( MLT_PRODUCER_SERVICE( producer ) );

	/* Allocate the image */
	if ( mlt_properties_get_int( producer_props, "force_reload" ) ) {
		if ( mlt_properties_get_int( producer_props, "force_reload" ) > 1 ) read_xml( producer_props );
		mlt_properties_set_int( producer_props, "force_reload", 0 );
		drawKdenliveTitle( self, frame, *format, *width, *height, mlt_frame_original_position( frame ), 1 );
	}
	else
	{
		drawKdenliveTitle( self, frame, *format, *width, *height, mlt_frame_original_position( frame ), 0 );
	}
	// Get width and height (may have changed during the refresh)
	*width = mlt_properties_get_int( properties, "width" );
	*height = mlt_properties_get_int( properties, "height" );
	*format = self->format;

	if ( self->current_image )
	{
		// Clone the image and the alpha
		int image_size = mlt_image_format_size( self->format, self->current_width, self->current_height, NULL );
		uint8_t *image_copy = mlt_pool_alloc( image_size );
		// We use height-1 because mlt_image_format_size() uses height + 1.
		// XXX Remove -1 when mlt_image_format_size() is changed.
		memcpy( image_copy, self->current_image,
			mlt_image_format_size( self->format, self->current_width, self->current_height - 1, NULL ) );
		// Now update properties so we free the copy after
		mlt_frame_set_image( frame, image_copy, image_size, mlt_pool_release );
		// We're going to pass the copy on
		*buffer = image_copy;

		// Clone the alpha channel
		if ( self->current_alpha )
		{
			image_copy = mlt_pool_alloc( self->current_width * self->current_height );
			memcpy( image_copy, self->current_alpha, self->current_width * self->current_height );
			mlt_frame_set_alpha( frame, image_copy, self->current_width * self->current_height, mlt_pool_release );
		}
	}
	else
	{
		error = 1;
	}

	mlt_service_unlock( MLT_PRODUCER_SERVICE( producer ) );

	return error;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )

{
  	// Get the real structure for this producer
	producer_ktitle this = producer->child;

	/* Generate a frame */
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	if ( *frame != NULL )
	{
		/* Obtain properties of frame and producer */
		mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

		/* Obtain properties of producer */
		mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

		/* Set the producer on the frame properties */
		mlt_properties_set_data( properties, "producer_kdenlivetitle", this, 0, NULL, NULL );

		/* Update timecode on the frame we're creating */
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		// Set producer-specific frame properties
		mlt_properties_set_int( properties, "progressive", mlt_properties_get_int( producer_props, "progressive" ) );
		double force_ratio = mlt_properties_get_double( producer_props, "force_aspect_ratio" );
		if ( force_ratio > 0.0 )
			mlt_properties_set_double( properties, "aspect_ratio", force_ratio );
		else
			mlt_properties_set_double( properties, "aspect_ratio", mlt_properties_get_double( producer_props, "aspect_ratio" ) );

		/* Push the get_image method */
		mlt_frame_push_get_image( *frame, producer_get_image );
	}

	/* Calculate the next timecode */
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer producer )
{
	producer_ktitle self = producer->child;
	producer->close = NULL;
	mlt_service_cache_purge( MLT_PRODUCER_SERVICE( producer ) );
	mlt_producer_close( producer );
	free( self );
}


mlt_producer producer_kdenlivetitle_init( mlt_profile profile, mlt_service_type type, const char *id, char *filename )
{
	/* Create a new producer object */
	
	producer_ktitle self = calloc( 1, sizeof( struct producer_ktitle_s ) );
	if ( self != NULL && mlt_producer_init( &self->parent, self ) == 0 )
	{
		mlt_producer producer = &self->parent;
		/* Get the properties interface */
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
		/* Callback registration */
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;
		mlt_properties_set( properties, "resource", filename );
		mlt_properties_set_int( properties, "progressive", 1 );
		mlt_properties_set_int( properties, "aspect_ratio", 1 );
		mlt_properties_set_int( properties, "seekable", 1 );
		read_xml(properties);
		return producer;
	}
	free( self );
	return NULL;
}

