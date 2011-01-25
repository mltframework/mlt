/*
 * rotoscoping.c -- keyframable vector based rotoscoping
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

#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX( x, y ) x > y ? x : y
#define MIN( x, y ) x < y ? x : y
#define SQR( x ) ( x ) * ( x )

/** x, y tuple with double precision */
typedef struct PointF
{
    double x;
    double y;
} PointF;

typedef struct BPointF
{
    struct PointF h1;
    struct PointF p;
    struct PointF h2;
} BPointF;

enum MODES { MODE_RGB, MODE_ALPHA, MODE_MATTE };
const char *MODESTR[3] = { "rgb", "alpha", "matte" };

enum ALPHAOPERATIONS { ALPHA_CLEAR, ALPHA_MAX, ALPHA_MIN, ALPHA_ADD, ALPHA_SUB };
const char *ALPHAOPERATIONSTR[5] = { "clear", "max", "min", "add", "sub" };


/** Returns the index of \param string in \param stringList.
 * Useful for assigning string parameters to enums. */
int stringValue( const char *string, const char **stringList, int max )
{
    int i;
    for ( i = 0; i < max; i++ )
        if ( strcmp( stringList[i], string ) == 0 )
            return i;
    return 0;
}

/** Linear interp */
void lerp( const PointF *a, const PointF *b, PointF *result, double t )
{
    result->x = a->x + ( b->x - a->x ) * t;
    result->y = a->y + ( b->y - a->y ) * t;
}

/** Linear interp. with t = 0.5
 * Speed gain? */
void lerpHalf( const PointF *a, const PointF *b, PointF *result )
{
    result->x = a->x + ( b->x - a->x ) * .5;
    result->y = a->y + ( b->y - a->y ) * .5;
}

/** Helper for using qsort with an array of integers. */
int ncompare( const void *a, const void *b )
{
    return *(const int*)a - *(const int*)b;
}

/** Turns a json array with two children into a point (x, y tuple). */
void jsonGetPoint( cJSON *json, PointF *point )
{
    if ( cJSON_GetArraySize( json ) == 2 )
    {
        point->x = json->child->valuedouble;
        point->y = json->child->next->valuedouble;
    }
}

/**
 * Turns the array of json elements into an array of Bézier points.
 * \param array cJSON array. values have to be Bézier points: handle 1, point , handl2
 *              ( [ [ [h1x, h1y], [px, py], [h2x, h2y] ], ... ] )
 * \param points pointer to array of points. Will be allocated and filled with the points in \param array
 * \return number of points
 */
int json2BCurves( cJSON *array, BPointF **points )
{
    int count = cJSON_GetArraySize( array );
    cJSON *child = array->child;
    *points = mlt_pool_alloc( count * sizeof( BPointF ) );

    int i = 0;
    do
    {
        if ( cJSON_GetArraySize( child ) == 3 )
        {
            jsonGetPoint( child->child , &(*points)[i].h1 );
            jsonGetPoint( child->child->next, &(*points)[i].p );
            jsonGetPoint( child->child->next->next, &(*points)[i].h2 );
            i++;
        }
    } while ( ( child = child->next ) );

    if ( i < count )
        *points = mlt_pool_realloc( *points, i * sizeof( BPointF ) );

    return i;
}

/**
 * Determines which points are located in the polygon and sets their value in \param map to \param value
 * \param vertices points defining the polygon
 * \param count number of vertices
 * \param with x range
 * \param height y range
 * \param value value identifying points in the polygon
 * \param map array of integers of the dimension width * height.
 *            The map entries belonging to the points in the polygon will be set to \param set * 255 the others to !set * 255.
 */
void fillMap( PointF *vertices, int count, int width, int height, uint8_t set, uint8_t *map )
{
    int nodes, nodeX[1024], pixelY, i, j, offset;

    memset( map, !set * 255, width * height );
    set *= 255;

    // Loop through the rows of the image
    for ( pixelY = 0; pixelY < height; pixelY++ )
    {
        offset = width * pixelY;
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
                for ( j = nodeX[i]; j <= nodeX[i+1]; j++ )
                    map[offset + j] = set;
            }
        }
    }
}

/** Determines the point in the middle of the Bézier curve (t = 0.5) defined by \param p1 and \param p2
 * using De Casteljau's algorithm.
 */
