/*
 * transition_composite.c -- compose one image over another using alpha channel
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

#include "transition_composite.h"
#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

/** Geometry struct.
*/

struct geometry_s
{
	float position;
	float mix;
	int nw; // normalised width
	int nh; // normalised height
	int sw; // scaled width, not including consumer scale based upon w/nw
	int sh; // scaled height, not including consumer scale based upon h/nh
	float x;
	float y;
	float w;
	float h;
	int halign; // horizontal alignment: 0=left, 1=center, 2=right
	int valign; // vertical alignment: 0=top, 1=middle, 2=bottom
	int distort;
	struct geometry_s *next;
};

/** Parse a value from a geometry string.
*/

static float parse_value( char **ptr, int normalisation, char delim, float defaults )
{
	float value = defaults;

	if ( *ptr != NULL && **ptr != '\0' )
	{
		char *end = NULL;
		value = strtod( *ptr, &end );
		if ( end != NULL )
		{
			if ( *end == '%' )
				value = ( value / 100.0 ) * normalisation;
			while ( *end == delim || *end == '%' )
				end ++;
		}
		*ptr = end;
	}

	return value;
}

/** Parse a geometry property string with the syntax X,Y:WxH:MIX. Any value can be 
	expressed as a percentage by appending a % after the value, otherwise values are
	assumed to be relative to the normalised dimensions of the consumer.
*/

static void geometry_parse( struct geometry_s *geometry, struct geometry_s *defaults, char *property, int nw, int nh )
{
	// Assign normalised width and height
	geometry->nw = nw;
	geometry->nh = nh;

	// Assign from defaults if available
	if ( defaults != NULL )
	{
		geometry->x = defaults->x;
		geometry->y = defaults->y;
		geometry->w = geometry->sw = defaults->w;
		geometry->h = geometry->sh = defaults->h;
		geometry->distort = defaults->distort;
		geometry->mix = defaults->mix;
		defaults->next = geometry;
	}
	else
	{
		geometry->mix = 100;
	}

	// Parse the geomtry string
	if ( property != NULL && strcmp( property, "" ) )
	{
		char *ptr = property;
		geometry->x = parse_value( &ptr, nw, ',', geometry->x );
		geometry->y = parse_value( &ptr, nh, ':', geometry->y );
		geometry->w = geometry->sw = parse_value( &ptr, nw, 'x', geometry->w );
		geometry->h = geometry->sh = parse_value( &ptr, nh, ':', geometry->h );
		if ( *ptr == '!' )
		{
			geometry->distort = 1;
			ptr ++;
			if ( *ptr == ':' )
				ptr ++;
		}
		geometry->mix = parse_value( &ptr, 100, ' ', geometry->mix );
	}
}

/** Calculate real geometry.
*/

static void geometry_calculate( struct geometry_s *output, struct geometry_s *in, float position )
{
	// Search in for position
	struct geometry_s *out = in->next;

	if ( position >= 1.0 )
	{
		int section = floor( position );
		position -= section;
		if ( section % 2 == 1 )
			position = 1.0 - position;
	}

	while ( out->next != NULL )
	{
		if ( position >= in->position && position < out->position )
			break;

		in = out;
		out = in->next;
	}

	position = ( position - in->position ) / ( out->position - in->position );

	// Calculate this frames geometry
	output->nw = in->nw;
	output->nh = in->nh;
	output->x = in->x + ( out->x - in->x ) * position;
	output->y = in->y + ( out->y - in->y ) * position;
	output->w = in->w + ( out->w - in->w ) * position;
	output->h = in->h + ( out->h - in->h ) * position;
	output->mix = in->mix + ( out->mix - in->mix ) * position;
	output->distort = in->distort;

	output->x = ( int )floor( output->x ) & 0xfffffffe;
	output->w = ( int )floor( output->w ) & 0xfffffffe;
	output->sw &= 0xfffffffe;
}

void transition_destroy_keys( void *arg )
{
	struct geometry_s *ptr = arg;
	struct geometry_s *next = NULL;

	while ( ptr != NULL )
	{
		next = ptr->next;
		free( ptr );
		ptr = next;
	}
}

static struct geometry_s *transition_parse_keys( mlt_transition this,  int normalised_width, int normalised_height )
{
	// Loop variable for property interrogation
	int i = 0;

