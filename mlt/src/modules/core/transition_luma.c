/*
 * transition_luma.c -- a generic dissolve/wipe processor
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

#include "transition_luma.h"
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/** Luma class.
*/

typedef struct 
{
	struct mlt_transition_s parent;
	char *filename;
	int width;
	int height;                                      		
	double *bitmap;
}
transition_luma;


// forward declarations
static void transition_close( mlt_transition parent );


// image processing functions

static inline double smoothstep( double edge1, double edge2, double a )
{
	if ( a < edge1 )
		return 0.0;

	if ( a >= edge2 )
		return 1.0;

	a = ( a - edge1 ) / ( edge2 - edge1 );

	return ( a * a * ( 3 - 2 * a ) );
}

/** powerful stuff

    \param field_order -1 = progressive, 0 = lower field first, 1 = top field first
*/
static void luma_composite( mlt_frame this, mlt_frame b_frame, int luma_width, int luma_height,
							double *luma_bitmap, double pos, double frame_delta, double softness, int field_order )
{
	int width_src, height_src;
	int width_dest, height_dest;
	mlt_image_format format_src, format_dest;
	uint8_t *p_src, *p_dest;
	int i, j;
	int stride_src;
	int stride_dest;
	double weight = 0;
	int field;

	format_src = mlt_image_yuv422;
	format_dest = mlt_image_yuv422;

	mlt_frame_get_image( this, &p_dest, &format_dest, &width_dest, &height_dest, 1 /* writable */ );
	mlt_frame_get_image( b_frame, &p_src, &format_src, &width_src, &height_src, 0 /* writable */ );

	stride_src = width_src * 2;
	stride_dest = width_dest * 2;

	// composite using luma map
	for ( field = 0; field < ( field_order < 0 ? 1 : 2 ); ++field )
	{
		// Offset the position based on which field we're looking at ...
		double field_pos = pos + ( ( field_order == 0 ? 1 - field : field) * frame_delta * 0.5 );

		// adjust the position for the softness level
		field_pos *= ( 1.0 + softness );

		for ( i = field; i < height_src; i += ( field_order < 0 ? 1 : 2 ) )
		{
			uint8_t *p = &p_src[ i * stride_src ];
			uint8_t *q = &p_dest[ i * stride_dest ];
			uint8_t *o = &p_dest[ i * stride_dest ];
			double  *l = &luma_bitmap[ i * luma_width ];

			for ( j = 0; j < width_src; j ++ )
			{
				uint8_t y = *p ++;
				uint8_t uv = *p ++;
                weight = *l ++;
				double value = smoothstep( weight, weight + softness, field_pos );

				*o ++ = (uint8_t)( y * value + *q++ * ( 1 - value ) );
				*o ++ = (uint8_t)( uv * value + *q++ * ( 1 - value ) );
			}
		}
	}
}

/** Get the image.
*/

static int transition_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	// Get the properties of the a frame
	mlt_properties a_props = mlt_frame_properties( this );

	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( this );

	// Get the properties of the b frame
	mlt_properties b_props = mlt_frame_properties( b_frame );

	// Arbitrary composite defaults
	static double previous_mix = 0;
	double mix = 0;
	int luma_width = 0;
	int luma_height = 0;
	double *luma_bitmap = NULL;
	double luma_softness = 0;
	int progressive = 0;
	int top_field_first = 0;

	// mix is the offset time value in the duration of the transition
	// - also used as the mixing level for a dissolve
	if ( mlt_properties_get( b_props, "mix" ) != NULL )
		mix = mlt_properties_get_double( b_props, "mix" );

	// (mix - previous_mix) is the animation delta, if backwards reset previous
	if ( mix < previous_mix )
		previous_mix = 0;

	// Get the interlace and field properties of the frame
	if ( mlt_properties_get( b_props, "progressive" ) != NULL )
		progressive = mlt_properties_get_int( b_props, "progressive" );
	if ( mlt_properties_get( b_props, "top_field_first" ) != NULL )
		top_field_first = mlt_properties_get_int( b_props, "top_field_first" );

	// Get the luma map parameters
	if ( mlt_properties_get( b_props, "luma.width" ) != NULL )
		luma_width = mlt_properties_get_int( b_props, "luma.width" );
	if ( mlt_properties_get( b_props, "luma.height" ) != NULL )
		luma_height = mlt_properties_get_int( b_props, "luma.height" );
	if ( mlt_properties_get( b_props, "luma.softness" ) != NULL )
		luma_softness = mlt_properties_get_double( b_props, "luma.softness" );
	luma_bitmap = (double*) mlt_properties_get_data( b_props, "luma.bitmap", NULL );

	if ( luma_width > 0 && luma_height > 0 && luma_bitmap != NULL )
		// Composite the frames using a luma map
		luma_composite( this, b_frame, luma_width, luma_height, luma_bitmap, mix, mix - previous_mix,
			luma_softness, progressive > 0 ? -1 : top_field_first );
	else
		// Dissolve the frames using the time offset for mix value
		mlt_frame_composite_yuv( this, b_frame, 0, 0, mix );

	// Extract the a_frame image info
	*width = mlt_properties_get_int( a_props, "width" );
	*height = mlt_properties_get_int( a_props, "height" );
	*image = mlt_properties_get_data( a_props, "image", NULL );

	// Close the b_frame
	mlt_frame_close( b_frame );
	
	previous_mix = mix;

	return 0;
}

