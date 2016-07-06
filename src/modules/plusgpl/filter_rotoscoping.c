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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

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

enum MODES { MODE_RGB, MODE_ALPHA, MODE_LUMA };
const char *MODESTR[3] = { "rgb", "alpha", "luma" };

enum ALPHAOPERATIONS { ALPHA_CLEAR, ALPHA_MAX, ALPHA_MIN, ALPHA_ADD, ALPHA_SUB };
const char *ALPHAOPERATIONSTR[5] = { "clear", "max", "min", "add", "sub" };


/** Returns the index of \param string in \param stringList.
 * Useful for assigning string parameters to enums. */
static int stringValue( const char *string, const char **stringList, int max )
{
    int i;
    for ( i = 0; i < max; i++ )
        if ( strcmp( stringList[i], string ) == 0 )
            return i;
    return 0;
}

/** Sets "spline_is_dirty" to 1 if property "spline" was changed.
 * We then know when to parse the json stored in "spline" */
static void rotoPropertyChanged( mlt_service owner, mlt_filter this, char *name )
{
    if ( !strcmp( name, "spline" ) )
        mlt_properties_set_int( MLT_FILTER_PROPERTIES( this ), "_spline_is_dirty", 1 );
}

/** Linear interp */
static inline void lerp( const PointF *a, const PointF *b, PointF *result, double t )
{
    result->x = a->x + ( b->x - a->x ) * t;
    result->y = a->y + ( b->y - a->y ) * t;
}

/** Linear interp. with t = 0.5
 * Speed gain? */
static inline void lerpHalf( const PointF *a, const PointF *b, PointF *result )
{
    result->x = ( a->x + b->x ) * .5;
    result->y = ( a->y + b->y ) * .5;
}

/** Helper for using qsort with an array of integers. */
int ncompare( const void *a, const void *b )
{
    return *(const int*)a - *(const int*)b;
}

/** Turns a json array with two children into a point (x, y tuple). */
static void jsonGetPoint( cJSON *json, PointF *point )
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
static int json2BCurves( cJSON *array, BPointF **points )
{
    int count = cJSON_GetArraySize( array );
    cJSON *child = array->child;
    *points = mlt_pool_alloc( count * sizeof( BPointF ) );

    int i = 0;
    do
    {
        if ( child && cJSON_GetArraySize( child ) == 3 )
        {
            jsonGetPoint( child->child , &(*points)[i].h1 );
            jsonGetPoint( child->child->next, &(*points)[i].p );
            jsonGetPoint( child->child->next->next, &(*points)[i].h2 );
            i++;
        }
    } while ( child && ( child = child->next ) );

    if ( i < count )
        *points = mlt_pool_realloc( *points, i * sizeof( BPointF ) );

    return i;
}

/** Blurs \param src horizontally. \See funtion blur. */
static void blurHorizontal( uint8_t *src, uint8_t *dst, int width, int height, int radius)
{
    int x, y, kx, yOff, total, amount, amountInit;
    amountInit = radius * 2 + 1;
    for (y = 0; y < height; ++y)
    {
        total = 0;
        yOff = y * width;
        // Process entire window for first pixel
        int size = MIN(radius + 1, width);
        for ( kx = 0; kx < size; ++kx )
            total += src[yOff + kx];
        dst[yOff] = total / ( radius + 1 );
        // Subsequent pixels just update window total
        for ( x = 1; x < width; ++x )
        {
            amount = amountInit;
            // Subtract pixel leaving window
            if ( x - radius - 1 >= 0 )
                total -= src[yOff + x - radius - 1];
            else
                amount -= radius - x;
            // Add pixel entering window
            if ( x + radius < width )
                total += src[yOff + x + radius];
            else
                amount -= radius - width + x;
            dst[yOff + x] = total / amount;
        }
    }
}

/** Blurs \param src vertically. \See funtion blur. */
static void blurVertical( uint8_t *src, uint8_t *dst, int width, int height, int radius)
{
    int x, y, ky, total, amount, amountInit;
    amountInit = radius * 2 + 1;
    for (x = 0; x < width; ++x)
    {
        total = 0;
        int size = MIN(radius + 1, height);
        for ( ky = 0; ky < size; ++ky )
            total += src[x + ky * width];
        dst[x] = total / ( radius + 1 );
        for ( y = 1; y < height; ++y )
        {
            amount = amountInit;
            if ( y - radius - 1 >= 0 )
                total -= src[( y - radius - 1 ) * width + x];
            else
                amount -= radius - y;
            if ( y + radius < height )
                total += src[( y + radius ) * width + x];
            else
                amount -= radius - height + y;
            dst[y * width + x] = total / amount;
        }
    }
}

/**
 * Blurs the \param map using a simple "average" blur.
 * \param map Will be blured; 1bpp
 * \param width x dimension of channel stored in \param map
 * \param height y dimension of channel stored in \param map
 * \param radius blur radius
 * \param passes blur passes
 */
