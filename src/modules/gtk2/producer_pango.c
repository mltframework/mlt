/*
 * producer_pango.c -- a pango-based titler
 * Copyright (C) 2003-2015 Meltytech, LLC
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
#include <framework/mlt_geometry.h>
#include <framework/mlt_cache.h>
#include <stdlib.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pangoft2.h>
#include <ft2build.h>
#include FT_FREETYPE_H
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

struct pango_cached_image_s
{
	uint8_t *image, *alpha;
	mlt_image_format format;
	int width, height;
};

static void pango_cached_image_destroy( void* p )
{
	struct pango_cached_image_s* i = p;

	if ( !i )
		return;
	if ( i->image )
		mlt_pool_release( i->image );
	if ( i->alpha )
		mlt_pool_release( i->alpha );
	mlt_pool_release( i );
};

struct producer_pango_s
{
	struct mlt_producer_s parent;
	int   width;
	int   height;
	GdkPixbuf *pixbuf;
	char *fgcolor;
	char *bgcolor;
	char *olcolor;
	int   align;
	int   pad;
	int   outline;
	char *markup;
	char *text;
	char *font;
	char *family;
	int   size;
	int   style;
	int   weight;
	int   stretch;
	int   rotate;
	int   width_crop;
	int   width_fit;
	int   wrap_type;
	int   wrap_width;
	int   line_spacing;
	double aspect_ratio;
};

static void clean_cached( producer_pango self )
{
	mlt_service_cache_put( MLT_PRODUCER_SERVICE( &self->parent ), "pango.image", NULL, 0, NULL );
}

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
		rgba_color fg, rgba_color bg, rgba_color ol, int pad, int align, char* family,
		int style, int weight, int stretch, int size, int outline, int rotate,
		int width_crop, int width_fit, int wrap_type, int wrap_width,
		int line_spacing, double aspect_ratio );
static void fill_pixbuf( GdkPixbuf* pixbuf, FT_Bitmap* bitmap, int w, int h, int pad, int align, rgba_color fg, rgba_color bg );
static void fill_pixbuf_with_outline( GdkPixbuf* pixbuf, FT_Bitmap* bitmap, int w, int h, int pad, int align, rgba_color fg, rgba_color bg, rgba_color ol, int outline );

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

static int parse_alignment( char* align )
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

/** Parse the style property.
*/

static int parse_style( char* style )
{
	int ret = PANGO_STYLE_NORMAL;
	if( !strncmp(style, "italic", 6) )
		ret = PANGO_STYLE_ITALIC;
	if( !strncmp(style, "oblique", 7) )
		ret = PANGO_STYLE_OBLIQUE;
	return ret;
}

static PangoFT2FontMap *fontmap = NULL;

static void on_fontmap_reload( );
mlt_producer producer_pango_init( const char *filename )
{
	producer_pango this = calloc( 1, sizeof( struct producer_pango_s ) );
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

		mlt_events_register( properties, "fontmap-reload", NULL );
		mlt_events_listen( properties, producer, "fontmap-reload", (mlt_listener) on_fontmap_reload );

		// Set the default properties
		mlt_properties_set( properties, "fgcolour", "0xffffffff" );
		mlt_properties_set( properties, "bgcolour", "0x00000000" );
		mlt_properties_set( properties, "olcolour", "0x00000000" );
		mlt_properties_set_int( properties, "align", pango_align_left );
		mlt_properties_set_int( properties, "pad", 0 );
		mlt_properties_set_int( properties, "outline", 0 );
		mlt_properties_set( properties, "text", "" );
		mlt_properties_set( properties, "font", NULL );
		mlt_properties_set( properties, "family", "Sans" );
		mlt_properties_set_int( properties, "size", 48 );
		mlt_properties_set( properties, "style", "normal" );
		mlt_properties_set( properties, "encoding", "UTF-8" );
		mlt_properties_set_int( properties, "weight", PANGO_WEIGHT_NORMAL );
		mlt_properties_set_int( properties, "stretch", PANGO_STRETCH_NORMAL + 1 );
		mlt_properties_set_int( properties, "rotate", 0 );
		mlt_properties_set_int( properties, "seekable", 1 );

		if ( filename == NULL || ( filename && ( !strcmp( filename, "" )
			|| strstr( filename, "<producer>" )
			// workaround for old kdenlive countdown generator
			|| strstr( filename, "&lt;producer&gt;" ) ) ) )
		{
			mlt_properties_set( properties, "markup", "" );
		}
		else if ( filename[ 0 ] == '+' || strstr( filename, "/+" ) )
		{
			char *copy = strdup( filename + 1 );
			char *markup = copy;
			if ( strstr( markup, "/+" ) )
				markup = strstr( markup, "/+" ) + 2;
			if ( strrchr( markup, '.' ) )
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
			mlt_position out_point = 0;
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
				out_point = MAX( out_point, item.frame );
			}
			mlt_geometry_interpolate( key_frames );
			mlt_properties_set_position( properties, "length", out_point + 1 );
			mlt_properties_set_position( properties, "out", out_point );
		}
		else
		{
			// Convert file name string encoding.
			mlt_properties_set( properties, "resource", filename );
			mlt_properties_from_utf8( properties, "resource", "_resource" );
			filename = mlt_properties_get( properties, "_resource" );

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
						if ( markup )
							strcat( markup, line );
					}
					else
					{
						markup = strdup( line );
					}
				}
				fclose( f );

				if ( markup && markup[ strlen( markup ) - 1 ] == '\n' )
					markup[ strlen( markup ) - 1 ] = '\0';

				if ( markup )
					mlt_properties_set( properties, "markup", markup );
				else
					mlt_properties_set( properties, "markup", "" );
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