	// Get the properties of the transition
	mlt_properties properties = mlt_transition_properties( this );

	// Get the in and out position
	mlt_position in = mlt_transition_get_in( this );
	mlt_position out = mlt_transition_get_out( this );

	// Create the start
	struct geometry_s *start = calloc( 1, sizeof( struct geometry_s ) );

	// Create the end (we always need two entries)
	struct geometry_s *end = calloc( 1, sizeof( struct geometry_s ) );

	// Pointer
	struct geometry_s *ptr = start;

	// Parse the start property
	geometry_parse( start, NULL, mlt_properties_get( properties, "start" ), normalised_width, normalised_height );

	// Parse the keys in between
	for ( i = 0; i < mlt_properties_count( properties ); i ++ )
	{
		// Get the name of the property
		char *name = mlt_properties_get_name( properties, i );

		// Check that it's valid
		if ( !strncmp( name, "key[", 4 ) )
		{
			// Get the value of the property
			char *value = mlt_properties_get_value( properties, i );

			// Determine the frame number
			int frame = atoi( name + 4 );

			// Determine the position
			float position = 0;
			
			if ( frame >= 0 && frame < ( out - in ) )
				position = ( float )frame / ( float )( out - in + 1 );
			else if ( frame < 0 && - frame < ( out - in ) )
				position = ( float )( out - in + frame ) / ( float )( out - in + 1 );

			// For now, we'll exclude all keys received out of order
			if ( position > ptr->position )
			{
				// Create a new geometry
				struct geometry_s *temp = calloc( 1, sizeof( struct geometry_s ) );

				// Parse and add to the list
				geometry_parse( temp, ptr, value, normalised_width, normalised_height );

				// Assign the position
				temp->position = position;

				// Allow the next to be appended after this one
				ptr = temp;
			}
			else
			{
				fprintf( stderr, "Key out of order - skipping %s\n", name );
			}
		}
	}
	
	// Parse the end
	geometry_parse( end, ptr, mlt_properties_get( properties, "end" ), normalised_width, normalised_height );
	if ( out > 0 )
		end->position = ( float )( out - in ) / ( float )( out - in + 1 );
	else
		end->position = 1;

	// Assign to properties to ensure we get destroyed
	mlt_properties_set_data( properties, "geometries", start, 0, transition_destroy_keys, NULL );

	return start;
}

/** Parse the alignment properties into the geometry.
*/

static int alignment_parse( char* align )
{
	int ret = 0;
	
	if ( align == NULL );
	else if ( isdigit( align[ 0 ] ) )
		ret = atoi( align );
	else if ( align[ 0 ] == 'c' || align[ 0 ] == 'm' )
		ret = 1;
	else if ( align[ 0 ] == 'r' || align[ 0 ] == 'b' )
		ret = 2;

	return ret;
}

/** Adjust position according to scaled size and alignment properties.
*/

static void alignment_calculate( struct geometry_s *geometry )
{
	geometry->x += ( geometry->w - geometry->sw ) * geometry->halign / 2;
	geometry->y += ( geometry->h - geometry->sh ) * geometry->valign / 2;
}

/** Calculate the position for this frame.
*/

static float position_calculate( mlt_transition this, mlt_position position )
{
	// Get the in and out position
	mlt_position in = mlt_transition_get_in( this );
	mlt_position out = mlt_transition_get_out( this );

	// Now do the calcs
	return ( float )( position - in ) / ( float )( out - in + 1 );
}

/** Calculate the field delta for this frame - position between two frames.
*/

static inline float delta_calculate( mlt_transition this, mlt_frame frame )
{
	// Get the in and out position
	mlt_position in = mlt_transition_get_in( this );
	mlt_position out = mlt_transition_get_out( this );

	// Get the position of the frame
	mlt_position position = mlt_frame_get_position( frame );

	// Now do the calcs
	float x = ( float )( position - in ) / ( float )( out - in + 1 );
	float y = ( float )( position + 1 - in ) / ( float )( out - in + 1 );

	return ( y - x ) / 2.0;
}

static int get_value( mlt_properties properties, char *preferred, char *fallback )
{
	int value = mlt_properties_get_int( properties, preferred );
	if ( value == 0 )
		value = mlt_properties_get_int( properties, fallback );
	return value;
}

