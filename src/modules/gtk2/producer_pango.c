/*
 * producer_pango.c -- a pango-based titler
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Dan Dennedy <dan@dennedy.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt_producer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_geometry.h>
#include <stdlib.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pangoft2.h>
#include <freetype/freetype.h>
#include <iconv.h>
#include <pthread.h>
#include <ctype.h>

typedef struct producer_pango_s *producer_pango;

typedef enum
{
	pango_align_left = 0,
	pango_align_center,
	pango_align_right
} pango_align;

static pthread_mutex_t pango_mutex = PTHREAD_MUTEX_INITIALIZER;

struct producer_pango_s
{
	struct mlt_producer_s parent;
	int   width;
	int   height;
	GdkPixbuf *pixbuf;
	char *fgcolor;
	char *bgcolor;
	int   align;
	int   pad;
	char *markup;
	char *text;
	char *font;
	int   weight;
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
	rgba_color fg, rgba_color bg, int pad, int align, int weight, int size );

/** Return nonzero if the two strings are equal, ignoring case, up to
    the first n characters.
*/
int strncaseeq(const char *s1, const char *s2, size_t n)
{
	for ( ; n > 0; n--)
	{
		if (tolower(*s1++) != tolower(*s2++))
			return 0;
	}
	return 1;
}

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

static PangoFT2FontMap *fontmap = NULL;

mlt_producer producer_pango_init( const char *filename )
{
	producer_pango this = calloc( sizeof( struct producer_pango_s ), 1 );
	if ( this != NULL && mlt_producer_init( &this->parent, this ) == 0 )
	{
		mlt_producer producer = &this->parent;

		pthread_mutex_lock( &pango_mutex );
		if ( fontmap == NULL )
			fontmap = (PangoFT2FontMap*) pango_ft2_font_map_new();
		g_type_init();
		pthread_mutex_unlock( &pango_mutex );

		producer->get_frame = producer_get_frame;
		producer->close = ( mlt_destructor )producer_close;

		// Get the properties interface
		mlt_properties properties = MLT_PRODUCER_PROPERTIES( &this->parent );

		// Set the default properties
		mlt_properties_set( properties, "fgcolour", "0xffffffff" );
		mlt_properties_set( properties, "bgcolour", "0x00000000" );
		mlt_properties_set_int( properties, "align", pango_align_left );
		mlt_properties_set_int( properties, "pad", 0 );
		mlt_properties_set( properties, "text", "" );
		mlt_properties_set( properties, "font", "Sans 48" );
		mlt_properties_set( properties, "encoding", "UTF-8" );
		mlt_properties_set_int( properties, "weight", PANGO_WEIGHT_NORMAL );

		if ( filename == NULL )
		{
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
			mlt_properties_set( properties, "resource", filename );
			mlt_properties_set( properties, "markup", markup );
			free( copy );
		}
		else if ( strstr( filename, ".mpl" ) ) 
		{
			int i = 0;
			mlt_properties contents = mlt_properties_load( filename );
			mlt_geometry key_frames = mlt_geometry_init( );
			struct mlt_geometry_item_s item;
			mlt_properties_set( properties, "resource", filename );
			mlt_properties_set_data( properties, "contents", contents, 0, ( mlt_destructor )mlt_properties_close, NULL );
			mlt_properties_set_data( properties, "key_frames", key_frames, 0, ( mlt_destructor )mlt_geometry_close, NULL );

			// Make sure we have at least one entry
			if ( mlt_properties_get( contents, "0" ) == NULL )
				mlt_properties_set( contents, "0", "" );

			for ( i = 0; i < mlt_properties_count( contents ); i ++ )
			{
				char *name = mlt_properties_get_name( contents, i );
				char *value = mlt_properties_get_value( contents, i );
				while ( value != NULL && strchr( value, '~' ) )
					( *strchr( value, '~' ) ) = '\n';
				item.frame = atoi( name );
				mlt_geometry_insert( key_frames, &item );
			}
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

				mlt_properties_set( properties, "resource", filename );
				mlt_properties_set( properties, "markup", ( markup == NULL ? "" : markup ) );
				free( markup );
			}
			else
			{
				producer->close = NULL;
				mlt_producer_close( producer );
				producer = NULL;
				free( this );
			}
		}

		return producer;
	}
	free( this );
	return NULL;
}

static void set_string( char **string, const char *value, const char *fallback )
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