void deCasteljau( BPointF *p1, BPointF *p2, BPointF *mid )
{
    struct PointF ab, bc, cd, abbc, bccd, final;

    lerpHalf( &(p1->p), &(p1->h2), &ab );
    lerpHalf( &(p1->h2), &(p2->h1), &bc );
    lerpHalf( &(p2->h1), &(p2->p), &cd );
    lerpHalf( &ab, &bc, &abbc );
    lerpHalf( &bc, &cd, &bccd );
    lerpHalf( &abbc, &bccd, &final );

    p1->h2 = ab;
    p2->h1 = cd;
    mid->h1 = abbc;
    mid->p = final;
    mid->h2 = bccd;
}

/**
 * Calculates points for the cubic Bézier curve defined by \param p1 and \param p2.
 * Points are calculated until the squared distanced between neighbour points is smaller than \param errorSquared.
 * \param points Pointer to list of points. Will be allocted and filled with calculated points.
 * \param count Number of calculated points in \param points
 * \param size Allocated size of \param points (in elements not in bytes)
 */
void curvePoints( BPointF p1, BPointF p2, PointF **points, int *count, int *size, const double *errorSquared )
{
    double errorSqr = SQR( p1.p.x - p2.p.x ) + SQR( p1.p.y - p2.p.y );

    if ( *size + 1 >= *count )
    {
        *size += (int)sqrt( errorSqr / *errorSquared );
        *points = mlt_pool_realloc( *points, *size * sizeof ( struct PointF ) );
    }
    
    (*points)[(*count)++] = p1.p;

    if ( errorSqr <= *errorSquared )
        return;

    BPointF mid;
    deCasteljau( &p1, &p2, &mid );

    curvePoints( p1, mid, points, count, size, errorSquared );

    curvePoints( mid, p2, points, count, size, errorSquared );

    (*points)[*(count)++] = p2.p;
}

/** Do it :-).
*/
static int filter_get_image( mlt_frame this, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
    mlt_properties properties = MLT_FRAME_PROPERTIES( this );

    int mode = mlt_properties_get_int( properties, "mode" );

    // Get the image
    if ( mode == MODE_RGB )
        *format = mlt_image_rgb24;
    int error = mlt_frame_get_image( this, image, format, width, height, 1 );

    // Only process if we have no error and a valid colour space
    if ( !error )
    {
        BPointF *bpoints;
        struct PointF *points;
        int bcount, length, count, size, i, j;
        bpoints = mlt_properties_get_data( properties, "points", &length );
        bcount = length / sizeof( BPointF );

        for ( i = 0; i < bcount; i++ )
        {
            // map to image dimensions
            bpoints[i].h1.x *= *width;
            bpoints[i].p.x  *= *width;
            bpoints[i].h2.x *= *width;
            bpoints[i].h1.y *= *height;
            bpoints[i].p.y  *= *height;
            bpoints[i].h2.y *= *height;
        }

        double errorSqr = (double)SQR( mlt_properties_get_int( properties, "precision" ) );
        count = 0;
        size = 1;
        points = mlt_pool_alloc( size * sizeof( struct PointF ) );
        for ( i = 0; i < bcount; i++ )
        {
            j = (i + 1) % bcount;
            curvePoints( bpoints[i], bpoints[j], &points, &count, &size, &errorSqr );
        }

        if ( count )
        {
            uint8_t *map = mlt_pool_alloc( *width * *height );
            uint8_t setPoint = !mlt_properties_get_int( properties, "invert" );
            fillMap( points, count, *width, *height, setPoint, map );

            double bpp = 4;
            if ( mode != MODE_ALPHA )
            {
                if ( *format == mlt_image_rgb24 )
                    bpp = 3;
                else if ( *format == mlt_image_yuv422 )
                    bpp = 2;
                else if ( *format == mlt_image_yuv420p )
                    bpp = 3 / 2.;
            }

            i = 0;
            length = *width * *height;

            uint8_t *p = *image;
            uint8_t *q = *image + (int)( length * bpp );

            uint8_t *alpha;

            switch ( mode )
            {
            case MODE_RGB:
                // *format == mlt_image_rgb24
                while ( p != q )
                {
                    if ( !map[i++] )
                        p[0] = p[1] = p[2] = 0;
                    p += 3;
                }
                break;
            case MODE_MATTE:
                switch ( *format )
                {
                    case mlt_image_rgb24:
                    case mlt_image_rgb24a:
                    case mlt_image_opengl:
                        while ( p != q )
                        {
                            p[0] = p[1] = p[2] = map[i++];
                            p += (int)bpp;
                        }
                        break;
                    case mlt_image_yuv422:
                        while ( p != q )
                        {
                            p[0] = map[i++];
                            p[1] = 128;
                            p += 2;
                        }
                        break;
                    case mlt_image_yuv420p:
                        memcpy( p, map, length );
                        memset( p + length, 128, length / 2 );
                        break;
                    default:
                        break;
                }
                break;
            case MODE_ALPHA:
                alpha = mlt_frame_get_alpha_mask( this );
                switch ( mlt_properties_get_int( properties, "alpha_operation" ) )
                {
                case ALPHA_CLEAR:
                    memcpy( alpha, map, length );
                    break;
                case ALPHA_MAX:
                    for ( ; i < length; i++, alpha++ )
                        *alpha = MAX( map[i], *alpha );
                    break;
                case ALPHA_MIN:
                    for ( ; i < length; i++, alpha++ )
                        *alpha = MIN( map[i], *alpha );
                    break;
                case ALPHA_ADD:
                    for ( ; i < length; i++, alpha++ )
                        *alpha = MIN( *alpha + map[i], 255 );
                    break;
                case ALPHA_SUB:
                    for ( ; i < length; i++, alpha++ )
                        *alpha = MAX( *alpha - map[i], 0 );
                    break;
                }
                break;
            }

            mlt_pool_release( map );
        }

        mlt_pool_release( points );
    }

    return error;
}

