/*
 *  transform.h
 *
 *  Copyright (C) Georg Martius - June 2007
 *
 *  This file is part of transcode, a video stream processing tool
 *      
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */
#ifndef __TRANSFORM_H
#define __TRANSFORM_H

#define DEFAULT_TRANS_FILE_NAME     "transforms.dat"

/* structure to hold information about frame transformations 
   x,y are translations, alpha is a rotation around the center in RAD,
   zoom is a percentage to zoom in and
   extra is for additional information like scene cut (unused)
 */
typedef struct _transform {
    double x;
    double y;
    double alpha;
    double zoom;
    int extra;    /* -1: ignore transform (only internal use);
                     0 for normal trans; 1 for inter scene cut (unused) */
} Transform;

/* helper functions to create and operate with transforms.
 * all functions are non-destructive
 * the "_" version uses non-pointer Transforms. This is slower
 * but useful when cascading calculations like 
 * add_transforms_(mult_transform(&t1, 5.0), &t2) 
 */
Transform null_transform(void);
Transform new_transform(double x, double y, double alpha, 
                        double zoom, int extra);
Transform add_transforms(const Transform* t1, const Transform* t2);
Transform add_transforms_(const Transform t1, const Transform t2);
Transform sub_transforms(const Transform* t1, const Transform* t2);
Transform mult_transform(const Transform* t1, double f);
Transform mult_transform_(const Transform t1, double f);

/* compares a transform with respect to x (for sort function) */
int cmp_trans_x(const void *t1, const void* t2);
/* compares a transform with respect to y (for sort function) */
int cmp_trans_y(const void *t1, const void* t2);
/* static int cmp_trans_alpha(const void *t1, const void* t2); */

/* compares two double values (for sort function)*/
int cmp_double(const void *t1, const void* t2);

/* calculates the median of an array of transforms, 
 * considering only x and y
 */
Transform median_xy_transform(const Transform* transforms, int len);
/* median of a double array */
double median(double* ds, int len);
/* mean of a double array */
double mean(const double* ds, int len);
/* mean with cutted upper and lower pentile 
 * (min and max are optionally returned)
 */
double cleanmean(double* ds, int len, double* minimum, double* maximum);
/* calculates the cleaned mean of an array of transforms,
 * considering only x and y
 */
Transform cleanmean_xy_transform(const Transform* transforms, int len);

/* calculates the cleaned (cutting of x-th percentil) 
 * maximum and minimum of an array of transforms,
 * considerung only x and y
 */
void cleanmaxmin_xy_transform(const Transform* transforms, int len,
                              int percentil, 
                              Transform* min, Transform* max);

/* helper functions */

/* optimized round function */
inline static int myround(float x) {
    if(x>0)
        return x + 0.5;
    else
        return x - 0.5;
}


/* optimized floor function 
   This does not give the correct value for negative integer values like -1.0. In this case
   it will produce -2.0.
*/
inline static int myfloor(float x) {
    if(x<0)
        return x - 1;
    else
        return x;
}


#endif

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