/** Convert a string property to UTF-8
*/
static int iconv_utf8( mlt_properties properties, const char *prop_name, const char* encoding )
{
	char *text = mlt_properties_get( properties, prop_name );
	int result = -1;
	
	iconv_t	cd = iconv_open( "UTF-8", encoding );
	if ( cd != ( iconv_t )-1 )
	{
		char *inbuf_p = text;
		size_t inbuf_n = strlen( text );
		size_t outbuf_n = inbuf_n * 6;
		char *outbuf = mlt_pool_alloc( outbuf_n );
		char *outbuf_p = outbuf;
		
		memset( outbuf, 0, outbuf_n );

		if ( text != NULL && strcmp( text, "" ) && iconv( cd, &inbuf_p, &inbuf_n, &outbuf_p, &outbuf_n ) != -1 )
			mlt_properties_set( properties, prop_name, outbuf );
		else
			mlt_properties_set( properties, prop_name, "" );

		mlt_pool_release( outbuf );
		iconv_close( cd );
		result = 0;
	}
	return result;
}

static void refresh_image( mlt_frame frame, int width, int height )
{
	// Pixbuf
	GdkPixbuf *pixbuf = mlt_properties_get_data( MLT_FRAME_PROPERTIES( frame ), "pixbuf", NULL );

	// Obtain properties of frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the producer pango for this frame
	producer_pango this = mlt_properties_get_data( properties, "producer_pango", NULL );

	// Obtain the producer 
	mlt_producer producer = &this->parent;

	// Obtain the producer properties
	mlt_properties producer_props = MLT_PRODUCER_PROPERTIES( producer );

	// Get producer properties
	char *fg = mlt_properties_get( producer_props, "fgcolour" );
	char *bg = mlt_properties_get( producer_props, "bgcolour" );
	int align = alignment_parse( mlt_properties_get( producer_props, "align" ) );
	int pad = mlt_properties_get_int( producer_props, "pad" );
	char *markup = mlt_properties_get( producer_props, "markup" );
	char *text = mlt_properties_get( producer_props, "text" );
	char *font = mlt_properties_get( producer_props, "font" );
	char *encoding = mlt_properties_get( producer_props, "encoding" );
	int weight = mlt_properties_get_int( producer_props, "weight" );
	int size = mlt_properties_get_int( producer_props, "size" );
	int property_changed = 0;

	if ( pixbuf == NULL )
	{
		// Check for file support
		int position = mlt_properties_get_position( properties, "pango_position" );
		mlt_properties contents = mlt_properties_get_data( producer_props, "contents", NULL );
		mlt_geometry key_frames = mlt_properties_get_data( producer_props, "key_frames", NULL );
		struct mlt_geometry_item_s item;
		if ( contents != NULL )
		{
			char temp[ 20 ];
			mlt_geometry_prev_key( key_frames, &item, position );
			sprintf( temp, "%d", item.frame );
			markup = mlt_properties_get( contents, temp );
		}
	
		// See if any properties changed
		property_changed = ( align != this->align );
		property_changed = property_changed || ( this->fgcolor == NULL || ( fg && strcmp( fg, this->fgcolor ) ) );
		property_changed = property_changed || ( this->bgcolor == NULL || ( bg && strcmp( bg, this->bgcolor ) ) );
		property_changed = property_changed || ( pad != this->pad );
		property_changed = property_changed || ( markup && this->markup && strcmp( markup, this->markup ) );
		property_changed = property_changed || ( text && this->text && strcmp( text, this->text ) );
		property_changed = property_changed || ( font && this->font && strcmp( font, this->font ) );
		property_changed = property_changed || ( weight != this->weight );

		// Save the properties for next comparison
		this->align = align;
		this->pad = pad;
		set_string( &this->fgcolor, fg, "0xffffffff" );
		set_string( &this->bgcolor, bg, "0x00000000" );
		set_string( &this->markup, markup, NULL );
		set_string( &this->text, text, NULL );
		set_string( &this->font, font, "Sans 48" );
		this->weight = weight;
	}

	if ( pixbuf == NULL && property_changed )
	{
		rgba_color fgcolor = parse_color( this->fgcolor );
		rgba_color bgcolor = parse_color( this->bgcolor );

		if ( this->pixbuf )
			g_object_unref( this->pixbuf );
		this->pixbuf = NULL;

		// Convert from specified encoding to UTF-8
		if ( encoding != NULL && !strncaseeq( encoding, "utf-8", 5 ) && !strncaseeq( encoding, "utf8", 4 ) )
		{
			if ( markup != NULL && iconv_utf8( producer_props, "markup", encoding ) != -1 )
			{
				markup = mlt_properties_get( producer_props, "markup" );
				set_string( &this->markup, markup, NULL );
			}
			if ( text != NULL && iconv_utf8( producer_props, "text", encoding ) != -1 )
			{
				text = mlt_properties_get( producer_props, "text" );
				set_string( &this->text, text, NULL );
			}
		}
		
		// Render the title
		pixbuf = pango_get_pixbuf( markup, text, font, fgcolor, bgcolor, pad, align, weight, size );

		if ( pixbuf != NULL )
		{
			// Register this pixbuf for destruction and reuse
			mlt_properties_set_data( producer_props, "pixbuf", pixbuf, 0, ( mlt_destructor )g_object_unref, NULL );
			g_object_ref( pixbuf );
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "pixbuf", pixbuf, 0, ( mlt_destructor )g_object_unref, NULL );

			mlt_properties_set_int( producer_props, "real_width", gdk_pixbuf_get_width( pixbuf ) );
			mlt_properties_set_int( producer_props, "real_height", gdk_pixbuf_get_height( pixbuf ) );

			// Store the width/height of the pixbuf temporarily
			this->width = gdk_pixbuf_get_width( pixbuf );
			this->height = gdk_pixbuf_get_height( pixbuf );
		}
	}
	else if ( pixbuf == NULL && width > 0 && ( this->pixbuf == NULL || width != this->width || height != this->height ) )
	{
		if ( this->pixbuf )
			g_object_unref( this->pixbuf );
		this->pixbuf = NULL;
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
		else if ( strcmp( interps, "hyper" ) == 0 || strcmp( interps, "bicubic" ) == 0 )
			interp = GDK_INTERP_HYPER;

// fprintf(stderr,"%s: scaling from %dx%d to %dx%d\n", __FILE__, this->width, this->height, width, height);

		// Note - the original pixbuf is already safe and ready for destruction
		this->pixbuf = gdk_pixbuf_scale_simple( pixbuf, width, height, interp );

		// Store width and height
		this->width = width;
		this->height = height;
	}

	// Set width/height
	mlt_properties_set_int( properties, "width", this->width );
	mlt_properties_set_int( properties, "height", this->height );
	mlt_properties_set_int( properties, "real_width", mlt_properties_get_int( producer_props, "real_width" ) );
	mlt_properties_set_int( properties, "real_height", mlt_properties_get_int( producer_props, "real_height" ) );
}