rgba_color parse_color( char *color, unsigned int color_int )
{
	rgba_color result = { 0xff, 0xff, 0xff, 0xff };

	if ( !strcmp( color, "red" ) )
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
	else if ( strcmp( color, "white" ) )
	{
		result.r = ( color_int >> 24 ) & 0xff;
		result.g = ( color_int >> 16 ) & 0xff;
		result.b = ( color_int >> 8 ) & 0xff;
		result.a = ( color_int ) & 0xff;
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
	if ( text && ( cd != ( iconv_t )-1 ) )
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
		result = 0;
	}
	iconv_close( cd );
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
	char *ol = mlt_properties_get( producer_props, "olcolour" );
	int align = parse_alignment( mlt_properties_get( producer_props, "align" ) );
	int pad = mlt_properties_get_int( producer_props, "pad" );
	int outline = mlt_properties_get_int( producer_props, "outline" );
	char *markup = mlt_properties_get( producer_props, "markup" );
	char *text = mlt_properties_get( producer_props, "text" );
	char *font = mlt_properties_get( producer_props, "font" );
	char *family = mlt_properties_get( producer_props, "family" );
	int style = parse_style( mlt_properties_get( producer_props, "style" ) );
	char *encoding = mlt_properties_get( producer_props, "encoding" );
	int weight = mlt_properties_get_int( producer_props, "weight" );
	int stretch = mlt_properties_get_int( producer_props, "stretch" );
	int rotate = mlt_properties_get_int( producer_props, "rotate" );
	int size = mlt_properties_get_int( producer_props, "size" );
	int width_crop = mlt_properties_get_int( producer_props, "width_crop" );
	int width_fit = mlt_properties_get_int( producer_props, "width_fit" );
	int wrap_type = mlt_properties_get_int( producer_props, "wrap_type" );
	int wrap_width = mlt_properties_get_int( producer_props, "wrap_width" );
	int line_spacing = mlt_properties_get_int( properties, "line_spacing" );
	double aspect_ratio = mlt_properties_get_double( properties, "aspect_ratio" );
	int property_changed = 0;

	if ( pixbuf == NULL )
	{
		// Check for file support
		mlt_properties contents = mlt_properties_get_data( producer_props, "contents", NULL );
		mlt_geometry key_frames = mlt_properties_get_data( producer_props, "key_frames", NULL );
		struct mlt_geometry_item_s item;
		if ( contents != NULL )
		{
			char temp[ 20 ];
			mlt_geometry_prev_key( key_frames, &item, mlt_frame_original_position( frame ) );
			sprintf( temp, "%d", item.frame );
			markup = mlt_properties_get( contents, temp );
		}

		// See if any properties changed
		property_changed = ( align != this->align );
		property_changed = property_changed || ( this->fgcolor == NULL || ( fg && strcmp( fg, this->fgcolor ) ) );
		property_changed = property_changed || ( this->bgcolor == NULL || ( bg && strcmp( bg, this->bgcolor ) ) );
		property_changed = property_changed || ( this->olcolor == NULL || ( ol && strcmp( ol, this->olcolor ) ) );
		property_changed = property_changed || ( pad != this->pad );
		property_changed = property_changed || ( outline != this->outline );
		property_changed = property_changed || ( markup && this->markup && strcmp( markup, this->markup ) );
		property_changed = property_changed || ( text && this->text && strcmp( text, this->text ) );
		property_changed = property_changed || ( font && this->font && strcmp( font, this->font ) );
		property_changed = property_changed || ( family && this->family && strcmp( family, this->family ) );
		property_changed = property_changed || ( weight != this->weight );
		property_changed = property_changed || ( stretch != this->stretch );
		property_changed = property_changed || ( rotate != this->rotate );
		property_changed = property_changed || ( style != this->style );
		property_changed = property_changed || ( size != this->size );
		property_changed = property_changed || ( width_crop != this->width_crop );
		property_changed = property_changed || ( width_fit != this->width_fit );
		property_changed = property_changed || ( wrap_type != this->wrap_type );
		property_changed = property_changed || ( wrap_width != this->wrap_width );
		property_changed = property_changed || ( line_spacing != this->line_spacing );
		property_changed = property_changed || ( aspect_ratio != this->aspect_ratio );

		// Save the properties for next comparison
		this->align = align;
		this->pad = pad;
		this->outline = outline;
		set_string( &this->fgcolor, fg, "0xffffffff" );
		set_string( &this->bgcolor, bg, "0x00000000" );
		set_string( &this->olcolor, ol, "0x00000000" );
		set_string( &this->markup, markup, NULL );
		set_string( &this->text, text, NULL );
		set_string( &this->font, font, NULL );
		set_string( &this->family, family, "Sans" );
		this->weight = weight;
		this->stretch = stretch;
		this->rotate = rotate;
		this->style = style;
		this->size = size;
		this->width_crop = width_crop;
		this->width_fit = width_fit;
		this->wrap_type = wrap_type;
		this->wrap_width = wrap_width;
		this->line_spacing = line_spacing;
		this->aspect_ratio = aspect_ratio;
	}

	if ( pixbuf == NULL && property_changed )
	{
		rgba_color fgcolor = parse_color( this->fgcolor, mlt_properties_get_int( producer_props, "fgcolour" ) );
		rgba_color bgcolor = parse_color( this->bgcolor, mlt_properties_get_int( producer_props, "bgcolour" ) );
		rgba_color olcolor = parse_color( this->olcolor, mlt_properties_get_int( producer_props, "olcolour" ) );

		if ( this->pixbuf )
			g_object_unref( this->pixbuf );
		this->pixbuf = NULL;
		clean_cached( this );

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
		pixbuf = pango_get_pixbuf( markup, text, font, fgcolor, bgcolor, olcolor, pad, align, family,
			style, weight, stretch, size, outline, rotate,
			width_crop, width_fit, wrap_type, wrap_width,
			line_spacing, aspect_ratio );

		if ( pixbuf != NULL )
		{
			// Register this pixbuf for destruction and reuse
			mlt_properties_set_data( producer_props, "pixbuf", pixbuf, 0, ( mlt_destructor )g_object_unref, NULL );
			g_object_ref( pixbuf );
			mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "pixbuf", pixbuf, 0, ( mlt_destructor )g_object_unref, NULL );

			mlt_properties_set_int( producer_props, "meta.media.width", gdk_pixbuf_get_width( pixbuf ) );
			mlt_properties_set_int( producer_props, "meta.media.height", gdk_pixbuf_get_height( pixbuf ) );

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
		clean_cached( this );
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
		clean_cached( this );

		// Store width and height
		this->width = width;
		this->height = height;
	}

	// Set width/height
	mlt_properties_set_int( properties, "width", this->width );
	mlt_properties_set_int( properties, "height", this->height );
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
		int size, bpp;
		uint8_t *buf;
		mlt_cache_item cached_item = mlt_service_cache_get( MLT_PRODUCER_SERVICE( &this->parent ), "pango.image" );
		struct pango_cached_image_s* cached = mlt_cache_item_data( cached_item, NULL );

		// destroy cached data if request is differ
		if ( !cached || ( cached && (cached->format != *format || cached->width != *width || cached->height != *height )))
		{
			mlt_cache_item_close( cached_item );
			cached_item = NULL;
			cached = NULL;
			clean_cached( this );
		}

		// create cached image
		if ( !cached )
		{
			int dst_stride, src_stride;

			cached = mlt_pool_alloc( sizeof( struct pango_cached_image_s ));
			cached->width = this->width;
			cached->height = this->height;
			cached->format = gdk_pixbuf_get_has_alpha( this->pixbuf ) ? mlt_image_rgb24a : mlt_image_rgb24;
			cached->alpha = NULL;
			cached->image = NULL;

			src_stride = gdk_pixbuf_get_rowstride( this->pixbuf );
			dst_stride = this->width * ( mlt_image_rgb24a == cached->format ? 4 : 3 );

			size = dst_stride * ( this->height + 1 );
			buf = mlt_pool_alloc( size );
			uint8_t *buf_save = buf;

			if ( src_stride != dst_stride )
			{
				int y = this->height;
				uint8_t *src = gdk_pixbuf_get_pixels( this->pixbuf );
				uint8_t *dst = buf;
				while ( y-- )
				{
					memcpy( dst, src, dst_stride );
					dst += dst_stride;
					src += src_stride;
				}
			}
			else
			{
				memcpy( buf, gdk_pixbuf_get_pixels( this->pixbuf ), src_stride * this->height );
			}

			// convert image
			if(frame->convert_image && cached->format != *format)
			{
				frame->convert_image( frame, &buf, &cached->format, *format );
				*format = cached->format;
				if ( buf != buf_save ) mlt_pool_release( buf_save );
			}

			size = mlt_image_format_size(cached->format, cached->width, cached->height, &bpp );
			cached->image = mlt_pool_alloc( size );
			memcpy( cached->image, buf, size );

			if ( ( buf = mlt_frame_get_alpha( frame ) ) )
			{
				size = cached->width * cached->height;
				cached->alpha = mlt_pool_alloc( size );
				memcpy( cached->alpha, buf, size );
			}
		}

		if ( cached )
		{
			// clone image surface
			size = mlt_image_format_size(cached->format, cached->width, cached->height, &bpp );
			buf = mlt_pool_alloc( size );
			memcpy( buf, cached->image, size );

			// set image surface
			mlt_frame_set_image( frame, buf, size, mlt_pool_release );
			*buffer = buf;

			// set alpha
			if ( cached->alpha )
			{
				size = cached->width * cached->height;
				buf = mlt_pool_alloc( size );
				memcpy( buf, cached->alpha, size );
				mlt_frame_set_alpha( frame, buf, size, mlt_pool_release );
			}
		}

		if ( cached_item )
			mlt_cache_item_close( cached_item );
		else
			mlt_service_cache_put( MLT_PRODUCER_SERVICE( &this->parent ), "pango.image",
				cached, sizeof( struct pango_cached_image_s ), pango_cached_image_destroy );
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

	// Fetch the producers properties
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );

	// Generate a frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( producer ) );

	// Obtain properties of frame and producer
	mlt_properties properties = MLT_FRAME_PROPERTIES( *frame );

	// Set the producer on the frame properties
	mlt_properties_set_data( properties, "producer_pango", this, 0, NULL, NULL );

	// Update timecode on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_position( producer ) );

	// Set producer-specific frame properties
	mlt_properties_set_int( properties, "progressive", 1 );
	double force_ratio = mlt_properties_get_double( producer_properties, "force_aspect_ratio" );
	if ( force_ratio > 0.0 )
		mlt_properties_set_double( properties, "aspect_ratio", force_ratio );
	else
	{
		mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) );
		mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile ) );
	}

	// Refresh the pango image
	pthread_mutex_lock( &pango_mutex );
	refresh_image( *frame, 0, 0 );
	pthread_mutex_unlock( &pango_mutex );

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
	mlt_service_cache_purge( MLT_PRODUCER_SERVICE(parent) );
	free( this->fgcolor );
	free( this->bgcolor );
	free( this->olcolor );
	free( this->markup );
	free( this->text );
	free( this->font );
	free( this->family );
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