/** A linear threshold determination function.
*/

static inline int32_t linearstep( int32_t edge1, int32_t edge2, int32_t a )
{
	if ( a < edge1 )
		return 0;

	if ( a >= edge2 )
		return 0x10000;

	return ( ( a - edge1 ) << 16 ) / ( edge2 - edge1 );
}

/** A smoother, non-linear threshold determination function.
*/

static inline int32_t smoothstep( int32_t edge1, int32_t edge2, uint32_t a )
{
	if ( a < edge1 )
		return 0;

	if ( a >= edge2 )
		return 0x10000;

	a = ( ( a - edge1 ) << 16 ) / ( edge2 - edge1 );

	return ( ( ( a * a ) >> 16 )  * ( ( 3 << 16 ) - ( 2 * a ) ) ) >> 16;
}

/** Load the luma map from PGM stream.
*/

static void luma_read_pgm( FILE *f, uint16_t **map, int *width, int *height )
{
	uint8_t *data = NULL;
	while (1)
	{
		char line[128];
		char comment[128];
		int i = 2;
		int maxval;
		int bpp;
		uint16_t *p;

		line[127] = '\0';

		// get the magic code
		if ( fgets( line, 127, f ) == NULL )
			break;

		// skip comments
		while ( sscanf( line, " #%s", comment ) > 0 )
			if ( fgets( line, 127, f ) == NULL )
				break;

		if ( line[0] != 'P' || line[1] != '5' )
			break;

		// skip white space and see if a new line must be fetched
		for ( i = 2; i < 127 && line[i] != '\0' && isspace( line[i] ); i++ );
		if ( ( line[i] == '\0' || line[i] == '#' ) && fgets( line, 127, f ) == NULL )
			break;

		// skip comments
		while ( sscanf( line, " #%s", comment ) > 0 )
			if ( fgets( line, 127, f ) == NULL )
				break;

		// get the dimensions
		if ( line[0] == 'P' )
			i = sscanf( line, "P5 %d %d %d", width, height, &maxval );
		else
			i = sscanf( line, "%d %d %d", width, height, &maxval );

		// get the height value, if not yet
		if ( i < 2 )
		{
			if ( fgets( line, 127, f ) == NULL )
				break;

			// skip comments
			while ( sscanf( line, " #%s", comment ) > 0 )
				if ( fgets( line, 127, f ) == NULL )
					break;

			i = sscanf( line, "%d", height );
			if ( i == 0 )
				break;
			else
				i = 2;
		}

		// get the maximum gray value, if not yet
		if ( i < 3 )
		{
			if ( fgets( line, 127, f ) == NULL )
				break;

			// skip comments
			while ( sscanf( line, " #%s", comment ) > 0 )
				if ( fgets( line, 127, f ) == NULL )
					break;

			i = sscanf( line, "%d", &maxval );
			if ( i == 0 )
				break;
		}

		// determine if this is one or two bytes per pixel
		bpp = maxval > 255 ? 2 : 1;

		// allocate temporary storage for the raw data
		data = mlt_pool_alloc( *width * *height * bpp );
		if ( data == NULL )
			break;

		// read the raw data
		if ( fread( data, *width * *height * bpp, 1, f ) != 1 )
			break;

		// allocate the luma bitmap
		*map = p = (uint16_t*)mlt_pool_alloc( *width * *height * sizeof( uint16_t ) );
		if ( *map == NULL )
			break;

		// proces the raw data into the luma bitmap
		for ( i = 0; i < *width * *height * bpp; i += bpp )
		{
			if ( bpp == 1 )
				*p++ = data[ i ] << 8;
			else
				*p++ = ( data[ i ] << 8 ) + data[ i+1 ];
		}

		break;
	}

	if ( data != NULL )
		mlt_pool_release( data );
}

/** Generate a luma map from any YUV image.
*/

static void luma_read_yuv422( uint8_t *image, uint16_t **map, int width, int height )
{
	int i;
	
	// allocate the luma bitmap
	uint16_t *p = *map = ( uint16_t* )mlt_pool_alloc( width * height * sizeof( uint16_t ) );
	if ( *map == NULL )
		return;

	// proces the image data into the luma bitmap
	for ( i = 0; i < width * height * 2; i += 2 )
		*p++ = ( image[ i ] - 16 ) * 299; // 299 = 65535 / 219
}