static int producer_get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	producer_pango this = ( producer_pango ) mlt_frame_pop_service( frame );

	// Obtain properties of frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	*width = mlt_properties_get_int( properties, "rescale_width" );
	*height = mlt_properties_get_int( properties, "rescale_height" );

	mlt_service_lock( MLT_PRODUCER_SERVICE( &this->parent ) );

	// Refresh the image
	pthread_mutex_lock( &pango_mutex );
	refresh_image( frame, *width, *height );

	// Get width and height
	*width = this->width;
	*height = this->height;

	// Always clone here to allow 'animated' text
	if ( this->pixbuf )
	{
		// Clone the image
		int image_size = this->width * this->height * 4;
		*buffer = mlt_pool_alloc( image_size );
		memcpy( *buffer, gdk_pixbuf_get_pixels( this->pixbuf ), image_size );

		// Now update properties so we free the copy after
		mlt_frame_set_image( frame, *buffer, image_size, mlt_pool_release );
		*format = mlt_image_rgb24a;
	}
	else
	{
		error = 1;
	}

	pthread_mutex_unlock( &pango_mutex );
	mlt_service_unlock( MLT_PRODUCER_SERVICE( &this->parent ) );

	return error;
}

static int producer_get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	producer_pango this = producer->child;

	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	// Obtain properties of frame and producer
	mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

	// Set the producer on the frame properties
	mlt_properties_set_data( properties, "producer_pango", this, 0, NULL, NULL );

	// Update timecode on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_position( producer ) );
	mlt_properties_set_position( properties, "pango_position", mlt_producer_frame( producer ) );

	// Refresh the pango image
	pthread_mutex_lock( &pango_mutex );
	refresh_image( *frame, 0, 0 );
	pthread_mutex_unlock( &pango_mutex );

	// Set producer-specific frame properties
	mlt_properties_set_int( properties, "progressive", 1 );
	mlt_properties_set_double( properties, "aspect_ratio", 1 );

	// Stack the get image callback
	mlt_frame_push_service( *frame, this );
	mlt_frame_push_get_image( *frame, producer_get_image );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	return 0;
}

