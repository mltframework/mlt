/*
 * producer_pango.c -- a pango-based titler
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "producer_pango.h"
#include <framework/mlt_frame.h>
#include <stdlib.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pangoft2.h>
#include <freetype/freetype.h>

// special color type used by internal pango routines
typedef struct
{
	uint8_t r, g, b, a;
} rgba_color;

// Forward declarations
static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );
static void pango_draw_background( GdkPixbuf *pixbuf, rgba_color bg );
static GdkPixbuf *pango_get_pixbuf( const char *markup, rgba_color fg, rgba_color bg, int pad, int align );

mlt_producer producer_pango_init( const char *markup )
{
	producer_pango this = calloc( sizeof( struct producer_pango_s ), 1 );
	if ( this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		mlt_producer producer = &this->parent;

		producer->get_frame = producer_get_frame;
		producer->close = producer_close;

		this->markup = strdup( markup );
		g_type_init();

		// Get the properties interface
		mlt_properties properties = mlt_producer_properties( &this->parent );

		// Set the default properties
		mlt_properties_set_int( properties, "video_standard", mlt_video_standard_pal );
		mlt_properties_set_int( properties, "fgcolor", 0xffffffff );
		mlt_properties_set_int( properties, "bgcolor", 0x00000000 );
		mlt_properties_set_int( properties, "align", pango_align_left );
		mlt_properties_set_int( properties, "pad", 0 );

		return producer;
	}
	free( this );
	return NULL;
}

static int producer_get_image( mlt_frame this, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( this );

	// May need to know the size of the image to clone it
	int size = 0;

	// Get the image
	uint8_t *image = mlt_properties_get_data( properties, "image", &size );

	// Get width and height
	*width = mlt_properties_get_int( properties, "width" );
	*height = mlt_properties_get_int( properties, "height" );

	// Clone if necessary
	if ( writable )
	{
		// Clone our image
		uint8_t *copy = malloc( size );
		memcpy( copy, image, size );

		// We're going to pass the copy on
		image = copy;

		// Now update properties so we free the copy after
		mlt_properties_set_data( properties, "image", copy, size, free, NULL );
	}

	// Pass on the image
	*buffer = image;

	return 0;
}

static uint8_t *producer_get_alpha_mask( mlt_frame this )
{
	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( this );

	// Return the alpha mask
	return mlt_properties_get_data( properties, "alpha", NULL );
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	producer_pango this = producer->child;
	GdkPixbuf *pixbuf = NULL;

	// Generate a frame
	*frame = mlt_frame_init( );

	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( *frame );

    // optimization for subsequent iterations on single picture
	if ( this->image != NULL )
	{
		// Set width/height
		mlt_properties_set_int( properties, "width", this->width );
		mlt_properties_set_int( properties, "height", this->height );

		// if picture sequence pass the image and alpha data without destructor
		mlt_properties_set_data( properties, "image", this->image, 0, NULL, NULL );
		mlt_properties_set_data( properties, "alpha", this->alpha, 0, NULL, NULL );

		// Set alpha mask call back
        ( *frame )->get_alpha_mask = producer_get_alpha_mask;

		// Stack the get image callback
		mlt_frame_push_get_image( *frame, producer_get_image );

	}
	else
	{
		// Obtain properties of producer
		mlt_properties props = mlt_producer_properties( producer );

		// Get properties
		int fg = mlt_properties_get_int( props, "fgcolor" );
		int bg = mlt_properties_get_int( props, "bgcolor" );
		int align = mlt_properties_get_int( props, "align" );
		int pad = mlt_properties_get_int( props, "pad" );
		rgba_color fgcolor =
		{
			( fg & 0xff000000 ) >> 24,
			( fg & 0x00ff0000 ) >> 16,
			( fg & 0x0000ff00 ) >> 8,
			( fg & 0x000000ff )
		};
		rgba_color bgcolor =
		{
			( bg & 0xff000000 ) >> 24,
			( bg & 0x00ff0000 ) >> 16,
			( bg & 0x0000ff00 ) >> 8,
			( bg & 0x000000ff )
		};
		
		// Render the title
		pixbuf = pango_get_pixbuf( this->markup, fgcolor, bgcolor, pad, align );
	}

	// If we have a pixbuf
	if ( pixbuf )
	{
		// Scale to adjust for sample aspect ratio
		if ( mlt_properties_get_int( properties, "video_standard" ) == mlt_video_standard_pal )
		{
			GdkPixbuf *temp = pixbuf;
			GdkPixbuf *scaled = gdk_pixbuf_scale_simple( pixbuf,
				(gint) ( (float) gdk_pixbuf_get_width( pixbuf ) * 54.0/59.0),
				gdk_pixbuf_get_height( pixbuf ), GDK_INTERP_HYPER );
			pixbuf = scaled;
			g_object_unref( temp );
		}
		else
		{
			GdkPixbuf *temp = pixbuf;
			GdkPixbuf *scaled = gdk_pixbuf_scale_simple( pixbuf,
				(gint) ( (float) gdk_pixbuf_get_width( pixbuf ) * 11.0/10.0 ),
				gdk_pixbuf_get_height( pixbuf ), GDK_INTERP_HYPER );
			pixbuf = scaled;
			g_object_unref( temp );
		}

		// Store width and height
		this->width = gdk_pixbuf_get_width( pixbuf );
		this->height = gdk_pixbuf_get_height( pixbuf );

		// Allocate/define image and alpha
		uint8_t *image = malloc( this->width * this->height * 2 );
		uint8_t *alpha = NULL;

		// Extract YUV422 and alpha
		if ( gdk_pixbuf_get_has_alpha( pixbuf ) )
		{
			// Allocate the alpha mask
			alpha = malloc( this->width * this->height );

			// Convert the image
			mlt_convert_rgb24a_to_yuv422( gdk_pixbuf_get_pixels( pixbuf ),
										  this->width, this->height,
										  gdk_pixbuf_get_rowstride( pixbuf ),
										  image, alpha );
		}
		else
		{ 
			// No alpha to extract
			mlt_convert_rgb24_to_yuv422( gdk_pixbuf_get_pixels( pixbuf ),
										 this->width, this->height,
										 gdk_pixbuf_get_rowstride( pixbuf ),
										 image );
		}

		// Finished with pixbuf now
		g_object_unref( pixbuf );

		// Set width/height of frame
		mlt_properties_set_int( properties, "width", this->width );
		mlt_properties_set_int( properties, "height", this->height );

		// if single picture, reference the image and alpha in the producer
		this->image = image;
		this->alpha = alpha;

		// pass the image and alpha data without destructor
		mlt_properties_set_data( properties, "image", image, 0, NULL, NULL );
		mlt_properties_set_data( properties, "alpha", alpha, 0, NULL, NULL );

		// Set alpha call back
		( *frame )->get_alpha_mask = producer_get_alpha_mask;

		// Push the get_image method
		mlt_frame_push_get_image( *frame, producer_get_image );
	}

	// Update timecode on the frame we're creating
	mlt_frame_set_timecode( *frame, mlt_producer_position( producer ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	producer_pango this = parent->child;
	if ( this->markup )
		free( this->markup );
	if ( this->image )
		free( this->image );
	if ( this->alpha )
		free( this->alpha );
	parent->close = NULL;
	mlt_producer_close( parent );
	free( this );
}

static void pango_draw_background( GdkPixbuf *pixbuf, rgba_color bg )
{
	int ww = gdk_pixbuf_get_width( pixbuf );
	int hh = gdk_pixbuf_get_height( pixbuf );
	uint8_t *p = gdk_pixbuf_get_pixels( pixbuf );
	int i, j;

	for ( j = 0; j < hh; j++ )
	{
		for ( i = 0; i < ww; i++ )
		{
			*p++ = bg.r;
			*p++ = bg.g;
			*p++ = bg.b;
			*p++ = bg.a;
		}
	}
}

static GdkPixbuf *pango_get_pixbuf( const char *markup, rgba_color fg, rgba_color bg, int pad, int align )
{
	PangoFT2FontMap *fontmap = (PangoFT2FontMap*) pango_ft2_font_map_new();
	PangoContext *context = pango_ft2_font_map_create_context( fontmap );
	PangoLayout *layout = pango_layout_new( context );
//	PangoFontDescription *font;
	int w, h, x;
	int i, j;
	GdkPixbuf *pixbuf = NULL;
	FT_Bitmap bitmap;
	uint8_t *src = NULL;
	uint8_t* dest = NULL;
	int stride;

	pango_ft2_font_map_set_resolution( fontmap, 72, 72 );
	pango_layout_set_width( layout, -1 ); // set wrapping constraints
//	pango_layout_set_font_description( layout, "Sans 48" );
//	pango_layout_set_spacing( layout, space );
	pango_layout_set_alignment( layout, ( PangoAlignment ) align  );
	pango_layout_set_markup( layout, markup, (markup == NULL ? 0 : strlen( markup ) ) );
	pango_layout_get_pixel_size( layout, &w, &h );

	pixbuf = gdk_pixbuf_new( GDK_COLORSPACE_RGB, TRUE /* has alpha */, 8, w + 2 * pad, h + 2 * pad );
	pango_draw_background( pixbuf, bg );

	stride = gdk_pixbuf_get_rowstride( pixbuf );

	bitmap.width     = w;
	bitmap.pitch     = 32 * ( ( w + 31 ) / 31 );
	bitmap.rows      = h;
	bitmap.buffer    = ( unsigned char * ) calloc( 1, h * bitmap.pitch );
	bitmap.num_grays = 256;
	bitmap.pixel_mode = ft_pixel_mode_grays;

	pango_ft2_render_layout( &bitmap, layout, 0, 0 );

	src = bitmap.buffer;
	x = ( gdk_pixbuf_get_width( pixbuf ) - w - 2 * pad ) * align / 2 + pad;
	dest = gdk_pixbuf_get_pixels( pixbuf ) + 4 * x + pad * stride;
	for ( j = 0; j < h; j++ )
	{
		uint8_t *d = dest;
		for ( i = 0; i < w; i++ )
		{
			float a = ( float ) bitmap.buffer[ j * bitmap.pitch + i ] / 255.0;
			*d++ = ( int ) ( a * fg.r + ( 1 - a ) * bg.r );
			*d++ = ( int ) ( a * fg.g + ( 1 - a ) * bg.g );
			*d++ = ( int ) ( a * fg.b + ( 1 - a ) * bg.b );
			*d++ = ( int ) ( a * fg.a + ( 1 - a ) * bg.a );
		}
		dest += stride;
	}
	free( bitmap.buffer );

	g_object_unref( layout );
	g_object_unref( context );
	g_object_unref( fontmap );

	return pixbuf;
}