/** Filter processing.
*/
static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
    mlt_properties properties = MLT_FILTER_PROPERTIES( this );
    mlt_properties frameProperties = MLT_FRAME_PROPERTIES( frame );
    char *spline = mlt_properties_get( properties, "spline" );
    char *splineOld = mlt_properties_get( properties, "spline_old" );
    char *modeStr = mlt_properties_get( properties, "mode" );

    cJSON *root;
    int newSpline = 1;
    if ( splineOld != NULL && strcmp( spline, splineOld ) == 0 ) {
        // the very same parameter was already parsed by json, use the saved json struct
        newSpline = 0;
        root = mlt_properties_get_data( properties, "spline_json", NULL );
    }
    else
    {
        root = cJSON_Parse( spline );
    }

    if ( root == NULL )
        return frame;

    BPointF *points;
    int count, i;

    if ( root->type == cJSON_Array )
    {
        /*
         * constant (over time)
         */
        count = json2BCurves( root, &points );
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
            count = json2BCurves( keyframe, &points );
        }
        else
        {
            /*
             * pos1 < time < pos2
             */

            BPointF *p1, *p2;
            int c1, c2;
            c1 = json2BCurves( keyframeOld, &p1 );
            c2 = json2BCurves( keyframe, &p2 );

            if ( c1 > c2 )
            {
                // number of points decreasing from p1 to p2; we can't handle this yet
                count = c2;
                points = mlt_pool_alloc( count * sizeof( BPointF ) );
                memcpy( points, p2, count * sizeof( BPointF ) );
                mlt_pool_release( p1 );
                mlt_pool_release( p2 );
            }
            else
            {
                // range 0-1
                double position = ( time - pos1 ) / (double)( pos2 - pos1 + 1 );

                count = c1;  // additional points in p2 are ignored
                points = mlt_pool_alloc( count * sizeof( BPointF ) );
                for ( i = 0; i < count; i++ )
                {
                    lerp( &(p1[i].h1), &(p2[i].h1), &(points[i].h1), position );
                    lerp( &(p1[i].p), &(p2[i].p), &(points[i].p), position );
                    lerp( &(p1[i].h2), &(p2[i].h2), &(points[i].h2), position );
                }

                mlt_pool_release( p1 );
                mlt_pool_release( p2 );
            }
        }
    }

    int length = count * sizeof( BPointF );

    if ( newSpline )
    {
        mlt_properties_set_data( properties, "spline_json", root, 0, (mlt_destructor)cJSON_Delete, NULL );
        mlt_properties_set( properties, "spline_old", strdup( spline ) );
    }

    mlt_properties_set_data( frameProperties, "points", points, length, (mlt_destructor)mlt_pool_release, NULL );
    mlt_properties_set_int( frameProperties, "mode", stringValue( modeStr, MODESTR, 3 ) );
    mlt_properties_set_int( frameProperties, "alpha_operation", stringValue( mlt_properties_get( properties, "alpha_operation" ), ALPHAOPERATIONSTR, 5 ) );
    mlt_properties_set_int( frameProperties, "invert", mlt_properties_get_int( properties, "invert" ) );
    mlt_properties_set_int( frameProperties, "precision", mlt_properties_get_int( properties, "precision" ) );
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
                mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "alpha_operation", "clear" );
                mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "invert", 0 );
                mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "precision", 1 );
                if ( arg != NULL )
                    mlt_properties_set( MLT_FILTER_PROPERTIES( this ), "spline", arg );
        }
        return this;
}
