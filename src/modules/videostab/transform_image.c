/*
 *  filter_transform.c
 *
 *  Copyright (C) Georg Martius - June 2007
 *   georg dot martius at web dot de
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
 * Typical call:
 * transcode -J transform -i inp.mpeg -y xdiv,tcaud inp_stab.avi
 */
#include "transform_image.h"
#include <string.h>
#include <stdlib.h>
#include <framework/mlt_log.h>

#define TC_MAX(a, b)		(((a) > (b)) ?(a) :(b))
#define TC_MIN(a, b)		(((a) < (b)) ?(a) :(b))
/* clamp x between a and b */
#define TC_CLAMP(x, a, b)	TC_MIN(TC_MAX((a), (x)), (b))


static const char* interpoltypes[5] = {"No (0)", "Linear (1)", "Bi-Linear (2)",
                                       "Quadratic (3)", "Bi-Cubic (4)"};

void (*interpolate)(unsigned char *rv, float x, float y,
                    unsigned char* img, int width, int height,
                    unsigned char def,unsigned char N, unsigned char channel) = 0;

/** interpolateBiLinBorder: bi-linear interpolation function that also works at the border.
    This is used by many other interpolation methods at and outsize the border, see interpolate */
void interpolateBiLinBorder(unsigned char *rv, float x, float y,
                            unsigned char* img, int width, int height,
                            unsigned char def,unsigned char N, unsigned char channel)
{
    int x_f = myfloor(x);
    int x_c = x_f+1;
    int y_f = myfloor(y);
    int y_c = y_f+1;
    short v1 = PIXELN(img, x_c, y_c, width, height, N, channel, def);
    short v2 = PIXELN(img, x_c, y_f, width, height, N, channel, def);
    short v3 = PIXELN(img, x_f, y_c, width, height, N, channel, def);
    short v4 = PIXELN(img, x_f, y_f, width, height, N, channel, def);
    float s  = (v1*(x - x_f)+v3*(x_c - x))*(y - y_f) +
        (v2*(x - x_f) + v4*(x_c - x))*(y_c - y);
    *rv = (unsigned char)s;
}

/* taken from http://en.wikipedia.org/wiki/Bicubic_interpolation for alpha=-0.5
   in matrix notation:
   a0-a3 are the neigthboring points where the target point is between a1 and a2
   t is the point of interpolation (position between a1 and a2) value between 0 and 1
                 | 0, 2, 0, 0 |  |a0|
                 |-1, 0, 1, 0 |  |a1|
   (1,t,t^2,t^3) | 2,-5, 4,-1 |  |a2|
                 |-1, 3,-3, 1 |  |a3|
*/
static short bicub_kernel(float t, short a0, short a1, short a2, short a3){
  return (2*a1 + t*((-a0+a2) + t*((2*a0-5*a1+4*a2-a3) + t*(-a0+3*a1-3*a2+a3) )) ) / 2;
}

/** interpolateBiCub: bi-cubic interpolation function using 4x4 pixel, see interpolate */
void interpolateBiCub(unsigned char *rv, float x, float y,
                      unsigned char* img, int width, int height, unsigned char def,unsigned char N, unsigned char channel)
{
    // do a simple linear interpolation at the border
    if (x < 1 || x >= width-2 || y < 1 || y >= height - 2) {
        interpolateBiLinBorder(rv, x,y,img,width,height,def,N,channel);
    } else {
        int x_f = myfloor(x);
        int y_f = myfloor(y);
        float tx = x-x_f;
        short v1 = bicub_kernel(tx,
                                PIXN(img, x_f-1, y_f-1, width, height, N, channel),
                                PIXN(img, x_f,   y_f-1, width, height, N, channel),
                                PIXN(img, x_f+1, y_f-1, width, height, N, channel),
                                PIXN(img, x_f+2, y_f-1, width, height, N, channel));
        short v2 = bicub_kernel(tx,
                                PIXN(img, x_f-1, y_f, width, height, N, channel),
                                PIXN(img, x_f,   y_f, width, height, N, channel),
                                PIXN(img, x_f+1, y_f, width, height, N, channel),
                                PIXN(img, x_f+2, y_f, width, height, N, channel));
        short v3 = bicub_kernel(tx,
                                PIXN(img, x_f-1, y_f+1, width, height, N, channel),
                                PIXN(img, x_f,   y_f+1, width, height, N, channel),
                                PIXN(img, x_f+1, y_f+1, width, height, N, channel),
                                PIXN(img, x_f+2, y_f+1, width, height, N, channel));
        short v4 = bicub_kernel(tx,
                                PIXN(img, x_f-1, y_f+2, width, height, N, channel),
                                PIXN(img, x_f,   y_f+2, width, height, N, channel),
                                PIXN(img, x_f+1, y_f+2, width, height, N, channel),
                                PIXN(img, x_f+2, y_f+2, width, height, N, channel));
        *rv = (unsigned char)bicub_kernel(y-y_f, v1, v2, v3, v4);
    }
}