/** Composite function.
*/

static int composite_yuv( uint8_t *p_dest, int width_dest, int height_dest, int bpp, uint8_t *p_src, int width_src, int height_src, uint8_t *p_alpha, struct geometry_s geometry, int field, uint16_t *p_luma, int32_t softness )
{
	int ret = 0;
	int i, j;
	int x_src = 0, y_src = 0;
	int32_t weight = ( 1 << 16 ) * ( geometry.mix / 100 );
	int stride_src = width_src * bpp;
	int stride_dest = width_dest * bpp;

	// Adjust to consumer scale
	int x = geometry.x * width_dest / geometry.nw;
	int y = geometry.y * height_dest / geometry.nh;

	x &= 0xfffffffe;
	width_src &= 0xfffffffe;

	// optimization points - no work to do
	if ( width_src <= 0 || height_src <= 0 )
		return ret;

	if ( ( x < 0 && -x >= width_src ) || ( y < 0 && -y >= height_src ) )
		return ret;

	// crop overlay off the left edge of frame
	if ( x < 0 )
	{
		x_src = -x;
		width_src -= x_src;
		x = 0;
	}
	
	// crop overlay beyond right edge of frame
	else if ( x + width_src > width_dest )
		width_src = width_dest - x;

	// crop overlay off the top edge of the frame
	if ( y < 0 )
	{
		y_src = -y;
		height_src -= y_src;
	}
	// crop overlay below bottom edge of frame
	else if ( y + height_src > height_dest )
		height_src = height_dest - y;

	// offset pointer into overlay buffer based on cropping
	p_src += x_src * bpp + y_src * stride_src;

	// offset pointer into frame buffer based upon positive coordinates only!
	p_dest += ( x < 0 ? 0 : x ) * bpp + ( y < 0 ? 0 : y ) * stride_dest;

	// offset pointer into alpha channel based upon cropping
	if ( p_alpha )
		p_alpha += x_src + y_src * stride_src / bpp;

	// offset pointer into luma channel based upon cropping
	if ( p_luma )
		p_luma += x_src + y_src * stride_src / bpp;
	
	// Assuming lower field first
	// Special care is taken to make sure the b_frame is aligned to the correct field.
	// field 0 = lower field and y should be odd (y is 0-based).
	// field 1 = upper field and y should be even.
	if ( ( field > -1 ) && ( y % 2 == field ) )
	{
		//fprintf( stderr, "field %d y %d\n", field, y );
		if ( ( field == 1 && y < height_dest - 1 ) || ( field == 0 && y == 0 ) )
			p_dest += stride_dest;
		else
			p_dest -= stride_dest;
	}

	// On the second field, use the other lines from b_frame
	if ( field == 1 )
	{
		p_src += stride_src;
		if ( p_alpha )
			p_alpha += stride_src / bpp;
		height_src--;
	}

	uint8_t *p = p_src;
	uint8_t *q = p_dest;
	uint8_t *o = p_dest;
	uint16_t *l = p_luma;
	uint8_t *z = p_alpha;

	uint8_t a;
	int32_t current_weight;
	int32_t value;
	int step = ( field > -1 ) ? 2 : 1;

	stride_src = stride_src * step;
	int alpha_stride = stride_src / bpp;
	stride_dest = stride_dest * step;

	// now do the compositing only to cropped extents
	for ( i = 0; i < height_src; i += step )
	{
		p = p_src;
		q = p_dest;
		o = q;
		l = p_luma;
		z = p_alpha;

		for ( j = 0; j < width_src; j ++ )
		{
			a = ( z == NULL ) ? 255 : *z ++;
			current_weight = ( l == NULL ) ? weight : linearstep( l[ j ], l[ j ] + softness, weight );
			value = ( current_weight * ( a + 1 ) ) >> 8;
			*o ++ = ( *p++ * value + *q++ * ( ( 1 << 16 ) - value ) ) >> 16;
			*o ++ = ( *p++ * value + *q++ * ( ( 1 << 16 ) - value ) ) >> 16;
		}

		p_src += stride_src;
		p_dest += stride_dest;
		if ( p_alpha )
			p_alpha += alpha_stride;
		if ( p_luma )
			p_luma += alpha_stride;
	}

	return ret;
}