static int transition_get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the properties of the a frame
	mlt_properties a_props = mlt_frame_properties( frame );

	// Get the b frame from the stack
	mlt_frame b_frame = mlt_frame_pop_frame( frame );

	// Get the properties of the b frame
	mlt_properties b_props = mlt_frame_properties( b_frame );

	// Restore the original get_audio
	frame->get_audio = mlt_properties_get_data( a_props, "get_audio", NULL );
	
	double mix = 0;
	if ( mlt_properties_get( b_props, "mix" ) != NULL )
		mix = mlt_properties_get_double( b_props, "mix" );
	mlt_frame_mix_audio( frame, b_frame, mix, buffer, format, frequency, channels, samples );

	// Push the b_frame back on for get_image
	mlt_frame_push_frame( frame, b_frame );

	return 0;
}


/** Load the luma map from PGM stream.
*/

static void luma_read_pgm( FILE *f, double **map, int *width, int *height )
{
	uint8_t *data = NULL;
	while (1)
	{
		char line[128];
		int i = 2;
		int maxval;
		int bpp;
		double *p;
		
		line[127] = '\0';

		// get the magic code
		if ( fgets( line, 127, f ) == NULL )
			break;
		if ( line[0] != 'P' || line[1] != '5' )
			break;

		// skip white space and see if a new line must be fetched
		for ( i = 2; i < 127 && line[i] != '\0' && isspace( line[i] ); i++ );
		if ( line[i] == '\0' && fgets( line, 127, f ) == NULL )
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
			i = sscanf( line, "%d", &maxval );
			if ( i == 0 )
				break;
		}

		// determine if this is one or two bytes per pixel
		bpp = maxval > 255 ? 2 : 1;
			// allocate temporary storage for the raw data
		data = malloc( *width * *height * bpp );
		if ( data == NULL )
			break;

		// read the raw data
		if ( fread( data, *width * *height * bpp, 1, f ) != 1 )
			break;
		
		// allocate the luma bitmap
		*map =  p = (double*) malloc( *width * *height * sizeof( double ) );
		if ( *map == NULL )
			break;

		// proces the raw data into the luma bitmap
		for ( i = 0; i < *width * *height * bpp; i += bpp )
		{
			if ( bpp == 1 )
				*p++ = (double) data[ i ] / (double) maxval;
			else
				*p++ = (double) ( ( data[ i ] << 8 ) + data[ i+1 ] ) / (double) maxval;
		}

		break;
	}
		
	if ( data != NULL )
		free( data );
}


/** Luma transition processing.
*/

static mlt_frame transition_process( mlt_transition transition, mlt_frame a_frame, mlt_frame b_frame )
{
	transition_luma *this = (transition_luma*) transition->child;

	// Get the properties of the transition
	mlt_properties properties = mlt_transition_properties( transition );
	
	// Get the properties of the b frame
	mlt_properties b_props = mlt_frame_properties( b_frame );

	// If the filename property changed, reload the map
	char *luma_file = mlt_properties_get( properties, "filename" );
	if ( luma_file != NULL && ( this->filename == NULL || ( this->filename && strcmp( luma_file, this->filename ) ) ) )
	{
		int width = mlt_properties_get_int( b_props, "width" );
		int height = mlt_properties_get_int( b_props, "height" );
		char command[ 512 ];
		FILE *pipe;
		
		command[ 511 ] = '\0';
		if ( this->filename )
			free( this->filename );
		this->filename = strdup( luma_file );
		snprintf( command, 511, "anytopnm %s | pnmscale -width %d -height %d", luma_file, width, height );
		//pipe = popen( command, "r" );
		pipe = fopen( luma_file, "r" );
		if ( pipe != NULL )
		{
			if ( this->bitmap )
				free( this->bitmap );
			luma_read_pgm( pipe, &this->bitmap, &this->width, &this->height );
			//pclose( pipe );
			fclose( pipe );
		}
		
	}

	// Determine the time position of this frame in the transition duration
	mlt_position in = mlt_transition_get_in( transition );
	mlt_position out = mlt_transition_get_out( transition );
	mlt_position time = mlt_frame_get_position( b_frame );
	double pos = ( time - in ) / ( out - in );
	
	// Set the b frame properties
	mlt_properties_set_double( b_props, "mix", pos );
	mlt_properties_set_int( b_props, "luma.width", this->width );
	mlt_properties_set_int( b_props, "luma.height", this->height );
	mlt_properties_set_data( b_props, "luma.bitmap", this->bitmap, 0, NULL, NULL );
	if ( mlt_properties_get( properties, "softness" ) != NULL )
		mlt_properties_set_double( b_props, "luma.softness", mlt_properties_get_double( properties, "softness" ) );

	mlt_frame_push_get_image( a_frame, transition_get_image );
	mlt_frame_push_frame( a_frame, b_frame );

/************************ AUDIO ***************************/
#if 1
	// Backup the original get_audio (it's still needed)
	mlt_properties_set_data( mlt_frame_properties( a_frame ), "get_audio", a_frame->get_audio, 0, NULL, NULL );

	// Override the get_audio method
	a_frame->get_audio = transition_get_audio;
#endif
	return a_frame;
}

/** Constructor for the filter.
*/

mlt_transition transition_luma_init( char *lumafile )
{
	transition_luma *this = calloc( sizeof( transition_luma ), 1 );
	if ( this != NULL )
	{
		mlt_transition transition = &this->parent;
		mlt_transition_init( transition, this );
		transition->process = transition_process;
		transition->close = transition_close;

		if ( lumafile != NULL )
			mlt_properties_set( mlt_transition_properties( transition ), "filename", lumafile );
		
		return &this->parent;
	}
	return NULL;
}

/** Close the transition.
*/

static void transition_close( mlt_transition parent )
{
	transition_luma *this = (transition_luma*) parent->child;

	if ( this->bitmap )
		free( this->bitmap );
	
	if ( this->filename )
		free( this->filename );

	free( this );
}

