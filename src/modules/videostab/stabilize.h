/*
 *  filter_stabilize.c
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
 */

/* Typical call:
 *  transcode -V -J stabilize=shakiness=5:show=1,preview 
 *         -i inp.mpeg -y null,null -o dummy
 *  all parameters are optional
*/

#define MOD_NAME    "filter_stabilize.so"
#define MOD_VERSION "v0.75 (2010-04-07)"
#define MOD_CAP     "extracts relative transformations of \n\
    subsequent frames (used for stabilization together with the\n\
    transform filter in a second pass)"
#define MOD_AUTHOR  "Georg Martius"

/* Ideas:
 - Try OpenCL/Cuda, this should work great
 - use smoothing on the frames and then use gradient decent!
 - stepsize could be adapted (maybe to check only one field with large
   stepsize and use the maximally required for the other fields
*/

#define MOD_FEATURES \
    TC_MODULE_FEATURE_FILTER|TC_MODULE_FEATURE_VIDEO
#define MOD_FLAGS  \
    TC_MODULE_FLAG_RECONFIGURABLE | TC_MODULE_FLAG_DELAY
  

#include "transform.h"

#include <math.h>
#include <libgen.h>
#include <stdio.h>
#include "tlist.h"
#include <framework/mlt_types.h>

/* if defined we are very verbose and generate files to analyse
 * this is really just for debugging and development */
// #define STABVERBOSE


typedef struct _field {
    int x;     // middle position x
    int y;     // middle position y
    int size;  // size of field
} Field;

// structure that contains the contrast and the index of a field
typedef struct _contrast_idx {
    double contrast;
    int index;
} contrast_idx;

/* private date structure of this filter*/
typedef struct _stab_data {
    int framesize;  // size of frame buffer in bytes (prev)
    unsigned char* curr; // current frame buffer (only pointer)
    unsigned char* currcopy; // copy of the current frame needed for drawing
    unsigned char* prev; // frame buffer for last frame (copied)
    unsigned char* grayimage; // frame buffer for last frame (copied)
    short hasSeenOneFrame; // true if we have a valid previous frame

    int width, height;
	mlt_image_format pixelformat;

    /* list of transforms*/
    //TCList* transs;
    tlist* transs;

    Field* fields;


    /* Options */
    /* maximum number of pixels we expect the shift of subsequent frames */
    int maxshift; 
    int stepsize; // stepsize of field transformation detection
    int allowmax; // 1 if maximal shift is allowed
    int algo;     // algorithm to use
    int field_num;  // number of measurement fields
    int maxfields;  // maximum number of fields used (selected by contrast)
    int field_size; // size    = min(sd->width, sd->height)/10;
    int field_rows; // number of rows
    /* if 1 and 2 then the fields and transforms are shown in the frames */
    int show; 
    /* measurement fields with lower contrast are discarded */
    double contrast_threshold;            
    /* maximal difference in angles of fields */
    double maxanglevariation;
    /* meta parameter for maxshift and fieldsize between 1 and 10 */
    int shakiness;   
    int accuracy;   // meta parameter for number of fields between 1 and 10
  
    int t;

    char conf_str[1024];
} StabData;

/* type for a function that calculates the transformation of a certain field 
 */
typedef Transform (*calcFieldTransFunc)(StabData*, const Field*, int);

/* type for a function that calculates the contrast of a certain field 
 */
typedef double (*contrastSubImgFunc)(StabData* sd, const Field* field);

static const char stabilize_help[] = ""
    "Overview:\n"
    "    Generates a file with relative transform information\n"
    "     (translation, rotation) about subsequent frames."
    " See also transform.\n" 
    "Options\n"
    "    'result'      path to the file used to write the transforms\n"
    "                  (def:inputfile.stab)\n"
    "    'shakiness'   how shaky is the video and how quick is the camera?\n"
    "                  1: little (fast) 10: very strong/quick (slow) (def: 4)\n"
    "    'accuracy'    accuracy of detection process (>=shakiness)\n"
    "                  1: low (fast) 15: high (slow) (def: 4)\n"
    "    'stepsize'    stepsize of search process, region around minimum \n"
    "                  is scanned with 1 pixel resolution (def: 6)\n"
    "    'algo'        0: brute force (translation only);\n"
    "                  1: small measurement fields (def)\n"
    "    'mincontrast' below this contrast a field is discarded (0-1) (def: 0.3)\n"
    "    'show'        0: draw nothing (def); 1,2: show fields and transforms\n"
    "                  in the resulting frames. Consider the 'preview' filter\n"
    "    'help'        print this help message\n";

int initFields(StabData* sd);
double compareImg(unsigned char* I1, unsigned char* I2, 
		  int width, int height,  int bytesPerPixel, int d_x, int d_y);
double compareSubImg(unsigned char* const I1, unsigned char* const I2, 
		     const Field* field, 
		     int width, int height, int bytesPerPixel,int d_x,int d_y);
double contrastSubImgYUV(StabData* sd, const Field* field);
double contrastSubImgRGB(StabData* sd, const Field* field);
double contrastSubImg(unsigned char* const I, const Field* field, 
                      int width, int height, int bytesPerPixel);
int cmp_contrast_idx(const void *ci1, const void* ci2);
tlist* selectfields(StabData* sd, contrastSubImgFunc contrastfunc);

Transform calcShiftRGBSimple(StabData* sd);
Transform calcShiftYUVSimple(StabData* sd);
double calcAngle(StabData* sd, Field* field, Transform* t,
                 int center_x, int center_y);
Transform calcFieldTransYUV(StabData* sd, const Field* field, 
                            int fieldnum);
Transform calcFieldTransRGB(StabData* sd, const Field* field, 
                            int fieldnum);
Transform calcTransFields(StabData* sd, calcFieldTransFunc fieldfunc,
                          contrastSubImgFunc contrastfunc);


void drawFieldScanArea(StabData* sd, const Field* field, const Transform* t);
void drawField(StabData* sd, const Field* field, const Transform* t);
void drawFieldTrans(StabData* sd, const Field* field, const Transform* t);
void drawBox(unsigned char* I, int width, int height, int bytesPerPixel, 
             int x, int y, int sizex, int sizey, unsigned char color);
void addTrans(StabData* sd, Transform sl);

int stabilize_configure(StabData* instance);
int stabilize_stop(StabData* instance);

int stabilize_filter_video(StabData* instance, unsigned char *frame,mlt_image_format imageformat);