static GdkPixbuf *pango_get_pixbuf( const char *markup, const char *text, const char *font,
	rgba_color fg, rgba_color bg, rgba_color ol, int pad, int align, char* family,
	int style, int weight, int stretch, int size, int outline, int rotate,
	int width_crop, int width_fit, int wrap_type, int wrap_width,
	int line_spacing, double aspect_ratio )
{
	PangoContext *context = pango_ft2_font_map_create_context( fontmap );
	PangoLayout *layout = pango_layout_new( context );
	int w, h;
	int x = 0, y = 0;
	GdkPixbuf *pixbuf = NULL;
	FT_Bitmap bitmap;
	PangoFontDescription *desc = NULL;

	if( font )
	{
		// Support for deprecated "font" property.
		desc = pango_font_description_from_string( font );
		pango_ft2_font_map_set_resolution( fontmap, 72, 72 );
	}
	else
	{
		desc = pango_font_description_from_string( family );
		pango_font_description_set_size( desc, PANGO_SCALE * size );
		pango_font_description_set_style( desc, (PangoStyle) style );
	}

	pango_font_description_set_weight( desc, ( PangoWeight ) weight  );

	if ( stretch )
		pango_font_description_set_stretch( desc, ( PangoStretch ) ( stretch - 1 ) );

	// set line_spacing
	if ( line_spacing )
		pango_layout_set_spacing( layout,  PANGO_SCALE * line_spacing );

	// set wrapping constraints
	if ( wrap_width <= 0 )
		pango_layout_set_width( layout, -1 );
	else
	{
		pango_layout_set_width( layout,  PANGO_SCALE * wrap_width );
		pango_layout_set_wrap( layout,  ( PangoWrapMode ) wrap_type );
	}

	pango_layout_set_font_description( layout, desc );
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

	if ( rotate )
	{
		double n_x, n_y;
		PangoRectangle rect;
		PangoMatrix m_layout = PANGO_MATRIX_INIT, m_offset = PANGO_MATRIX_INIT;

		pango_matrix_rotate( &m_layout, rotate );
		pango_matrix_rotate( &m_offset, -rotate );

		pango_context_set_base_gravity( context, PANGO_GRAVITY_AUTO );
		pango_context_set_matrix( context, &m_layout );
		pango_layout_context_changed( layout );
		pango_layout_get_extents( layout, NULL, &rect );
		pango_matrix_transform_rectangle( pango_context_get_matrix( context ), &rect);

		n_x = -rect.x;
		n_y = -rect.y;
		pango_matrix_transform_point( &m_offset, &n_x, &n_y );
		rect.x = n_x;
		rect.y = n_y;

		pango_extents_to_pixels( &rect, NULL );

		w = rect.width;
		h = rect.height;

		x = rect.x;
		y = rect.y;
	}
	else
		pango_layout_get_pixel_size( layout, &w, &h );

	// respect aspect ratio
	if ( 0.0 < aspect_ratio && aspect_ratio != 1.0)
	{
		double n_x, n_y;
		PangoRectangle rect;
		PangoMatrix m_layout = PANGO_MATRIX_INIT, m_offset = PANGO_MATRIX_INIT;
#if 1
		pango_matrix_scale( &m_layout, 1.0 / aspect_ratio, 1.0 );
		pango_matrix_scale( &m_offset, 1.0 / aspect_ratio, 1.0 );
#else
		pango_matrix_scale( &m_layout, 1.0, aspect_ratio );
		pango_matrix_scale( &m_offset, 1.0, aspect_ratio );
#endif
		pango_context_set_base_gravity( context, PANGO_GRAVITY_AUTO );
		pango_context_set_matrix( context, &m_layout );
		pango_layout_context_changed( layout );
		pango_layout_get_extents( layout, NULL, &rect );
		pango_matrix_transform_rectangle( pango_context_get_matrix( context ), &rect);

		n_x = -rect.x;
		n_y = -rect.y;
		pango_matrix_transform_point( &m_offset, &n_x, &n_y );
		rect.x = n_x;
		rect.y = n_y;

		pango_extents_to_pixels( &rect, NULL );

		w = rect.width;
		h = rect.height;

		x = rect.x;
		y = rect.y;
	}

	// limit width
	if ( width_crop && w > width_crop)
		w = width_crop;
	else if (width_fit && w > width_fit)
	{
		double n_x, n_y;
		PangoRectangle rect;
		PangoMatrix m_layout = PANGO_MATRIX_INIT, m_offset = PANGO_MATRIX_INIT;

		pango_matrix_scale( &m_layout, width_fit / (double)w, 1.0 );
		pango_matrix_scale( &m_offset, width_fit / (double)w, 1.0 );

		pango_context_set_base_gravity( context, PANGO_GRAVITY_AUTO );
		pango_context_set_matrix( context, &m_layout );
		pango_layout_context_changed( layout );
		pango_layout_get_extents( layout, NULL, &rect );
		pango_matrix_transform_rectangle( pango_context_get_matrix( context ), &rect);

		n_x = -rect.x;
		n_y = -rect.y;
		pango_matrix_transform_point( &m_offset, &n_x, &n_y );
		rect.x = n_x;
		rect.y = n_y;

		pango_extents_to_pixels( &rect, NULL );

		w = rect.width;
		h = rect.height;

		x = rect.x;
		y = rect.y;
	}

	if ( pad == 0 )
		pad = 1;

	pixbuf = gdk_pixbuf_new( GDK_COLORSPACE_RGB, TRUE /* has alpha */, 8, w + 2 * pad, h + 2 * pad );
	pango_draw_background( pixbuf, bg );

	bitmap.width     = w;
	bitmap.pitch     = 32 * ( ( w + 31 ) / 31 );
	bitmap.rows      = h;
	bitmap.buffer    = mlt_pool_alloc( h * bitmap.pitch );
	bitmap.num_grays = 256;
	bitmap.pixel_mode = ft_pixel_mode_grays;

	memset( bitmap.buffer, 0, h * bitmap.pitch );

	pango_ft2_render_layout( &bitmap, layout, x, y );

	if ( outline )
	{
		fill_pixbuf_with_outline( pixbuf, &bitmap, w, h, pad, align, fg, bg, ol, outline );
	}
	else
	{
		fill_pixbuf( pixbuf, &bitmap, w, h, pad, align, fg, bg );
	}

	mlt_pool_release( bitmap.buffer );
	pango_font_description_free( desc );
	g_object_unref( layout );
	g_object_unref( context );

	return pixbuf;
}