/** interpolateSqr: bi-quatratic interpolation function, see interpolate */
void interpolateSqr(unsigned char *rv, float x, float y,
                    unsigned char* img, int width, int height, unsigned char def,unsigned char N, unsigned char channel)
{
    if (x < 0 || x >= width-1 || y < 0 || y >= height-1) {
        interpolateBiLinBorder(rv, x, y, img, width, height, def,N,channel);
    } else {
        int x_f = myfloor(x);
        int x_c = x_f+1;
        int y_f = myfloor(y);
        int y_c = y_f+1;
        short v1 = PIXN(img, x_c, y_c, width, height, N, channel);
        short v2 = PIXN(img, x_c, y_f, width, height, N, channel);
        short v3 = PIXN(img, x_f, y_c, width, height, N, channel);
        short v4 = PIXN(img, x_f, y_f, width, height, N, channel);
        float f1 = 1 - sqrt((x_c - x) * (y_c - y));
        float f2 = 1 - sqrt((x_c - x) * (y - y_f));
        float f3 = 1 - sqrt((x - x_f) * (y_c - y));
        float f4 = 1 - sqrt((x - x_f) * (y - y_f));
        float s  = (v1*f1 + v2*f2 + v3*f3+ v4*f4)/(f1 + f2 + f3 + f4);
        *rv = (unsigned char)s;
    }
}

/** interpolateBiLin: bi-linear interpolation function, see interpolate */
void interpolateBiLin(unsigned char *rv, float x, float y,
                      unsigned char* img, int width, int height,
                      unsigned char def,unsigned char N, unsigned char channel)
{
    if (x < 0 || x >= width-1 || y < 0 || y >= height - 1) {
        interpolateBiLinBorder(rv, x, y, img, width, height, def,N,channel);
    } else {
        int x_f = myfloor(x);
        int x_c = x_f+1;
        int y_f = myfloor(y);
        int y_c = y_f+1;
        short v1 = PIXN(img, x_c, y_c, width, height, N, channel);
        short v2 = PIXN(img, x_c, y_f, width, height, N, channel);
        short v3 = PIXN(img, x_f, y_c, width, height, N, channel);
        short v4 = PIXN(img, x_f, y_f, width, height, N, channel);
        float s  = (v1*(x - x_f)+v3*(x_c - x))*(y - y_f) +
            (v2*(x - x_f) + v4*(x_c - x))*(y_c - y);
        *rv = (unsigned char)s;
    }
}


/** interpolateLin: linear (only x) interpolation function, see interpolate */
void interpolateLin(unsigned char *rv, float x, float y,
                    unsigned char* img, int width, int height,
                    unsigned char def,unsigned char N, unsigned char channel)
{
    int x_f = myfloor(x);
    int x_c = x_f+1;
    int y_n = myround(y);
    float v1 = PIXELN(img, x_c, y_n, width, height, N, channel,def);
    float v2 = PIXELN(img, x_f, y_n, width, height, N, channel,def);
    float s  = v1*(x - x_f) + v2*(x_c - x);
    *rv = (unsigned char)s;
}