static void blur( uint8_t *map, int width, int height, int radius, int passes )
{
    uint8_t *src = mlt_pool_alloc( width * height );
    uint8_t *tmp = mlt_pool_alloc( width * height );

    int i;
    for ( i = 0; i < passes; ++i )
    {
        memcpy( src, map, width * height );
        blurHorizontal( src, tmp, width, height, radius );
        blurVertical( tmp, map, width, height, radius );
    }

    mlt_pool_release(src);
    mlt_pool_release(tmp);
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
static void fillMap( PointF *vertices, int count, int width, int height, int invert, uint8_t *map )
{
    int nodes, nodeX[1024], pixelY, i, j, value;

    value = !invert * 255;
    memset( map, invert * 255, width * height );

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
                memset( map + width * pixelY + nodeX[i], value, nodeX[i+1] - nodeX[i] );
            }
        }
    }
}

/** Determines the point in the middle of the Bézier curve (t = 0.5) defined by \param p1 and \param p2
 * using De Casteljau's algorithm.
 */
static void deCasteljau( BPointF *p1, BPointF *p2, BPointF *mid )
{
    struct PointF ab, bc, cd;

    lerpHalf( &(p1->p), &(p1->h2), &ab );
    lerpHalf( &(p1->h2), &(p2->h1), &bc );
    lerpHalf( &(p2->h1), &(p2->p), &cd );
    lerpHalf( &ab, &bc, &(mid->h1) ); // mid->h1 = abbc
    lerpHalf( &bc, &cd, &(mid->h2) ); // mid->h2 = bccd
    lerpHalf( &(mid->h1), &(mid->h2), &(mid->p) );

    p1->h2 = ab;
    p2->h1 = cd;
}

/**
 * Calculates points for the cubic Bézier curve defined by \param p1 and \param p2.
 * Points are calculated until the squared distanced between neighbour points is smaller than 2.
 * \param points Pointer to list of points. Will be allocted and filled with calculated points.
 * \param count Number of calculated points in \param points
 * \param size Allocated size of \param points (in elements not in bytes)
 */
static void curvePoints( BPointF p1, BPointF p2, PointF **points, int *count, int *size )
{
    double errorSqr = SQR( p1.p.x - p2.p.x ) + SQR( p1.p.y - p2.p.y );

    if ( *size + 1 >= *count )
    {
        *size += (int)sqrt( errorSqr / 2 );
        *points = mlt_pool_realloc( *points, *size * sizeof ( struct PointF ) );
    }
    
    (*points)[(*count)++] = p1.p;

    if ( errorSqr <= 2 )
        return;

    BPointF mid;
    deCasteljau( &p1, &p2, &mid );

    curvePoints( p1, mid, points, count, size );

    curvePoints( mid, p2, points, count, size );

    (*points)[*(count)++] = p2.p;
}

