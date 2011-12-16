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

#include "transform.h"

#include <math.h>
#include <stdio.h>
#include "tlist.h"
#include <framework/mlt_types.h>

#define DEFAULT_TRANS_FILE_NAME     "transforms.dat"

#define PIXEL(img, x, y, w, h, def) ((x) < 0 || (y) < 0) ? def       \
    : (((x) >=w || (y) >= h) ? def : img[(x) + (y) * w])
#define PIX(img, x, y, w, h) (img[(x) + (y) * w])
#define PIXN(img, x, y, w, h,N,channel) (img[((x) + (y) * w)*N+channel])
// gives Pixel in N-channel image. channel in {0..N-1}
#define PIXELN(img, x, y, w, h, N,channel , def) ((x) < 0 || (y) < 0) ? def  \
    : (((x) >=w || (y) >= h) ? def : img[((x) + (y) * w)*N + channel])

typedef struct {
    int framesize_src;  // size of frame buffer in bytes (src)
    int framesize_dest; // size of frame buffer in bytes (dest)
    unsigned char* src;  // copy of the current frame buffer
    unsigned char* dest; // pointer to the current frame buffer (to overwrite)

	int pixelformat;
    int width_src, height_src;
    int width_dest, height_dest;
    Transform* trans;    // array of transformations
    int current_trans;   // index to current transformation
    int trans_len;       // length of trans array
    short warned_transform_end; // whether we warned that there is no transform left

    /* Options */
    int maxshift;        // maximum number of pixels we will shift
    double maxangle;     // maximum angle in rad

    /* whether to consider transforms as relative (to previous frame)
     * or absolute transforms
     */
    int relative;
    /* number of frames (forward and backward)
     * to use for smoothing transforms */
    int smoothing;
    int crop;       // 1: black bg, 0: keep border from last frame(s)
    int invert;     // 1: invert transforms, 0: nothing
    /* constants */
    /* threshhold below which no rotation is performed */
    double rotation_threshhold;
    double zoom;      // percentage to zoom: 0->no zooming 10:zoom in 10%
    int optzoom;      // 1: determine optimal zoom, 0: nothing
    int interpoltype; // type of interpolation: 0->Zero,1->Lin,2->BiLin,3->Sqr
    double sharpen;   // amount of sharpening

    char conf_str[1024];
} TransformData;

/* forward declarations, please look below for documentation*/
void interpolateBiLinBorder(unsigned char *rv, float x, float y,
                            unsigned char* img, int w, int h, unsigned char def,unsigned char N, unsigned char channel);
void interpolateBiCub(unsigned char *rv, float x, float y,
                      unsigned char* img, int width, int height, unsigned char def,unsigned char N, unsigned char channel);
void interpolateSqr(unsigned char *rv, float x, float y,
                    unsigned char* img, int w, int h, unsigned char def,unsigned char N, unsigned char channel);
void interpolateBiLin(unsigned char *rv, float x, float y,
                      unsigned char* img, int w, int h, unsigned char def,unsigned char N, unsigned char channel);
void interpolateLin(unsigned char *rv, float x, float y,
                      unsigned char* img, int w, int h, unsigned char def,unsigned char N, unsigned char channel);
void interpolateZero(unsigned char *rv, float x, float y,
                     unsigned char* img, int w, int h, unsigned char def,unsigned char N, unsigned char channel);
void interpolateN(unsigned char *rv, float x, float y,
                  unsigned char* img, int width, int height,
                  unsigned char N, unsigned char channel, unsigned char def);
int transformRGB(TransformData* td);
int transformYUV(TransformData* td);
int read_input_file(TransformData* td,tlist* list);
int preprocess_transforms(TransformData* td);


/**
 * interpolate: general interpolation function pointer for one channel image data
 *
 * Parameters:
 *             rv: destination pixel (call by reference)
 *            x,y: the source coordinates in the image img. Note this
 *                 are real-value coordinates, that's why we interpolate
 *            img: source image
 *   width,height: dimension of image
 *            def: default value if coordinates are out of range
 * Return value:  None
 */
/*void (*interpolate)(unsigned char *rv, float x, float y,
                    unsigned char* img, int width, int height,
                    unsigned char def) = 0;
*/
/** interpolateBiLinBorder: bi-linear interpolation function that also works at the border.
    This is used by many other interpolation methods at and outsize the border, see interpolate */
int transform_configure(TransformData *self,int width,int height, mlt_image_format pixelformat, unsigned char* image,Transform* tx,int trans_len) ;

int transform_filter_video(TransformData *self,
		                                  unsigned char *frame,mlt_image_format pixelformat);