static uint16_t* get_luma( mlt_properties properties, int width, int height )
{
	// The cached luma map information
	int luma_width = mlt_properties_get_int( properties, "_luma.width" );
	int luma_height = mlt_properties_get_int( properties, "_luma.height" );
	uint16_t *luma_bitmap = mlt_properties_get_data( properties, "_luma.bitmap", NULL );
	
	// If the filename property changed, reload the map
	char *resource = mlt_properties_get( properties, "luma" );

	if ( luma_bitmap == NULL && resource != NULL )
	{
		char *extension = extension = strrchr( resource, '.' );

		// See if it is a PGM
		if ( extension != NULL && strcmp( extension, ".pgm" ) == 0 )
		{
			// Open PGM
			FILE *f = fopen( resource, "r" );
			if ( f != NULL )
			{
				// Load from PGM
				luma_read_pgm( f, &luma_bitmap, &luma_width, &luma_height );
				fclose( f );

				// Set the transition properties
				mlt_properties_set_int( properties, "_luma.width", luma_width );
				mlt_properties_set_int( properties, "_luma.height", luma_height );
				mlt_properties_set_data( properties, "_luma.bitmap", luma_bitmap, luma_width * luma_height * 2, mlt_pool_release, NULL );
			}
		}
		else
		{
			// Get the factory producer service
			char *factory = mlt_properties_get( properties, "factory" );

			// Create the producer
			mlt_producer producer = mlt_factory_producer( factory, resource );

			// If we have one
			if ( producer != NULL )
			{
				// Get the producer properties
				mlt_properties producer_properties = mlt_producer_properties( producer );

				// Ensure that we loop
				mlt_properties_set( producer_properties, "eof", "loop" );

				// Now pass all producer. properties on the transition down
				mlt_properties_pass( producer_properties, properties, "luma." );

				// We will get the alpha frame from the producer
				mlt_frame luma_frame = NULL;

				// Get the luma frame
				if ( mlt_service_get_frame( mlt_producer_service( producer ), &luma_frame, 0 ) == 0 )
				{
					uint8_t *luma_image;
					mlt_image_format luma_format = mlt_image_yuv422;

					// Request a luma image the size of transition image request
					luma_width = width;
					luma_height = height;

					// Get image from the luma producer
					mlt_properties_set( mlt_frame_properties( luma_frame ), "distort", "true" );
					mlt_frame_get_image( luma_frame, &luma_image, &luma_format, &luma_width, &luma_height, 0 );

					// Generate the luma map
					if ( luma_image != NULL && luma_format == mlt_image_yuv422 )
					{
						luma_read_yuv422( luma_image, &luma_bitmap, luma_width, luma_height );
					
						// Set the transition properties
						mlt_properties_set_int( properties, "_luma.width", luma_width );
						mlt_properties_set_int( properties, "_luma.height", luma_height );
						mlt_properties_set_data( properties, "_luma.bitmap", luma_bitmap, luma_width * luma_height * 2, mlt_pool_release, NULL );
					}

					// Cleanup the luma frame
					mlt_frame_close( luma_frame );
				}

				// Cleanup the luma producer
				mlt_producer_close( producer );
			}
		}
	}
	return luma_bitmap;
}

/** Get the properly sized image from b_frame.
*/