static void producer_close( mlt_producer parent )
{
	producer_pango this = parent->child;
	if ( this->pixbuf )
		g_object_unref( this->pixbuf );
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

static GdkPixbuf *pango_get_pixbuf( const char *markup, const char *text, const char *font, rgba_color fg, rgba_color bg, int pad, int align, int weight, int size )
{
	PangoContext *context = pango_ft2_font_map_create_context( fontmap );
	PangoLayout *layout = pango_layout_new( context );
	int w, h, x;
	int i, j;
	GdkPixbuf *pixbuf = NULL;
	FT_Bitmap bitmap;
	uint8_t *src = NULL;
	uint8_t* dest = NULL;
	uint8_t *d, *s, a;
	int stride;
	PangoFontDescription *desc = pango_font_description_from_string( font );

	pango_ft2_font_map_set_resolution( fontmap, 72, 72 );
	pango_layout_set_width( layout, -1 ); // set wrapping constraints
	pango_font_description_set_weight( desc, ( PangoWeight ) weight  );
	if ( size != 0 )
		pango_font_description_set_absolute_size( desc, PANGO_SCALE * size );
	pango_layout_set_font_description( layout, desc );
//	pango_layout_set_spacing( layout, space );
	pango_layout_set_alignment( layout, ( PangoAlignment ) align  );
	if ( markup != NULL && strcmp( markup, "" ) != 0 )
	{
		pango_layout_set_markup( layout, markup, strlen( markup ) );
	}
	else if ( text != NULL && strcmp( text, "" ) != 0 )
	{
		// Replace all ~'s with a line feed (silly convention, but handy)
		char *copy = strdup( text );
		while ( strchr( copy, '~' ) )
			( *strchr( copy, '~' ) ) = '\n';
		pango_layout_set_text( layout, copy, strlen( copy ) );
		free( copy );
	}
	else
	{
		// Pango doesn't like empty strings
		pango_layout_set_text( layout, "  ", 2 );
	}
	pango_layout_get_pixel_size( layout, &w, &h );

	// Interpret size property as an absolute pixel height and compensate for
	// freetype's "interpretation" of our absolute size request. This gives
	// precise control over compositing and better quality by reducing scaling
	// artifacts with composite geometries that constrain the dimensions.
	// If you do not want this, then put the size in the font property or in
	// the pango markup.
	if ( size != 0 )
	{
		pango_font_description_set_absolute_size( desc, PANGO_SCALE * size * size/h );
		pango_layout_set_font_description( layout, desc );
		pango_layout_get_pixel_size( layout, &w, &h );
	}

        if ( pad == 0 )
            pad = 1;

	pixbuf = gdk_pixbuf_new( GDK_COLORSPACE_RGB, TRUE /* has alpha */, 8, w + 2 * pad, h + 2 * pad );
	pango_draw_background( pixbuf, bg );

	stride = gdk_pixbuf_get_rowstride( pixbuf );

	bitmap.width     = w;
	bitmap.pitch     = 32 * ( ( w + 31 ) / 31 );
	bitmap.rows      = h;
	bitmap.buffer    = mlt_pool_alloc( h * bitmap.pitch );
	bitmap.num_grays = 256;
	bitmap.pixel_mode = ft_pixel_mode_grays;

	memset( bitmap.buffer, 0, h * bitmap.pitch );

	pango_ft2_render_layout( &bitmap, layout, 0, 0 );

	src = bitmap.buffer;
	x = ( gdk_pixbuf_get_width( pixbuf ) - w - 2 * pad ) * align / 2 + pad;
	dest = gdk_pixbuf_get_pixels( pixbuf ) + 4 * x + pad * stride;
	j = h;

	while( j -- )
	{
		d = dest;
		s = src;
		i = w;
		while( i -- )
		{
			a = *s ++;
			*d++ = ( a * fg.r + ( 255 - a ) * bg.r ) >> 8;
			*d++ = ( a * fg.g + ( 255 - a ) * bg.g ) >> 8;
			*d++ = ( a * fg.b + ( 255 - a ) * bg.b ) >> 8;
			*d++ = ( a * fg.a + ( 255 - a ) * bg.a ) >> 8;
		}
		dest += stride;
		src += bitmap.pitch;
	}
	mlt_pool_release( bitmap.buffer );
	pango_font_description_free( desc );
	g_object_unref( layout );
	g_object_unref( context );

	return pixbuf;
}