/** interpolateZero: nearest neighbor interpolation function, see interpolate */
void interpolateZero(unsigned char *rv, float x, float y,
                   unsigned char* img, int width, int height, unsigned char def,unsigned char N, unsigned char channel)
{
    int x_n = myround(x);
    int y_n = myround(y);
    *rv = (unsigned char) PIXELN(img, x_n, y_n, width, height, N,channel,def);
}


/**
 * interpolateN: Bi-linear interpolation function for N channel image.
 *
 * Parameters:
 *             rv: destination pixel (call by reference)
 *            x,y: the source coordinates in the image img. Note this
 *                 are real-value coordinates, that's why we interpolate
 *            img: source image
 *   width,height: dimension of image
 *              N: number of channels
 *        channel: channel number (0..N-1)
 *            def: default value if coordinates are out of range
 * Return value:  None
 */
void interpolateN(unsigned char *rv, float x, float y,
                  unsigned char* img, int width, int height,
                  unsigned char N, unsigned char channel,
                  unsigned char def)
{
    if (x < - 1 || x > width || y < -1 || y > height) {
        *rv = def;
    } else {
        int x_f = myfloor(x);
        int x_c = x_f+1;
        int y_f = myfloor(y);
        int y_c = y_f+1;
        short v1 = PIXELN(img, x_c, y_c, width, height, N, channel, def);
        short v2 = PIXELN(img, x_c, y_f, width, height, N, channel, def);
        short v3 = PIXELN(img, x_f, y_c, width, height, N, channel, def);
        short v4 = PIXELN(img, x_f, y_f, width, height, N, channel, def);
        float s  = (v1*(x - x_f)+v3*(x_c - x))*(y - y_f) +
            (v2*(x - x_f) + v4*(x_c - x))*(y_c - y);
        *rv = (unsigned char)s;
    }
}


/**
 * transformRGB: applies current transformation to frame
 * Parameters:
 *         td: private data structure of this filter
 * Return value:
 *         0 for failture, 1 for success
 * Preconditions:
 *  The frame must be in RGB format
 */
int transformRGB(TransformData* td)
{
    Transform t;
    int x = 0, y = 0, z = 0;
    unsigned char *D_1, *D_2;
    t = td->trans[td->current_trans];

    D_1  = td->src;
    D_2  = td->dest;
		float zm = 1.0-t.zoom/100;
		float zcos_a = zm*cos(-t.alpha); // scaled cos
    float zsin_a = zm*sin(-t.alpha); // scaled sin
    float c_s_x = td->width_src/2.0;
    float c_s_y = td->height_src/2.0;
    float c_d_x = td->width_dest/2.0;
    float c_d_y = td->height_dest/2.0;

    /* for each pixel in the destination image we calc the source
     * coordinate and make an interpolation:
     *      p_d = c_d + M(p_s - c_s) + t
     * where p are the points, c the center coordinate,
     *  _s source and _d destination,
     *  t the translation, and M the rotation matrix
     *      p_s = M^{-1}(p_d - c_d - t) + c_s
     */
    /* All 3 channels */
    if (fabs(t.alpha) > td->rotation_threshhold || t.zoom != 0) {
        for (x = 0; x < td->width_dest; x++) {
            for (y = 0; y < td->height_dest; y++) {
                float x_d1 = (x - c_d_x);
                float y_d1 = (y - c_d_y);
                float x_s  =  zcos_a * x_d1
                    + zsin_a * y_d1 + c_s_x -t.x;
                float y_s  = -zsin_a * x_d1
                    + zcos_a * y_d1 + c_s_y -t.y;
                for (z = 0; z < 3; z++) { // iterate over colors
                    unsigned char* dest = &D_2[(x + y * td->width_dest)*3+z];
                    interpolate(dest, x_s, y_s, D_1,
                                 td->width_src, td->height_src,
                                 td->crop ? 16 : *dest,3,z);
                }
            }
        }
     }else {
        /* no rotation, just translation
         *(also no interpolation, since no size change (so far)
         */
        int round_tx = myround(t.x);
        int round_ty = myround(t.y);
        for (x = 0; x < td->width_dest; x++) {
            for (y = 0; y < td->height_dest; y++) {
                for (z = 0; z < 3; z++) { // iterate over colors
                    short p = PIXELN(D_1, x - round_tx, y - round_ty,
                                     td->width_src, td->height_src, 3, z, -1);
                    if (p == -1) {
                        if (td->crop == 1)
                            D_2[(x + y * td->width_dest)*3+z] = 16;
                    } else {
                        D_2[(x + y * td->width_dest)*3+z] = (unsigned char)p;
                    }
                }
            }
        }
    }
    return 1;
}