static int get_b_frame_image( mlt_transition this, mlt_frame b_frame, uint8_t **image, int *width, int *height, struct geometry_s *geometry )
{
	int ret = 0;
	mlt_image_format format = mlt_image_yuv422;

	// Get the properties objects
	mlt_properties b_props = mlt_frame_properties( b_frame );
	mlt_properties properties = mlt_transition_properties( this );

	if ( mlt_properties_get( properties, "distort" ) == NULL && geometry->distort == 0 )
	{
		// Adjust b_frame pixel aspect
		int normalised_width = geometry->w;
		int normalised_height = geometry->h;
		int real_width = get_value( b_props, "real_width", "width" );
		int real_height = get_value( b_props, "real_height", "height" );
		double input_ar = mlt_frame_get_aspect_ratio( b_frame );
		double output_ar = mlt_properties_get_double( b_props, "consumer_aspect_ratio" );
		int scaled_width = input_ar / output_ar * real_width;
		int scaled_height = real_height;
			
		// Now ensure that our images fit in the normalised frame
		if ( scaled_width > normalised_width )
		{
			scaled_height = scaled_height * normalised_width / scaled_width;
			scaled_width = normalised_width;
		}
		if ( scaled_height > normalised_height )
		{
			scaled_width = scaled_width * normalised_height / scaled_height;
			scaled_height = normalised_height;
		}

		// Now apply the fill
		// TODO: Should combine fill/distort in one property
		if ( mlt_properties_get( properties, "fill" ) != NULL )
		{
			scaled_width = ( geometry->w / scaled_width ) * scaled_width;
			scaled_height = ( geometry->h / scaled_height ) * scaled_height;
		}

		// Save the new scaled dimensions
		geometry->sw = scaled_width;
		geometry->sh = scaled_height;
	}
	else
	{
		geometry->sw = geometry->w;
		geometry->sh = geometry->h;
	}

	// We want to ensure that we bypass resize now...
	mlt_properties_set( b_props, "distort", "true" );

	// Take into consideration alignment for optimisation
	alignment_calculate( geometry );

	// Adjust to consumer scale
	int x = geometry->x * *width / geometry->nw;
	int y = geometry->y * *height / geometry->nh;
	*width = geometry->sw * *width / geometry->nw;
	*height = geometry->sh * *height / geometry->nh;

	x -= x % 2;

	// optimization points - no work to do
	if ( *width <= 0 || *height <= 0 )
		return 1;

	if ( ( x < 0 && -x >= *width ) || ( y < 0 && -y >= *height ) )
		return 1;

	ret = mlt_frame_get_image( b_frame, image, &format, width, height, 1 );

	return ret;
}


struct geometry_s *composite_calculate( struct geometry_s *result, mlt_transition this, mlt_frame a_frame, float position )
{
	// Get the properties from the transition
	mlt_properties properties = mlt_transition_properties( this );

	// Get the properties from the frame
	mlt_properties a_props = mlt_frame_properties( a_frame );
	
	// Structures for geometry
	struct geometry_s *start = mlt_properties_get_data( properties, "geometries", NULL );

	// Now parse the geometries
	if ( start == NULL )
	{
		// Obtain the normalised width and height from the a_frame
		int normalised_width = mlt_properties_get_int( a_props, "normalised_width" );
		int normalised_height = mlt_properties_get_int( a_props, "normalised_height" );

		// Parse the transitions properties
		start = transition_parse_keys( this, normalised_width, normalised_height );
	}

	// Do the calculation
	geometry_calculate( result, start, position );

	// Now parse the alignment
	result->halign = alignment_parse( mlt_properties_get( properties, "halign" ) );
	result->valign = alignment_parse( mlt_properties_get( properties, "valign" ) );

	return start;
}

mlt_frame composite_copy_region( mlt_transition this, mlt_frame a_frame, mlt_position frame_position )
{
	// Create a frame to return
	mlt_frame b_frame = mlt_frame_init( );

	// Get the properties of the a frame
	mlt_properties a_props = mlt_frame_properties( a_frame );

	// Get the properties of the b frame
	mlt_properties b_props = mlt_frame_properties( b_frame );

	// Get the position
	float position = position_calculate( this, frame_position );

	// Destination image
	uint8_t *dest = NULL;

	// Get the image and dimensions
	uint8_t *image = mlt_properties_get_data( a_props, "image", NULL );
	int width = mlt_properties_get_int( a_props, "width" );
	int height = mlt_properties_get_int( a_props, "height" );

	// Pointers for copy operation
	uint8_t *p;
	uint8_t *q;
	uint8_t *r;

	// Corrdinates
	int w = 0;
	int h = 0;
	int x = 0;
	int y = 0;

	// Will need to know region to copy
	struct geometry_s result;

	// Calculate the region now
	composite_calculate( &result, this, a_frame, position );

	// Need to scale down to actual dimensions
	x = result.x * width / result.nw ;
	y = result.y * height / result.nh;
	w = result.w * width / result.nw;
	h = result.h * height / result.nh;

	x &= 0xfffffffe;
	w &= 0xfffffffe;

	// Now we need to create a new destination image
	dest = mlt_pool_alloc( w * h * 2 );

	// Copy the region of the image
	p = image + y * width * 2 + x * 2;
	q = dest;
	r = dest + w * h * 2; 

	while ( q < r )
	{
		memcpy( q, p, w * 2 );
		q += w * 2;
		p += width * 2;
	}

	// Assign to the new frame
	mlt_properties_set_data( b_props, "image", dest, w * h * 2, mlt_pool_release, NULL );
	mlt_properties_set_int( b_props, "width", w );
	mlt_properties_set_int( b_props, "height", h );

	// Assign this position to the b frame
	mlt_frame_set_position( b_frame, frame_position );

	// Return the frame
	return b_frame;
}

