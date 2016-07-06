/*
 * producer_swfdec.c -- swfdec producer for Flash files
 * Copyright (C) 2010 Dan Dennedy <dan@dennedy.org>
 *
 * swfdec library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * swfdec library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with swfdec library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt.h>
#include <swfdec/swfdec.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

typedef struct
{
	struct mlt_producer_s parent;
    SwfdecPlayer *player;
    SwfdecURL *url;
    cairo_surface_t *surface;
    cairo_t *cairo;
    mlt_position last_position;
	guint width;
	guint height;
} *producer_swfdec;

void swfdec_open( producer_swfdec swfdec, mlt_profile profile )
{
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( &swfdec->parent );

	// Setup the swfdec player
	swfdec->player = swfdec_player_new( NULL );
	if ( mlt_properties_get( properties, "variables") )
		swfdec_player_set_variables( swfdec->player, mlt_properties_get( properties, "variables" ) );
	swfdec_player_set_url( swfdec->player, swfdec->url );
	swfdec_player_set_maximum_runtime( swfdec->player, 10000 );

	// Setup size
	swfdec_player_get_default_size( swfdec->player, &swfdec->width, &swfdec->height );
	if ( swfdec->width == 0 || swfdec->height == 0 )
	{
		swfdec_player_set_size( swfdec->player, profile->width, profile->height );
		swfdec->width = profile->width;
		swfdec->height = profile->height;
	}

	// Setup scaling
	double scale = 1.0;
	if ( swfdec->width > 2 * swfdec->height )
		scale = 0.5 * profile->width / swfdec->height;
	else if ( swfdec->height > 2 * swfdec->width )
		scale = 0.5 * profile->height / swfdec->width;
	else
		scale = (double) profile->width / MAX( swfdec->width, swfdec->height );
	swfdec->width = ceil( scale * swfdec->width );
	swfdec->height = ceil( scale * swfdec->height );

	// Compute the centering translation
	double x = swfdec->width > profile->width ? (swfdec->width - profile->width) / 2 : 0;
	double y = swfdec->height > profile->height ? (swfdec->height - profile->height) / 2 : 0;

	// Setup cairo
	swfdec->surface = cairo_image_surface_create( CAIRO_FORMAT_ARGB32, MIN(profile->width, swfdec->width), MIN(profile->height, swfdec->height) );
	swfdec->cairo = cairo_create( swfdec->surface );
	cairo_translate( swfdec->cairo, -x, -y );
	cairo_scale( swfdec->cairo, scale, scale );
}

void swfdec_close( producer_swfdec swfdec )
{
	if ( swfdec->cairo )
		cairo_destroy( swfdec->cairo );
	swfdec->cairo = NULL;
	if ( swfdec->player )
		g_object_unref( swfdec->player );
	swfdec->player = NULL;
	if ( swfdec->surface )
		cairo_surface_destroy( swfdec->surface );
	swfdec->surface = NULL;
}

// Cairo uses 32 bit native endian ARGB
static void bgra_to_rgba( uint8_t *src, uint8_t* dst, int width, int height )
{
	int n = width * height + 1;

	while ( --n )
	{
		*dst++ = src[2];
		*dst++ = src[1];
		*dst++ = src[0];
		*dst++ = src[3];
		src += 4;
	}	
}

static int get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	producer_swfdec swfdec = mlt_frame_pop_service( frame );
	mlt_service service = MLT_PRODUCER_SERVICE( &swfdec->parent );
	mlt_profile profile = mlt_service_profile( service );

	mlt_service_lock( service );
	
	if ( !swfdec->player )
		swfdec_open( swfdec, profile );

	// Set width and height
	*width = swfdec->width;
	*height = swfdec->height;
	*format = mlt_image_rgb24a;

	*buffer = mlt_pool_alloc( *width * ( *height + 1 ) * 4 );
	mlt_frame_set_image( frame, *buffer, *width * ( *height + 1 ) * 4, mlt_pool_release );

	// Seek
	mlt_position pos = mlt_frame_original_position( frame );
	if ( pos > swfdec->last_position )
	{
		gulong msec = 1000UL * ( pos - swfdec->last_position ) * profile->frame_rate_den / profile->frame_rate_num;
		while ( msec > 0 )
			msec -= swfdec_player_advance( swfdec->player, msec );
	}
	else if ( pos < swfdec->last_position )
	{
		swfdec_close( swfdec );
		swfdec_open( swfdec, mlt_service_profile( service ) );
		gulong msec = 1000UL * pos * profile->frame_rate_den / profile->frame_rate_num;
		while ( msec > 0 )
			msec -= swfdec_player_advance( swfdec->player, msec );
	}
	swfdec->last_position = pos;

	// Render
	cairo_save( swfdec->cairo );
	//cairo_set_source_rgba( swfdec->cairo, r, g, b, a );
	cairo_set_operator( swfdec->cairo, CAIRO_OPERATOR_CLEAR );
	cairo_paint( swfdec->cairo );
	cairo_restore( swfdec->cairo );
	swfdec_player_render( swfdec->player, swfdec->cairo );

	// Get image from surface
	uint8_t *image = cairo_image_surface_get_data( swfdec->surface );
	
	mlt_service_unlock( service );
	
	// Convert to RGBA
	bgra_to_rgba( image, *buffer, swfdec->width, swfdec->height );

	return 0;
}

static int get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	// Access the private data
	producer_swfdec swfdec = producer->child;

	// Create an empty frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	// Get the frames properties
	mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

	// Update other info on the frame
	mlt_properties_set_int( properties, "test_image", 0 );
	mlt_properties_set_int( properties, "width", swfdec->width );
	mlt_properties_set_int( properties, "height", swfdec->height );
	mlt_properties_set_int( properties, "progressive", 1 );
	mlt_properties_set_double( properties, "aspect_ratio", 1.0 );

	// Push the get_image method on to the stack
	mlt_frame_push_service( *frame, swfdec );
	mlt_frame_push_get_image( *frame, get_image );
	
	// Update timecode on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	// Obtain swfdec
	producer_swfdec swfdec = parent->child;

	// Close the file
	swfdec_close( swfdec );
	if ( swfdec->url )
		swfdec_url_free( swfdec->url );

	// Close the parent
	parent->close = NULL;
	mlt_producer_close( parent );

	// Free the memory
	free( swfdec );
}

mlt_producer producer_swfdec_init( mlt_profile profile, mlt_service_type type, const char *id, char *filename )
{
	if ( !filename ) return NULL;
	producer_swfdec swfdec = calloc( 1, sizeof( *swfdec ) );
	mlt_producer producer = NULL;

	if ( swfdec && mlt_producer_init( &swfdec->parent, swfdec ) == 0 )
	{
		// Initialize swfdec and try to open the file
		swfdec->url = swfdec_url_new_from_input( filename );
		if ( swfdec->url )
		{
			// Set the return value
			producer = &swfdec->parent;

			// Set the resource property (required for all producers)
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );
			mlt_properties_set( properties, "resource", filename );

			// Set the callbacks
			producer->close = (mlt_destructor) producer_close;
			producer->get_frame = get_frame;

			// Set the meta media attributes
			swfdec->width = profile->width;
			swfdec->height = profile->height;
			mlt_properties_set_int( properties, "meta.media.nb_streams", 1 );
			mlt_properties_set( properties, "meta.media.0.stream.type", "video" );
			mlt_properties_set( properties, "meta.media.0.codec.name", "swf" );
			mlt_properties_set( properties, "meta.media.0.codec.long_name", "Adobe Flash" );
			mlt_properties_set( properties, "meta.media.0.codec.pix_fmt", "bgra" );
			mlt_properties_set_int( properties, "meta.media.width", profile->width );
			mlt_properties_set_int( properties, "meta.media.height", profile->height );
			mlt_properties_set_double( properties, "meta.media.sample_aspect_num", 1.0 );
			mlt_properties_set_double( properties, "meta.media.sample_aspect_den", 1.0 );
			mlt_properties_set_int( properties, "meta.media.frame_rate_num", profile->frame_rate_num );
			mlt_properties_set_int( properties, "meta.media.frame_rate_den", profile->frame_rate_den );
			mlt_properties_set_int( properties, "meta.media.progressive", 1 );
		}
		else
		{
			g_object_unref( swfdec->player );
			mlt_producer_close( &swfdec->parent );
			free( swfdec );
		}
	}
	else
	{
		free( swfdec );
	}

	return producer;
}

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/swfdec/%s", mlt_environment( "MLT_DATA" ), (char*) data );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	swfdec_init();
	MLT_REGISTER( producer_type, "swfdec", producer_swfdec_init );
	MLT_REGISTER_METADATA( producer_type, "swfdec", metadata, "producer_swfdec.yml" );
}