/**
 * transformYUV: applies current transformation to frame
 *
 * Parameters:
 *         td: private data structure of this filter
 * Return value:
 *         0 for failture, 1 for success
 * Preconditions:
 *  The frame must be in YUV format
 */
int transformYUV(TransformData* td)
{
    Transform t;
    int x = 0, y = 0;
    unsigned char *Y_1, *Y_2, *Cb_1, *Cb_2, *Cr_1, *Cr_2;
    t = td->trans[td->current_trans];

    Y_1  = td->src;
    Y_2  = td->dest;
    Cb_1 = td->src + td->width_src * td->height_src;
    Cb_2 = td->dest + td->width_dest * td->height_dest;
    Cr_1 = td->src + 5*td->width_src * td->height_src/4;
    Cr_2 = td->dest + 5*td->width_dest * td->height_dest/4;
    float c_s_x = td->width_src/2.0;
    float c_s_y = td->height_src/2.0;
    float c_d_x = td->width_dest/2.0;
    float c_d_y = td->height_dest/2.0;

    float z = 1.0-t.zoom/100;
    float zcos_a = z*cos(-t.alpha); // scaled cos
    float zsin_a = z*sin(-t.alpha); // scaled sin

    /* for each pixel in the destination image we calc the source
     * coordinate and make an interpolation:
     *      p_d = c_d + M(p_s - c_s) + t
     * where p are the points, c the center coordinate,
     *  _s source and _d destination,
     *  t the translation, and M the rotation and scaling matrix
     *      p_s = M^{-1}(p_d - c_d - t) + c_s
     */
    /* Luminance channel */
    if (fabs(t.alpha) > td->rotation_threshhold || t.zoom != 0) {
        for (x = 0; x < td->width_dest; x++) {
            for (y = 0; y < td->height_dest; y++) {
                float x_d1 = (x - c_d_x);
                float y_d1 = (y - c_d_y);
                float x_s  =  zcos_a * x_d1
                    + zsin_a * y_d1 + c_s_x -t.x;
                float y_s  = -zsin_a * x_d1
                    + zcos_a * y_d1 + c_s_y -t.y;
                unsigned char* dest = &Y_2[x + y * td->width_dest];
                interpolate(dest, x_s, y_s, Y_1,
                            td->width_src, td->height_src,
                            td->crop ? 16 : *dest,1,0);
            }
        }
     }else {
        /* no rotation, no zooming, just translation
         *(also no interpolation, since no size change)
         */
        int round_tx = myround(t.x);
        int round_ty = myround(t.y);
        for (x = 0; x < td->width_dest; x++) {
            for (y = 0; y < td->height_dest; y++) {
                short p = PIXEL(Y_1, x - round_tx, y - round_ty,
                                td->width_src, td->height_src, -1);
                if (p == -1) {
                    if (td->crop == 1)
                        Y_2[x + y * td->width_dest] = 16;
                } else {
                    Y_2[x + y * td->width_dest] = (unsigned char)p;
                }
            }
        }
    }

    /* Color channels */
    int ws2 = td->width_src/2;
    int wd2 = td->width_dest/2;
    int hs2 = td->height_src/2;
    int hd2 = td->height_dest/2;
    if (fabs(t.alpha) > td->rotation_threshhold || t.zoom != 0) {
        for (x = 0; x < wd2; x++) {
            for (y = 0; y < hd2; y++) {
                float x_d1 = x - (c_d_x)/2;
                float y_d1 = y - (c_d_y)/2;
                float x_s  =  zcos_a * x_d1
                    + zsin_a * y_d1 + (c_s_x -t.x)/2;
                float y_s  = -zsin_a * x_d1
                    + zcos_a * y_d1 + (c_s_y -t.y)/2;
                unsigned char* dest = &Cr_2[x + y * wd2];
                interpolate(dest, x_s, y_s, Cr_1, ws2, hs2,
                            td->crop ? 128 : *dest,1,0);
                dest = &Cb_2[x + y * wd2];
                interpolate(dest, x_s, y_s, Cb_1, ws2, hs2,
                            td->crop ? 128 : *dest,1,0);
            }
        }
    } else { // no rotation, no zoom, no interpolation, just translation
        int round_tx2 = myround(t.x/2.0);
        int round_ty2 = myround(t.y/2.0);
        for (x = 0; x < wd2; x++) {
            for (y = 0; y < hd2; y++) {
                short cr = PIXEL(Cr_1, x - round_tx2, y - round_ty2,
                                 wd2, hd2, -1);
                short cb = PIXEL(Cb_1, x - round_tx2, y - round_ty2,
                                 wd2, hd2, -1);
                if (cr == -1) {
                    if (td->crop==1) {
                        Cr_2[x + y * wd2] = 128;
                        Cb_2[x + y * wd2] = 128;
                    }
                } else {
                    Cr_2[x + y * wd2] = (unsigned char)cr;
                    Cb_2[x + y * wd2] = (unsigned char)cb;
                }
            }
        }
    }
    return 1;
}