/** Get the image.
*/

static int transition_get_image( mlt_frame a_frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( a_frame );

	// Get the transition from the a frame
	mlt_transition this = mlt_frame_pop_service( a_frame );

	// This compositer is yuv422 only
	*format = mlt_image_yuv422;

	// Get the image from the a frame
	mlt_frame_get_image( a_frame, image, format, width, height, 1 );

	if ( b_frame != NULL )
	{
		// Get the properties of the a frame
		mlt_properties a_props = mlt_frame_properties( a_frame );

		// Get the properties of the b frame
		mlt_properties b_props = mlt_frame_properties( b_frame );

		// Get the properties from the transition
		mlt_properties properties = mlt_transition_properties( this );

		// Structures for geometry
		struct geometry_s result;

		// Calculate the position
		float position = mlt_properties_get_double( b_props, "relative_position" );
		float delta = delta_calculate( this, a_frame );

		// Do the calculation
		struct geometry_s *start = composite_calculate( &result, this, a_frame, position );
		
		// Optimisation - no compositing required
		if ( result.mix == 0 )
			return 0;

		// Since we are the consumer of the b_frame, we must pass along these
		// consumer properties from the a_frame
		mlt_properties_set_double( b_props, "consumer_aspect_ratio", mlt_properties_get_double( a_props, "consumer_aspect_ratio" ) );

		// Get the image from the b frame
		uint8_t *image_b = NULL;
		int width_b = *width;
		int height_b = *height;
		
		if ( get_b_frame_image( this, b_frame, &image_b, &width_b, &height_b, &result ) == 0 )
		{
			uint8_t *dest = *image;
			uint8_t *src = image_b;
			int bpp = 2;
			uint8_t *alpha = mlt_frame_get_alpha_mask( b_frame );
			int progressive = mlt_properties_get_int( a_props, "progressive" ) ||
					mlt_properties_get_int( a_props, "consumer_progressive" ) ||
					mlt_properties_get_int( properties, "progressive" );
			int field;
			
			int32_t luma_softness = mlt_properties_get_double( properties, "softness" ) * ( 1 << 16 );
			uint16_t *luma_bitmap = get_luma( properties, width_b, height_b );

			for ( field = 0; field < ( progressive ? 1 : 2 ); field++ )
			{
				// Assume lower field (0) first
				float field_position = position + field * delta;
				
				// Do the calculation if we need to
				geometry_calculate( &result, start, field_position );

				// Align
				alignment_calculate( &result );

				// Composite the b_frame on the a_frame
				composite_yuv( dest, *width, *height, bpp, src, width_b, height_b, alpha, result, progressive ? -1 : field, luma_bitmap, luma_softness );
			}
		}
	}

	return 0;
}

/** Composition transition processing.
*/

static mlt_frame composite_process( mlt_transition this, mlt_frame a_frame, mlt_frame b_frame )
{
	// Propogate the transition properties to the b frame
	mlt_properties_set_double( mlt_frame_properties( b_frame ), "relative_position", position_calculate( this, mlt_frame_get_position( a_frame ) ) );
	mlt_frame_push_service( a_frame, this );
	mlt_frame_push_get_image( a_frame, transition_get_image );
	mlt_frame_push_frame( a_frame, b_frame );
	return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_composite_init( char *arg )
{
	mlt_transition this = calloc( sizeof( struct mlt_transition_s ), 1 );
	if ( this != NULL && mlt_transition_init( this, NULL ) == 0 )
	{
		this->process = composite_process;
		mlt_properties_set( mlt_transition_properties( this ), "start", arg != NULL ? arg : "85%,5%:10%x10%" );
		
		// Default factory
		mlt_properties_set( mlt_transition_properties( this ), "factory", "fezzik" );
	}
	return this;
}