static void fill_pixbuf( GdkPixbuf* pixbuf, FT_Bitmap* bitmap, int w, int h, int pad, int align, rgba_color fg, rgba_color bg )
{
	int stride = gdk_pixbuf_get_rowstride( pixbuf );
	uint8_t* src = bitmap->buffer;
	int x = ( gdk_pixbuf_get_width( pixbuf ) - w - 2 * pad ) * align / 2 + pad;
	uint8_t* dest = gdk_pixbuf_get_pixels( pixbuf ) + 4 * x + pad * stride;
	int j = h;
	int i = 0;
	uint8_t *d, *s, a;

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
		src += bitmap->pitch;
	}
}

static void fill_pixbuf_with_outline( GdkPixbuf* pixbuf, FT_Bitmap* bitmap, int w, int h, int pad, int align, rgba_color fg, rgba_color bg, rgba_color ol, int outline )
{
	int stride = gdk_pixbuf_get_rowstride( pixbuf );
	int x = ( gdk_pixbuf_get_width( pixbuf ) - w - 2 * pad ) * align / 2 + pad;
	uint8_t* dest = gdk_pixbuf_get_pixels( pixbuf ) + 4 * x + pad * stride;
	int j ,i;
	uint8_t *d = NULL;
	float a_ol = 0;
	float a_fg = 0;

	for ( j = 0; j < h; j++ )
	{
		d = dest;
		for ( i = 0; i < w; i++ )
		{
#define geta(x, y) (float) bitmap->buffer[ (y) * bitmap->pitch + (x) ] / 255.0

			a_ol = geta(i, j);
			// One pixel fake circle
			if ( i > 0 )
				a_ol = MAX( a_ol, geta(i - 1, j) );
			if ( i < w - 1 )
				a_ol = MAX( a_ol, geta(i + 1, j) );
			if ( j > 0 )
				a_ol = MAX( a_ol, geta(i, j - 1) );
			if ( j < h - 1 )
				a_ol = MAX( a_ol, geta(i, j + 1) );
			if ( outline >= 2 ) {
				// Two pixels fake circle
				if ( i > 1 ) {
					a_ol = MAX( a_ol, geta(i - 2, j) );
					if ( j > 0 )
						a_ol = MAX( a_ol, geta(i - 2, j - 1) );
					if ( j < h - 1 )
						a_ol = MAX( a_ol, geta(i - 2, j + 1) );
				}
				if ( i > 0 ) {
					if ( j > 0 )
						a_ol = MAX( a_ol, geta(i - 1, j - 1) );
					if ( j > 1 )
						a_ol = MAX( a_ol, geta(i - 1, j - 2) );
					if ( j < h - 1 )
						a_ol = MAX( a_ol, geta(i - 1, j + 1) );
					if ( j < h - 2 )
						a_ol = MAX( a_ol, geta(i - 1, j + 2) );
				}
				if ( j > 1 )
					a_ol = MAX( a_ol, geta(i, j - 2) );
				if ( j < h - 2 )
					a_ol = MAX( a_ol, geta(i, j + 2) );
				if ( i < w - 1 ) {
					if ( j > 0 )
						a_ol = MAX( a_ol, geta(i + 1, j - 1) );
					if ( j > 1 )
						a_ol = MAX( a_ol, geta(i + 1, j - 2) );
					if ( j < h - 1 )
						a_ol = MAX( a_ol, geta(i + 1, j + 1) );
					if ( j < h - 2 )
						a_ol = MAX( a_ol, geta(i + 1, j + 2) );
				}
				if ( i < w - 2 ) {
					a_ol = MAX( a_ol, geta(i + 2, j) );
					if ( j > 0 )
						a_ol = MAX( a_ol, geta(i + 2, j - 1) );
					if ( j < h - 1 )
						a_ol = MAX( a_ol, geta(i + 2, j + 1) );
				}
			}
			if ( outline >= 3 ) {
				// Three pixels fake circle
				if ( i > 2 ) {
					a_ol = MAX( a_ol, geta(i - 3, j) );
					if ( j > 0 )
						a_ol = MAX( a_ol, geta(i - 3, j - 1) );
					if ( j < h - 1 )
						a_ol = MAX( a_ol, geta(i - 3, j + 1) );
				}
				if ( i > 1 ) {
					if ( j > 1 )
						a_ol = MAX( a_ol, geta(i - 2, j - 2) );
					if ( j < h - 2 )
						a_ol = MAX( a_ol, geta(i - 2, j + 2) );
				}
				if ( i > 0 ) {
					if ( j > 2 )
						a_ol = MAX( a_ol, geta(i - 1, j - 3) );
					if ( j < h - 3 )
						a_ol = MAX( a_ol, geta(i - 1, j + 3) );
				}
				if ( j > 2 )
					a_ol = MAX( a_ol, geta(i, j - 3) );
				if ( j < h - 3 )
					a_ol = MAX( a_ol, geta(i, j + 3) );
				if ( i < w - 1 ) {
					if ( j > 2 )
						a_ol = MAX( a_ol, geta(i + 1, j - 3) );
					if ( j < h - 3 )
						a_ol = MAX( a_ol, geta(i + 1, j + 3) );
				}
				if ( i < w - 2 ) {
					if ( j > 1 )
						a_ol = MAX( a_ol, geta(i + 2, j - 2) );
					if ( j < h - 2 )
						a_ol = MAX( a_ol, geta(i + 2, j + 2) );
				}
				if ( i < w - 3 ) {
					a_ol = MAX( a_ol, geta(i + 3, j) );
					if ( j > 0 )
						a_ol = MAX( a_ol, geta(i + 3, j - 1) );
					if ( j < h - 1 )
						a_ol = MAX( a_ol, geta(i + 3, j + 1) );
				}
			}

			a_fg = ( float ) bitmap->buffer[ j * bitmap->pitch + i ] / 255.0;

			*d++ = ( int ) ( a_fg * fg.r + ( 1 - a_fg ) * ( a_ol * ol.r + ( 1 - a_ol ) * bg.r ) );
			*d++ = ( int ) ( a_fg * fg.g + ( 1 - a_fg ) * ( a_ol * ol.g + ( 1 - a_ol ) * bg.g ) );
			*d++ = ( int ) ( a_fg * fg.b + ( 1 - a_fg ) * ( a_ol * ol.b + ( 1 - a_ol ) * bg.b ) );
			*d++ = ( int ) ( a_fg * fg.a + ( 1 - a_fg ) * ( a_ol * ol.a + ( 1 - a_ol ) * bg.a ) );
		}
		dest += stride;
	}
}

static void on_fontmap_reload()
{
	PangoFT2FontMap *new_fontmap = NULL, *old_fontmap = NULL;

	FcInitReinitialize();

	new_fontmap = (PangoFT2FontMap*) pango_ft2_font_map_new();

	pthread_mutex_lock( &pango_mutex );
	old_fontmap = fontmap;
	fontmap = new_fontmap;
	pthread_mutex_unlock( &pango_mutex );

	if ( old_fontmap )
		g_object_unref( old_fontmap );
}
