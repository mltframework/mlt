/*
 * rotoscoping.c -- (not yet)keyframable vector based rotoscoping
 * Copyright (C) 2011 Till Theato <root@ttill.de>
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_tokeniser.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX( x, y ) x > y ? x : y
#define MIN( x, y ) x < y ? x : y

/** x, y pair with double precision */
typedef struct PointF
{
    double x;
    double y;
} PointF;

/**
 * Extracts the polygon points stored in \param string
 * \param string string containing the points in the format: x1;y1#x2;y2#...
 * \param points pointer to array of points. Will be allocated and filled with the points in \param string
 * \return number of points
 */
int parsePolygonString( char *string, PointF **points )
{
    // Create a tokeniser
    mlt_tokeniser tokens = mlt_tokeniser_init( );

    // Tokenise
    if ( string != NULL )
        mlt_tokeniser_parse_new( tokens, string, "#" );

    // Iterate through each token
    int count = mlt_tokeniser_count( tokens );
    *points = malloc( (count + 1) * sizeof( struct PointF ) );
    int i;
    for ( i = 0; i < count; ++i )
    {
        char *value = mlt_tokeniser_get_string( tokens, i );
        (*points)[i].x = atof( value );
        (*points)[i].y = atof( strchr( value, ';' ) + 1 );
    }

    // Remove the tokeniser
    mlt_tokeniser_close( tokens );

    return count;
}

/** helper for using qsort with an array of integers. */
int ncompare( const void *a, const void *b )
{
    return *(const int*)a - *(const int*)b;
}

/**
 * Determines which points are located in the polygon and sets their value in \param map to 1
 * \param vertices points defining the polygon
 * \param count number of vertices
 * \param with x range
 * \param height y range
 * \param map array of integers of the dimension width * height.
 *            The map entries belonging to the points in the polygon will be set to 1, the other entries remain untouched.
 * 
 * based on public-domain code by Darel Rex Finley, 2007
 * \see: http://alienryderflex.com/polygon_fill/
 */
void fillMap( PointF *vertices, int count, int width, int height, int *map )
{
    int nodes, nodeX[1024], pixelX, pixelY, i, j, swap;
    
    // Loop through the rows of the image
    for ( pixelY = 0; pixelY < height; pixelY++ )
    {
        /*
         * Build a list of nodes.
         * nodes are located at the borders of the polygon
         * and therefore indicate a move from in to out or vice versa
         */
        nodes = 0;
        for ( i = 0, j = count - 1; i < count; j = i++ )
            if ( (vertices[i].y > (double)pixelY) != (vertices[j].y > (double)pixelY) )
                nodeX[nodes++] = (int)(vertices[i].x + (pixelY - vertices[i].y) / (vertices[j].y - vertices[i].y) * (vertices[j].x - vertices[i].x) );

        qsort( nodeX, nodes, sizeof( int ), ncompare );

        // Fill the pixels between node pairs.
        for ( i = 0; i < nodes; i += 2 )
        {
            if ( nodeX[i] >= width )
                break;
            if ( nodeX[i+1] > 0 )
            {
                nodeX[i] = MAX( 0, nodeX[i] );
                nodeX[i+1] = MIN( nodeX[i+1], width );
                for ( j = nodeX[i]; j < nodeX[i+1]; j++ )
                    map[width * pixelY + j] = 1;
            }
        }
    }
}

/** Do it :-).
*/
static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
    // Get the image
    *format = mlt_image_rgb24a;
    int error = mlt_frame_get_image( this, image, format, width, height, 1 );

    // Only process if we have no error and a valid colour space
    if ( !error )
    {
        struct PointF *points;
        int length, count;
        points = mlt_properties_get_data( MLT_FRAME_PROPERTIES( this ), "points", &length );
        count = length / sizeof( struct PointF );

        int i;
        for ( i = 0; i < count; ++i ) {
            points[i].x *= *width;
            points[i].y *= *height;
        }

        if ( count )
        {
            int *map = calloc( *width * *height, sizeof( int ) );
            fillMap( points, count, *width, *height, map );

            uint8_t *p = *image;
            uint8_t *q = *image + *width * *height * 4;

            while ( p != q )
            {
                int pixel = (p - *image) / 4;
                if ( !map[pixel] ) {
                    // pixel is not in polygon
                    // => set to black
                    p[0] = p[1] = p[2] = 0;
                }
                p += 4;
            }

            free( map );
        }
    }

    return error;
}

/** Filter processing.
*/
static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
    char *polygon = mlt_properties_get( MLT_FILTER_PROPERTIES( this ), "polygon" );

    if ( polygon == NULL || strcmp( polygon, "" ) == 0 ) {
        // :( we are redundant
        return frame;
    }

    struct PointF *points;
    int count = parsePolygonString( polygon, &points );
    int length = count * sizeof( struct PointF );

    mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "points", points, length, free, NULL );
    mlt_frame_push_get_image( frame, filter_get_image );

    return frame;
}

/** Constructor for the filter.
*/
mlt_filter filter_rotoscoping_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
        mlt_filter this = mlt_filter_new( );
        if ( this != NULL )
        {
                this->process = filter_process;
                if ( arg != NULL )
                    mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "polygon", arg );
        }
        return this;
}
