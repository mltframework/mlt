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

#include "cJSON.h"

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

enum MODES { MODE_RGB, MODE_ALPHA, MODE_MASK };


/**
 * Turns the array of json elements into an array of points.
 * \param array cJSON array. values have to be arrays of 2 doubles ( [ [x, y], [x, y], ... ] )
 * \param points pointer to array of points. Will be allocated and filled with the points in \param array
 * \return number of points
 */
int json2Polygon( cJSON *array, PointF **points )
{
    int count = cJSON_GetArraySize( array );
    cJSON *child = array->child;
    *points = malloc( count * sizeof( struct PointF ) );

    int i = 0;
    do
    {
        if ( cJSON_GetArraySize( child ) == 2 )
        {
            (*points)[i].x = child->child->valuedouble;
            (*points)[i].y = child->child->next->valuedouble;
            i++;
        }
    } while ( ( child = child->next ) );

    if ( i < count )
        *points = realloc( *points, i * sizeof( struct PointF ) );

    return i;
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
    int nodes, nodeX[1024], pixelY, i, j;
    
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

        // Set map values for points between the node pairs to 1
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

            int pixel;
            switch ( mlt_properties_get_int( MLT_FRAME_PROPERTIES( this ), "mode" ) )
            {
            case MODE_RGB:
                while ( p != q )
                {
                    pixel = (p - *image) / 4;
                    if ( !map[pixel] )
                        p[0] = p[1] = p[2] = 0;
                    p += 4;
                }
                break;
            case MODE_ALPHA:
                while ( p != q )
                {
                    pixel = (p - *image) / 4;
                    p[3] = map[pixel] * 255;
                    p += 4;
                }
                break;
            case MODE_MASK:
                while ( p != q )
                {
                    pixel = (p - *image) / 4;
                    p[0] = p[1] = p[2] = map[pixel] * 255;
                    p += 4;
                }
                break;
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
    char *modeStr = mlt_properties_get( MLT_FILTER_PROPERTIES( this ), "mode" );

    if ( polygon == NULL || strcmp( polygon, "" ) == 0 ) {
        // :( we are redundant
        return frame;
    }

    cJSON *root = cJSON_Parse( polygon );

    if ( root == NULL ) {
        // parameter malformed
        return frame;
    }

    struct PointF *points;
    int count;

    if ( root->type == cJSON_Array )
    {
        /*
         * constant (over time)
         */

        count = json2Polygon( root, &points );
    }
    else
    {
        /*
         * keyframes
         */

        mlt_position time, pos1, pos2;
        time = mlt_frame_get_position( frame );

        cJSON *keyframe = root->child;
        cJSON *keyframeOld = NULL;
        while ( atoi( keyframe->string ) < time && keyframe->next )
        {
            keyframeOld = keyframe;
            keyframe = keyframe->next;
        }

        if ( keyframeOld == NULL ) {
            // parameter has only 1 keyframe or we are before the 1. keyframe
            keyframeOld = keyframe;
        }

        if ( !keyframe )
        {
            if ( keyframeOld )
            {
                // parameter malformed
                keyframe = keyframeOld;
            }
            else if ( root->child )
            {
                // parameter malformed
                keyframe = root->child;
                keyframeOld = keyframe;
            }
            else
            {
                // json object has no children
                cJSON_Delete( root );
                return frame;
            }
        }

        pos2 = atoi( keyframe->string );
        pos1 = atoi( keyframeOld->string );

        if ( pos1 >= pos2 || time >= pos2 )
        {
            // keyframes in wrong order or after last keyframe
            count = json2Polygon( keyframe, &points );
        }
        else
        {
            /*
             * pos1 < time < pos2
             */

            struct PointF *p1, *p2;
            int c1, c2;
            c1 = json2Polygon( keyframeOld, &p1 );
            c2 = json2Polygon( keyframe, &p2 );

            if ( c1 > c2 )
            {
                // number of points decreasing from p1 to p2; we can't handle this yet
                count = c2;
                points = malloc( count * sizeof( struct PointF ) );
                memcpy( points, p2, count * sizeof( struct PointF ) );
                free( p1 );
                free( p2 );
            }
            else
            {
                // range 0-1
                double position = ( time - pos1 ) / (double)( pos2 - pos1 + 1 );

                count = c1;  // additional points in p2 are ignored
                points = malloc( count * sizeof( struct PointF ) );
                int i;
                for ( i = 0; i < count; i++ )
                {
                    // linear interp.
                    points[i].x = p1[i].x + ( p2[i].x - p1[i].x ) * position;
                    points[i].y = p1[i].y + ( p2[i].y - p1[i].y ) * position;
                }

                free( p1 );
                free( p2 );
            }
        }
    }

    int length = count * sizeof( struct PointF );
    cJSON_Delete( root );


    enum MODES mode = MODE_RGB;
    if ( strcmp( modeStr, "rgb" ) == 0 )
        mode = MODE_RGB;
    else if ( strcmp( modeStr, "alpha" ) == 0 )
        mode = MODE_ALPHA;
    else if ( strcmp( modeStr, "mask" ) == 0)
        mode = MODE_MASK;

    mlt_properties_set_data( MLT_FRAME_PROPERTIES( frame ), "points", points, length, free, NULL );
    mlt_properties_set_int( MLT_FRAME_PROPERTIES( frame ), "mode", (int)mode );
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
                mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "mode", "rgb" );
                if ( arg != NULL )
                    mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "polygon", arg );
        }
        return this;
}