/**
 * preprocess_transforms: does smoothing, relative to absolute conversion,
 *  and cropping of too large transforms.
 *  This is actually the core algorithm for canceling the jiggle in the
 *  movie. We perform a low-pass filter in terms of transformation size.
 *  This enables still camera movement, but in a smooth fasion.
 *
 * Parameters:
 *            td: tranform private data structure
 * Return value:
 *     1 for success and 0 for failture
 * Preconditions:
 *     None
 * Side effects:
 *     td->trans will be modified
 */
int preprocess_transforms(TransformData* td)
{
    Transform* ts = td->trans;
    int i;

    if (td->trans_len < 1)
        return 0;
    if (0) {
        mlt_log_debug(NULL,"Preprocess transforms:");
    }
    if (td->smoothing>0) {
        /* smoothing */
        Transform* ts2 = malloc(sizeof(Transform) * td->trans_len);
        memcpy(ts2, ts, sizeof(Transform) * td->trans_len);

        /*  we will do a sliding average with minimal update
         *   \hat x_{n/2} = x_1+x_2 + .. + x_n
         *   \hat x_{n/2+1} = x_2+x_3 + .. + x_{n+1} = x_{n/2} - x_1 + x_{n+1}
         *   avg = \hat x / n
         */
        int s = td->smoothing * 2 + 1;
        Transform null = null_transform();
        /* avg is the average over [-smoothing, smoothing] transforms
           around the current point */
        Transform avg;
        /* avg2 is a sliding average over the filtered signal! (only to past)
         *  with smoothing * 10 horizont to kill offsets */
        Transform avg2 = null_transform();
        double tau = 1.0/(3 * s);
        /* initialise sliding sum with hypothetic sum centered around
         * -1st element. We have two choices:
         * a) assume the camera is not moving at the beginning
         * b) assume that the camera moves and we use the first transforms
         */
        Transform s_sum = null;
        for (i = 0; i < td->smoothing; i++){
            s_sum = add_transforms(&s_sum, i < td->trans_len ? &ts2[i]:&null);
        }
        mult_transform(&s_sum, 2); // choice b (comment out for choice a)

        for (i = 0; i < td->trans_len; i++) {
            Transform* old = ((i - td->smoothing - 1) < 0)
                ? &null : &ts2[(i - td->smoothing - 1)];
            Transform* new = ((i + td->smoothing) >= td->trans_len)
                ? &null : &ts2[(i + td->smoothing)];
            s_sum = sub_transforms(&s_sum, old);
            s_sum = add_transforms(&s_sum, new);

            avg = mult_transform(&s_sum, 1.0/s);

            /* lowpass filter:
             * meaning high frequency must be transformed away
             */
            ts[i] = sub_transforms(&ts2[i], &avg);
            /* kill accumulating offset in the filtered signal*/
            avg2 = add_transforms_(mult_transform(&avg2, 1 - tau),
                                   mult_transform(&ts[i], tau));
            ts[i] = sub_transforms(&ts[i], &avg2);

            if (0 /*verbose*/ ) {
                mlt_log_warning(NULL,"s_sum: %5lf %5lf %5lf, ts: %5lf, %5lf, %5lf\n",
                           s_sum.x, s_sum.y, s_sum.alpha,
                           ts[i].x, ts[i].y, ts[i].alpha);
                mlt_log_warning(NULL,
                           "  avg: %5lf, %5lf, %5lf avg2: %5lf, %5lf, %5lf",
                           avg.x, avg.y, avg.alpha,
                           avg2.x, avg2.y, avg2.alpha);
            }
        }
        free(ts2);
    }


    /*  invert? */
    if (td->invert) {
        for (i = 0; i < td->trans_len; i++) {
            ts[i] = mult_transform(&ts[i], -1);
        }
    }

    /* relative to absolute */
    if (td->relative) {
        Transform t = ts[0];
        for (i = 1; i < td->trans_len; i++) {
            if (0/*verbose*/ ) {
                mlt_log_warning(NULL, "shift: %5lf   %5lf   %lf \n",
                           t.x, t.y, t.alpha *180/M_PI);
            }
            ts[i] = add_transforms(&ts[i], &t);
            t = ts[i];
        }
    }
    /* crop at maximal shift */
    if (td->maxshift != -1)
        for (i = 0; i < td->trans_len; i++) {
            ts[i].x     = TC_CLAMP(ts[i].x, -td->maxshift, td->maxshift);
            ts[i].y     = TC_CLAMP(ts[i].y, -td->maxshift, td->maxshift);
        }
    if (td->maxangle != - 1.0)
        for (i = 0; i < td->trans_len; i++)
            ts[i].alpha = TC_CLAMP(ts[i].alpha, -td->maxangle, td->maxangle);

    /* Calc optimal zoom
     *  cheap algo is to only consider transformations
     *  uses cleaned max and min
     */
    if (td->optzoom != 0 && td->trans_len > 1){
        Transform min_t, max_t;
        cleanmaxmin_xy_transform(ts, td->trans_len, 10, &min_t, &max_t);
        // the zoom value only for x
        double zx = 2*TC_MAX(max_t.x,fabs(min_t.x))/td->width_src;
        // the zoom value only for y
        double zy = 2*TC_MAX(max_t.y,fabs(min_t.y))/td->height_src;
        td->zoom += 100* TC_MAX(zx,zy); // use maximum
        mlt_log_debug(NULL,"Final zoom: %lf\n", td->zoom);
    }

    /* apply global zoom */
    if (td->zoom != 0){
        for (i = 0; i < td->trans_len; i++)
            ts[i].zoom += td->zoom;
    }

    return 1;
}

