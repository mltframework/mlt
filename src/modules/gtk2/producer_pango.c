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

struct producer_pango_s
{
	struct mlt_producer_s parent;
	int width;
	int height;
	uint8_t *image;
	uint8_t *alpha;
	char *fgcolor;
	char *bgcolor;
	int   align;
	int   pad;
	char *markup;
	char *text;
	char *font;
};

// special color type used by internal pango routines
typedef struct
{
	uint8_t r, g, b, a;
} rgba_color;

// Forward declarations
static int producer_get_frame( mlt_producer parent, mlt_frame_ptr frame, int index );
static void producer_close( mlt_producer parent );
static void pango_draw_background( GdkPixbuf *pixbuf, rgba_color bg );
static GdkPixbuf *pango_get_pixbuf( const char *markup, const char *text, const char *font,
	rgba_color fg, rgba_color bg, int pad, int align );

/** Parse the alignment property.
*/

static int alignment_parse( char* align )
{
	int ret = pango_align_left;

	if ( align == NULL );
	else if ( isdigit( align[ 0 ] ) )
		ret = atoi( align );
	else if ( align[ 0 ] == 'c' || align[ 0 ] == 'm' )
		ret = pango_align_center;
	else if ( align[ 0 ] == 'r' )
		ret = pango_align_right;

	return ret;
}

mlt_producer producer_pango_init( const char *filename )
{
	producer_pango this = calloc( sizeof( struct producer_pango_s ), 1 );
	if ( this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		mlt_producer producer = &this->parent;

		producer->get_frame = producer_get_frame;
		producer->close = producer_close;

		// This is required to initialise gdk-pixbuf
		g_type_init();

		// Get the properties interface
		mlt_properties properties = mlt_producer_properties( &this->parent );

		// Set the default properties
		mlt_properties_set( properties, "fgcolour", "0xffffffff" );
		mlt_properties_set( properties, "bgcolour", "0x00000000" );
		mlt_properties_set_int( properties, "align", pango_align_left );
		mlt_properties_set_int( properties, "pad", 0 );
		mlt_properties_set( properties, "text", "" );
		mlt_properties_set( properties, "font", "Sans 48" );

		if ( filename == NULL )
		{
			mlt_properties_set( properties, "resource", "pango" );
			mlt_properties_set( properties, "markup", "" );
		}
		else if ( filename[ 0 ] == '+' || strstr( filename, "/+" ) )
		{
			char *copy = strdup( filename + 1 );
			char *markup = copy;
			if ( strstr( markup, "/+" ) )
				markup = strstr( markup, "/+" ) + 2;
			( *strrchr( markup, '.' ) ) = '\0';
			while ( strchr( markup, '~' ) )
				( *strchr( markup, '~' ) ) = '\n';
			mlt_properties_set( properties, "resource", ( char * )filename );
			mlt_properties_set( properties, "markup", markup );
			free( copy );
		}
		else
		{
			FILE *f = fopen( filename, "r" );
			if ( f != NULL )
			{
				char line[81];
				char *markup = NULL;
				size_t size = 0;
				line[80] = '\0';
				
				while ( fgets( line, 80, f ) )
				{
					size += strlen( line ) + 1;
					if ( markup )
					{
						markup = realloc( markup, size );
						strcat( markup, line );
					}
					else
					{
						markup = strdup( line );
					}
				}
				fclose( f );

				if ( markup[ strlen( markup ) - 1 ] == '\n' ) 
					markup[ strlen( markup ) - 1 ] = '\0';

				mlt_properties_set( properties, "resource", ( char * ) filename );
				mlt_properties_set( properties, "markup", ( char * ) ( markup == NULL ? "" : markup ) );
				free( markup );
			}
			else
			{
				mlt_properties_set( properties, "resource", "pango" );
				mlt_properties_set( properties, "markup", "" );
			}
		}

		return producer;
	}
	free( this );
	return NULL;
}

static void set_string( char **string, char *value, char *fallback )
{
	if ( value != NULL )
	{
		free( *string );
		*string = strdup( value );
	}
	else if ( *string == NULL && fallback != NULL )
	{
		*string = strdup( fallback );
	}
	else if ( *string != NULL && fallback == NULL )
	{
		free( *string );
		*string = NULL;
	}
}

