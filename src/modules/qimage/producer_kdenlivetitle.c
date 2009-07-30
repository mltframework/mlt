/*
 * producer_kdenlivetitle.c -- kdenlive producer
 * Copyright (c) 2009 Marco Gittler <g.marco@freenet.de>
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

#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>


extern void refresh_kdenlivetitle( mlt_producer producer, uint8_t*, int, int, double, int );

void read_xml(mlt_properties properties)
{
	FILE *f = fopen( mlt_properties_get( properties, "resource" ), "r");
	if ( f != NULL )
	{
		int size = 0;
		long lSize;
 
		fseek (f , 0 , SEEK_END);
		lSize = ftell (f);
		rewind (f);

		char *infile = (char*) mlt_pool_alloc(lSize);
		size=fread(infile,1,lSize,f);
		infile[size] = '\0';
		fclose(f);
		mlt_properties_set(properties, "xmldata", infile);
		mlt_pool_release( infile );
	}
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	/* Obtain properties of frame */
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	/* Obtain the producer for this frame */
	mlt_producer producer = mlt_properties_get_data( properties, "producer_kdenlivetitle", NULL );

	/* Obtain properties of producer */
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );
	
	*width = mlt_properties_get_int( properties, "rescale_width" );
	*height = mlt_properties_get_int( properties, "rescale_height" );
	
	/* Allocate the image */
	int size = *width * ( *height ) * 4;

	*buffer = mlt_pool_alloc( size );

	mlt_properties_set_int( properties, "width", *width );
	mlt_properties_set_int( properties, "height", *height );

	/* cache later ?? */

	if ( 1 )
	{
		/* Allocate the image */
		*format = mlt_image_rgb24a;
		mlt_position in = mlt_producer_get_in( producer );
		mlt_position out = mlt_producer_get_out( producer );
		mlt_position time = mlt_producer_position( producer );
		double position = ( double )( time - in ) / ( double )( out - in + 1 );
		if ( mlt_properties_get_int( producer_props, "force_reload" ) ) {
			if (mlt_properties_get_int( producer_props, "force_reload" ) > 1) read_xml(producer_props);
			mlt_properties_set_int( producer_props, "force_reload", 0 );
			refresh_kdenlivetitle( producer, *buffer, *width, *height, position, 1);
		}
		else refresh_kdenlivetitle( producer, *buffer, *width, *height, position, 0);
		/* Update the frame */
		mlt_properties_set_data( properties, "image", *buffer, size, mlt_pool_release, NULL );
		
		mlt_log_debug( MLT_PRODUCER_SERVICE( producer ), "width:%d height:%d %s\n", *width, *height, mlt_image_format_name( *format ) );
	}

	return 0;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )

{

	/* Generate a frame */
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	if ( *frame != NULL )
	{
		/* Obtain properties of frame and producer */
		mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

		/* Obtain properties of producer */
		mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

		/* Set the producer on the frame properties */
		mlt_properties_set_data( properties, "producer_kdenlivetitle", producer, 0, NULL, NULL );

		/* Update timecode on the frame we're creating */
		mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

		/* Set producer-specific frame properties */
		mlt_properties_set_int( properties, "progressive", 1 );
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
	/* fprintf(stderr, "::::::::::::::  CLOSING TITLE\n"); */
	producer->close = NULL;
	mlt_producer_close( producer );
	free( producer );
}


mlt_producer producer_kdenlivetitle_init( mlt_profile profile, mlt_service_type type, const char *id, char *filename )
{
  	/* fprintf(stderr, ":::::::::::: CREATE TITLE\n"); */
	/* Create a new producer object */

	mlt_producer producer = calloc( 1, sizeof( struct mlt_producer_s ) );
	if ( producer != NULL && mlt_producer_init( producer, NULL ) == 0 )
	{
		/* Get the properties interface */
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
		/* Callback registration */
		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;
		mlt_properties_set( properties, "resource", filename );
		mlt_properties_set_int( properties, "aspect_ratio", 1 );
		read_xml(properties);
		return producer;
	}
	free( producer );
	return NULL;
}