/** Do it :-).
*/
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
    mlt_properties unique = mlt_frame_pop_service( frame );

    int mode = mlt_properties_get_int( unique, "mode" );

    // Get the image
    if ( mode == MODE_RGB )
        *format = mlt_image_rgb24;
    int error = mlt_frame_get_image( frame, image, format, width, height, writable );

    // Only process if we have no error and a valid colour space
    if ( !error )
    {
        BPointF *bpoints;
        struct PointF *points;
        int bcount, length, count, size, i, j;
        bpoints = mlt_properties_get_data( unique, "points", &length );
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

        count = 0;
        size = 1;
        points = mlt_pool_alloc( size * sizeof( struct PointF ) );
        for ( i = 0; i < bcount; i++ )
        {
            j = (i + 1) % bcount;
            curvePoints( bpoints[i], bpoints[j], &points, &count, &size );
        }

        if ( count )
        {
            length = *width * *height;
            uint8_t *map = mlt_pool_alloc( length );
            int invert = mlt_properties_get_int( unique, "invert" );
            fillMap( points, count, *width, *height, invert, map );

            int feather = mlt_properties_get_int( unique, "feather" );
            if ( feather && mode != MODE_RGB )
                blur( map, *width, *height, feather, mlt_properties_get_int( unique, "feather_passes" ) );

            int bpp;
            size = mlt_image_format_size( *format, *width, *height, &bpp );
            uint8_t *p = *image;
            uint8_t *q = *image + size;

            i = 0;
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
            case MODE_LUMA:
                switch ( *format )
                {
                    case mlt_image_rgb24:
                    case mlt_image_rgb24a:
                    case mlt_image_opengl:
                        while ( p != q )
                        {
                            p[0] = p[1] = p[2] = map[i++];
                            p += bpp;
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
                switch ( *format )
                {
                case mlt_image_rgb24a:
                case mlt_image_opengl:
                    switch ( mlt_properties_get_int( unique, "alpha_operation" ) )
                    {
                    case ALPHA_CLEAR:
                        while ( p != q )
                        {
                            p[3] = map[i++];
                            p += 4;
                        }
                        break;
                    case ALPHA_MAX:
                        while ( p != q )
                        {
                            p[3] = MAX( p[3], map[i] );
                            p += 4;
                            i++;
                        }
                        break;
                    case ALPHA_MIN:
                        while ( p != q )
                        {
                            p[3] = MIN( p[3], map[i] );
                            p += 4;
                            i++;
                        }
                        break;
                    case ALPHA_ADD:
                        while ( p != q )
                        {
                            p[3] = MIN( p[3] + map[i], 255 );
                            p += 4;
                            i++;
                        }
                        break;
                    case ALPHA_SUB:
                        while ( p != q )
                        {
                            p[3] = MAX( p[3] - map[i], 0 );
                            p += 4;
                            i++;
                        }
                        break;
                    }
                    break;
                default:
                    alpha = mlt_frame_get_alpha_mask( frame );
                    switch ( mlt_properties_get_int( unique, "alpha_operation" ) )
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
static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
    mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
    int splineIsDirty = mlt_properties_get_int( properties, "_spline_is_dirty" );
    char *modeStr = mlt_properties_get( properties, "mode" );
    cJSON *root = mlt_properties_get_data( properties, "_spline_parsed", NULL );

    if ( splineIsDirty || root == NULL )
    {
        // we need to (re-)parse
        char *spline = mlt_properties_get( properties, "spline" );
        root = cJSON_Parse( spline );
        mlt_properties_set_data( properties, "_spline_parsed", root, 0, (mlt_destructor)cJSON_Delete, NULL );
        mlt_properties_set_int( properties, "_spline_is_dirty", 0 );
    }

    if ( root == NULL )
        return frame;

    BPointF *points;
    int count, i;

    if ( root->type == cJSON_Array )
    {
        /*
         * constant
         */
        count = json2BCurves( root, &points );
    }
    else if ( root->type == cJSON_Object )
    {
        /*
         * keyframes
         */

        mlt_position time, pos1, pos2;
        time = mlt_frame_get_position( frame );

        cJSON *keyframe = root->child;
        cJSON *keyframeOld = keyframe;

        if ( !keyframe )
            return frame;

        while ( atoi( keyframe->string ) < time && keyframe->next )
        {
            keyframeOld = keyframe;
            keyframe = keyframe->next;
        }

        pos1 = atoi( keyframeOld->string );
        pos2 = atoi( keyframe->string );

        if ( pos1 >= pos2 || time >= pos2 )
        {
            // keyframes in wrong order or before first / after last keyframe
            count = json2BCurves( keyframe, &points );
        }
        else
        {
            /*
             * pos1 < time < pos2
             */

            BPointF *p1, *p2;
            int c1 = json2BCurves( keyframeOld, &p1 );
            int c2 = json2BCurves( keyframe, &p2 );

            // range 0-1
            double position = ( time - pos1 ) / (double)( pos2 - pos1 + 1 );

            count = MIN( c1, c2 );  // additional points are ignored
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
    else
    {
        return frame;
    }

    mlt_properties unique = mlt_frame_unique_properties( frame, MLT_FILTER_SERVICE( filter ) );
    mlt_properties_set_data( unique, "points", points, count * sizeof( BPointF ), (mlt_destructor)mlt_pool_release, NULL );
    mlt_properties_set_int( unique, "mode", stringValue( modeStr, MODESTR, 3 ) );
    mlt_properties_set_int( unique, "alpha_operation", stringValue( mlt_properties_get( properties, "alpha_operation" ), ALPHAOPERATIONSTR, 5 ) );
    mlt_properties_set_int( unique, "invert", mlt_properties_get_int( properties, "invert" ) );
    mlt_properties_set_int( unique, "feather", mlt_properties_get_int( properties, "feather" ) );
    mlt_properties_set_int( unique, "feather_passes", mlt_properties_get_int( properties, "feather_passes" ) );
    mlt_frame_push_service( frame, unique );
    mlt_frame_push_get_image( frame, filter_get_image );

    return frame;
}

/** Constructor for the filter.
*/
mlt_filter filter_rotoscoping_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
        mlt_filter filter = mlt_filter_new( );
        if ( filter )
        {
                filter->process = filter_process;
                mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
                mlt_properties_set( properties, "mode", "alpha" );
                mlt_properties_set( properties, "alpha_operation", "clear" );
                mlt_properties_set_int( properties, "invert", 0 );
                mlt_properties_set_int( properties, "feather", 0 );
                mlt_properties_set_int( properties, "feather_passes", 1 );
                if ( arg )
                    mlt_properties_set( properties, "spline", arg );

                mlt_events_listen( properties, filter, "property-changed", (mlt_listener)rotoPropertyChanged );
        }
        return filter;
}