rgba_color parse_color( char *color )
{
	rgba_color result = { 0xff, 0xff, 0xff, 0xff };

	if ( !strncmp( color, "0x", 2 ) )
	{
		unsigned int temp = 0;
		sscanf( color + 2, "%x", &temp );
		result.r = ( temp >> 24 ) & 0xff;
		result.g = ( temp >> 16 ) & 0xff;
		result.b = ( temp >> 8 ) & 0xff;
		result.a = ( temp ) & 0xff;
	}
	else if ( !strcmp( color, "red" ) )
	{
		result.r = 0xff;
		result.g = 0x00;
		result.b = 0x00;
	}
	else if ( !strcmp( color, "green" ) )
	{
		result.r = 0x00;
		result.g = 0xff;
		result.b = 0x00;
	}
	else if ( !strcmp( color, "blue" ) )
	{
		result.r = 0x00;
		result.g = 0x00;
		result.b = 0xff;
	}
	else
	{
		unsigned int temp = 0;
		sscanf( color, "%d", &temp );
		result.r = ( temp >> 24 ) & 0xff;
		result.g = ( temp >> 16 ) & 0xff;
		result.b = ( temp >> 8 ) & 0xff;
		result.a = ( temp ) & 0xff;
	}

	return result;
}

static void refresh_image( mlt_frame frame, int width, int height )
{
	// Pixbuf 
	GdkPixbuf *pixbuf = NULL;

	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( frame );

	// Obtain the producer pango for this frame
	producer_pango this = mlt_properties_get_data( properties, "producer_pango", NULL );

	// Obtain the producer 
	mlt_producer producer = &this->parent;

	// Obtain the producer properties
	mlt_properties producer_props = mlt_producer_properties( producer );

	// Get producer properties
	char *fg = mlt_properties_get( producer_props, "fgcolour" );
	char *bg = mlt_properties_get( producer_props, "bgcolour" );
	int align = alignment_parse( mlt_properties_get( producer_props, "align" ) );
	int pad = mlt_properties_get_int( producer_props, "pad" );
	char *markup = mlt_properties_get( producer_props, "markup" );
	char *text = mlt_properties_get( producer_props, "text" );
	char *font = mlt_properties_get( producer_props, "font" );

	// See if any properties changed
	int property_changed = ( align != this->align );
	property_changed = property_changed || ( this->fgcolor == NULL || ( fg && strcmp( fg, this->fgcolor ) ) );
	property_changed = property_changed || ( this->bgcolor == NULL || ( bg && strcmp( bg, this->bgcolor ) ) );
	property_changed = property_changed || ( pad != this->pad );
	property_changed = property_changed || ( markup && this->markup && strcmp( markup, this->markup ) );
	property_changed = property_changed || ( text && this->text && strcmp( text, this->text ) );
	property_changed = property_changed || ( font && this->font && strcmp( font, this->font ) );

	// Save the properties for next comparison
	this->align = align;
	this->pad = pad;
	set_string( &this->fgcolor, fg, "0xffffffff" );
	set_string( &this->bgcolor, bg, "0x00000000" );
	set_string( &this->markup, markup, NULL );
	set_string( &this->text, text, NULL );
	set_string( &this->font, font, "Sans 48" );

	if ( property_changed )
	{
		rgba_color fgcolor = parse_color( this->fgcolor );
		rgba_color bgcolor = parse_color( this->bgcolor );

		free( this->image );
		free( this->alpha );
		this->image = NULL;
		this->alpha = NULL;

		// Render the title
		pixbuf = pango_get_pixbuf( markup, text, font, fgcolor, bgcolor, pad, align );

		if ( pixbuf != NULL )
		{
			// Register this pixbuf for destruction and reuse
			mlt_properties_set_data( producer_props, "pixbuf", pixbuf, 0, ( mlt_destructor )g_object_unref, NULL );

			mlt_properties_set_int( producer_props, "real_width", gdk_pixbuf_get_width( pixbuf ) );
			mlt_properties_set_int( producer_props, "real_height", gdk_pixbuf_get_height( pixbuf ) );

			// Store the width/height of the pixbuf temporarily
			this->width = gdk_pixbuf_get_width( pixbuf );
			this->height = gdk_pixbuf_get_height( pixbuf );
		}
	}
	else if ( width > 0 && ( this->image == NULL || width != this->width || height != this->height ) )
	{
		free( this->image );
		free( this->alpha );
		this->image = NULL;
		this->alpha = NULL;

		pixbuf = mlt_properties_get_data( producer_props, "pixbuf", NULL );
	}

	// If we have a pixbuf and a valid width
	if ( pixbuf && width > 0 )
	{
		char *interps = mlt_properties_get( properties, "rescale.interp" );
		int interp = GDK_INTERP_BILINEAR;

		if ( strcmp( interps, "nearest" ) == 0 )
			interp = GDK_INTERP_NEAREST;
		else if ( strcmp( interps, "tiles" ) == 0 )
			interp = GDK_INTERP_TILES;
		else if ( strcmp( interps, "hyper" ) == 0 )
			interp = GDK_INTERP_HYPER;

//		fprintf( stderr, "SCALING PANGO from %dx%d to %dx%d was %dx%d\n", gdk_pixbuf_get_width( pixbuf ), gdk_pixbuf_get_height( pixbuf ), width, height, this->width, this->height );
			
		// Note - the original pixbuf is already safe and ready for destruction
		pixbuf = gdk_pixbuf_scale_simple( pixbuf, width, height, interp );

		// Store width and height
		this->width = width;
		this->height = height;

		// Allocate/define image
		// IRRIGATE ME
		uint8_t *image = malloc( width * ( height + 1 ) * 2 );
		uint8_t *alpha = NULL;

		// Allocate the alpha mask
		alpha = malloc( this->width * this->height );

		// Convert the image
		mlt_convert_rgb24a_to_yuv422( gdk_pixbuf_get_pixels( pixbuf ),
									  this->width, this->height,
									  gdk_pixbuf_get_rowstride( pixbuf ),
									  image, alpha );

		// Finished with pixbuf now
		g_object_unref( pixbuf );
		
		// reference the image in the producer
		this->image = image;
		this->alpha = alpha;
	}

	// Set width/height
	mlt_properties_set_int( properties, "width", this->width );
	mlt_properties_set_int( properties, "height", this->height );
	mlt_properties_set_int( properties, "real_width", mlt_properties_get_int( producer_props, "real_width" ) );
	mlt_properties_set_int( properties, "real_height", mlt_properties_get_int( producer_props, "real_height" ) );

	// pass the image data without destructor
	mlt_properties_set_data( properties, "image", this->image, this->width * ( this->height + 1 ) * 2, NULL, NULL );
	mlt_properties_set_data( properties, "alpha", this->alpha, this->width * this->height, NULL, NULL );
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	// Obtain properties of frame
	mlt_properties properties = mlt_frame_properties( frame );

	*width = mlt_properties_get_int( properties, "rescale_width" );
	*height = mlt_properties_get_int( properties, "rescale_height" );

	// Refresh the image
	refresh_image( frame, *width, *height );

	// Determine format
	//mlt_producer this = mlt_properties_get_data( properties, "producer_pango", NULL );
	//*format = ( mlt_properties_get_int( mlt_producer_properties( this ), "bpp" ) == 4 ) ? mlt_image_rgb24a : mlt_image_rgb24;

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
		// IRRIGATE ME
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

	// Generate a frame
	*frame = mlt_frame_init( );

	// Obtain properties of frame and producer
	mlt_properties properties = mlt_frame_properties( *frame );

	// Set the producer on the frame properties
	mlt_properties_set_data( properties, "producer_pango", this, 0, NULL, NULL );

	// Refresh the pango image
	refresh_image( *frame, 0, 0 );

	// Set producer-specific frame properties
	mlt_properties_set_int( properties, "progressive", 1 );
	mlt_properties_set_double( properties, "aspect_ratio", mlt_properties_get_double( properties, "real_width" ) / mlt_properties_get_double( properties, "real_height" ) );

	// Set alpha call back
	( *frame )->get_alpha_mask = producer_get_alpha_mask;

	// Stack the get image callback
	mlt_frame_push_get_image( *frame, producer_get_image );

	// Update timecode on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	producer_pango this = parent->child;
	free( this->image );
	free( this->alpha );
	free( this->fgcolor );
	free( this->bgcolor );
	free( this->markup );
	free( this->text );
	free( this->font );
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

static GdkPixbuf *pango_get_pixbuf( const char *markup, const char *text, const char *font, rgba_color fg, rgba_color bg, int pad, int align )
{
	PangoFT2FontMap *fontmap = (PangoFT2FontMap*) pango_ft2_font_map_new();
	PangoContext *context = pango_ft2_font_map_create_context( fontmap );
	PangoLayout *layout = pango_layout_new( context );
	int w, h, x;
	int i, j;
	GdkPixbuf *pixbuf = NULL;
	FT_Bitmap bitmap;
	uint8_t *src = NULL;
	uint8_t* dest = NULL;
	int stride;

	pango_ft2_font_map_set_resolution( fontmap, 72, 72 );
	pango_layout_set_width( layout, -1 ); // set wrapping constraints
	pango_layout_set_font_description( layout, pango_font_description_from_string( font ) );
//	pango_layout_set_spacing( layout, space );
	pango_layout_set_alignment( layout, ( PangoAlignment ) align  );
	if ( markup != NULL && strcmp( markup, "" ) != 0 )
		pango_layout_set_markup( layout, markup, strlen( markup ) );
	else if ( text != NULL && strcmp( text, "" ) != 0 )
		pango_layout_set_text( layout, text, strlen( text ) );
	else
		return NULL;
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