/**
 * transform_init:  Initialize this instance of the module.  See
 * tcmodule-data.h for function details.
 */

#if 0

static int transform_init(TCModuleInstance *self, uint32_t features)
{

    TransformData* td = NULL;
    TC_MODULE_SELF_CHECK(self, "init");
    TC_MODULE_INIT_CHECK(self, MOD_FEATURES, features);

    td = tc_zalloc(sizeof(TransformData));
    if (td == NULL) {
        tc_log_error(MOD_NAME, "init: out of memory!");
        return TC_ERROR;
    }
    self->userdata = td;
    if (verbose) {
        tc_log_info(MOD_NAME, "%s %s", MOD_VERSION, MOD_CAP);
    }

    return T;
}
#endif

/**
 * transform_configure:  Configure this instance of the module.  See
 * tcmodule-data.h for function details.
 */
int transform_configure(TransformData *self,int width,int height, mlt_image_format  pixelformat, unsigned char* image ,Transform* tx,int trans_len)
{
    TransformData *td = self;

    /**** Initialise private data structure */

    /* td->framesize = td->vob->im_v_width *
     *  MAX_PLANES * sizeof(char) * 2 * td->vob->im_v_height * 2;
     */
	// rgb24 = w*h*3 , yuv420p = w* h* 3/2
    td->framesize_src = width*height*(pixelformat==mlt_image_rgb24 ? 3 : (3.0/2.0));
    td->src = malloc(td->framesize_src); /* FIXME */
    if (td->src == NULL) {
        mlt_log_error(NULL,"tc_malloc failed\n");
        return -1;
    }

    td->width_src  = width;
    td->height_src = height;

    /* Todo: in case we can scale the images, calc new size later */
    td->width_dest  = width;
    td->height_dest = height;
    td->framesize_dest = td->framesize_src;
    td->dest = 0;

    td->trans = tx;
    td->trans_len = trans_len;
    td->current_trans = 0;
    td->warned_transform_end = 0;

    /* Options */
    // set from  filter td->maxshift = -1;
    // set from  filter td->maxangle = -1;


    // set from  filter td->crop = 0;
    // set from  filter td->relative = 1;
    // set from  filter td->invert = 0;
    // set from  filter td->smoothing = 10;

    td->rotation_threshhold = 0.25/(180/M_PI);

    // set from  filter td->zoom    = 0;
    // set from  filter td->optzoom = 1;
    // set from  filter td->interpoltype = 2; // bi-linear
    // set from  filter td->sharpen = 0.8;

    td->interpoltype = TC_MIN(td->interpoltype,4);
    if (1) {
        mlt_log_debug(NULL, "Image Transformation/Stabilization Settings:\n");
        mlt_log_debug(NULL, "    smoothing = %d\n", td->smoothing);
        mlt_log_debug(NULL, "    maxshift  = %d\n", td->maxshift);
        mlt_log_debug(NULL, "    maxangle  = %f\n", td->maxangle);
        mlt_log_debug(NULL, "    crop      = %s\n",
                        td->crop ? "Black" : "Keep");
        mlt_log_debug(NULL, "    relative  = %s\n",
                    td->relative ? "True": "False");
        mlt_log_debug(NULL, "    invert    = %s\n",
                    td->invert ? "True" : "False");
        mlt_log_debug(NULL, "    zoom      = %f\n", td->zoom);
        mlt_log_debug(NULL, "    optzoom   = %s\n",
                    td->optzoom ? "On" : "Off");
        mlt_log_debug(NULL, "    interpol  = %s\n",
                    interpoltypes[td->interpoltype]);
        mlt_log_debug(NULL, "    sharpen   = %f\n", td->sharpen);
    }

    if (td->maxshift > td->width_dest/2
        ) td->maxshift = td->width_dest/2;
    if (td->maxshift > td->height_dest/2)
        td->maxshift = td->height_dest/2;

    if (!preprocess_transforms(td)) {
        mlt_log_error(NULL,"error while preprocessing transforms!");
        return -1;
    }

    switch(td->interpoltype){
      case 0:  interpolate = &interpolateZero; break;
      case 1:  interpolate = &interpolateLin; break;
      case 2:  interpolate = &interpolateBiLin; break;
      case 3:  interpolate = &interpolateSqr; break;
      case 4:  interpolate = &interpolateBiCub; break;
      default: interpolate = &interpolateBiLin;
    }

    return 0;
}


/**
 * transform_filter_video: performs the transformation of frames
 * See tcmodule-data.h for function details.
 */
int transform_filter_video(TransformData *self,
                                  unsigned char *frame,mlt_image_format pixelformat)
{
    TransformData *td = self;


    td->dest = frame;
    memcpy(td->src, frame, td->framesize_src);
    if (td->current_trans >= td->trans_len) {
        td->current_trans = td->trans_len-1;
        if(!td->warned_transform_end)
            mlt_log_warning(NULL,"not enough transforms found, use last transformation!\n");
        td->warned_transform_end = 1;
    }

    if (pixelformat == mlt_image_rgb24 ) {
        transformRGB(td);
    } else if (pixelformat == mlt_image_yuv420p) {
        transformYUV(td);
    } else {
        mlt_log_error(NULL,"unsupported Codec: %i\n", pixelformat);
        return 1;
    }
    td->current_trans++;
    return 0;
}


