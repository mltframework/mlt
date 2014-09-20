/*
 *  filter_stabilize.c
 *
 *  Copyright (C) Georg Martius - June 2007
 *   georg dot martius at web dot de
 *  mlt adaption by Marco Gittler marco at gitma dot de 2011
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
#define MAX(a,b)         ((a < b) ?  (b) : (a))
#define MIN(a,b)         ((a < b) ?  (a) : (b))
#include "stabilize.h"
#include <stdlib.h>
#include <string.h>
#include <framework/mlt_types.h>
#include <framework/mlt_log.h>
#ifdef USE_SSE2
#include <emmintrin.h>
#endif

void addTrans(StabData* sd, Transform sl)
{
    if (!sd->transs) {
       sd->transs = tlist_new(0);
    }
    tlist_append(sd->transs, &sl,sizeof(Transform) );
}



/** initialise measurement fields on the frame.
    The size of the fields and the maxshift is used to
    calculate an optimal distribution in the frame.
*/
int initFields(StabData* sd)
{
    int size     = sd->field_size;
    int rows = MAX(3,(sd->height - sd->maxshift*2)/size-1);
    int cols = MAX(3,(sd->width  - sd->maxshift*2)/size-1);
    // make sure that the remaining rows have the same length
    sd->field_num  = rows*cols;
    sd->field_rows = rows;
    mlt_log_debug (NULL,"field setup: rows: %i cols: %i Total: %i fields",
                rows, cols, sd->field_num);

    if (!(sd->fields = malloc(sizeof(Field) * sd->field_num))) {
        mlt_log_error ( NULL,  "malloc failed!\n");
        return 0;
    } else {
        int i, j;
        // the border is the amount by which the field centers
        // have to be away from the image boundary
        // (stepsize is added in case shift is increased through stepsize)
        int border   = size/2 + sd->maxshift + sd->stepsize;
        int step_x   = (sd->width  - 2*border)/MAX(cols-1,1);
        int step_y   = (sd->height - 2*border) / MAX(rows-1,1);
        for (j = 0; j < rows; j++) {
            for (i = 0; i < cols; i++) {
                int idx = j*cols+i;
                sd->fields[idx].x = border + i*step_x;
                sd->fields[idx].y = border + j*step_y;
                sd->fields[idx].size = size;
            }
        }
    }
    return 1;
}


/**
   compares the two given images and returns the average absolute difference
   \param d_x shift in x direction
   \param d_y shift in y direction
*/
double compareImg(unsigned char* I1, unsigned char* I2,
                  int width, int height,  int bytesPerPixel, int d_x, int d_y)
{
    int i, j;
    unsigned char* p1 = NULL;
    unsigned char* p2 = NULL;
    long int sum = 0;
    int effectWidth = width - abs(d_x);
    int effectHeight = height - abs(d_y);

/*   DEBUGGING code to export single frames */
/*   char buffer[100]; */
/*   sprintf(buffer, "pic_%02ix%02i_1.ppm", d_x, d_y); */
/*   FILE *pic1 = fopen(buffer, "w"); */
/*   sprintf(buffer, "pic_%02ix%02i_2.ppm", d_x, d_y); */
/*   FILE *pic2 = fopen(buffer, "w"); */
/*   fprintf(pic1, "P6\n%i %i\n255\n", effectWidth, effectHeight); */
/*   fprintf(pic2, "P6\n%i %i\n255\n", effectWidth, effectHeight); */

    for (i = 0; i < effectHeight; i++) {
        p1 = I1;
        p2 = I2;
        if (d_y > 0 ){
            p1 += (i + d_y) * width * bytesPerPixel;
            p2 += i * width * bytesPerPixel;
        } else {
            p1 += i * width * bytesPerPixel;
            p2 += (i - d_y) * width * bytesPerPixel;
        }
        if (d_x > 0) {
            p1 += d_x * bytesPerPixel;
        } else {
            p2 -= d_x * bytesPerPixel;
        }
#ifdef USE_SSE2
		__m128i A,B,C,D,E;
		for (j = 0; j < effectWidth * bytesPerPixel - 16; j++) {
#else
		for (j = 0; j < effectWidth * bytesPerPixel; j++) {
#endif
            /* fwrite(p1,1,1,pic1);fwrite(p1,1,1,pic1);fwrite(p1,1,1,pic1);
               fwrite(p2,1,1,pic2);fwrite(p2,1,1,pic2);fwrite(p2,1,1,pic2);
             */
#ifdef USE_SSE2
			A= _mm_loadu_si128((__m128i*)p1); //load unaligned data
			B= _mm_loadu_si128((__m128i*)p2);
			C= _mm_sad_epu8(A, B);//diff per 8 bit pixel stored in 2 64 byte values
			D = _mm_srli_si128(C, 8); // shift first 64 byte value to align at the same as C
			E = _mm_add_epi32(C, D); // add the 2 values (sum of all diffs)
			sum+= _mm_cvtsi128_si32(E); //convert _m128i to int

            p1+=16;
            p2+=16;
			j+=15;
#else
            sum += abs((int)*p1 - (int)*p2);
            p1++;
            p2++;
#endif
        }
    }
    /*  fclose(pic1);
        fclose(pic2);
     */
    return sum/((double) effectWidth * effectHeight * bytesPerPixel);
}

/**
   compares a small part of two given images
   and returns the average absolute difference.
   Field center, size and shift have to be choosen,
   so that no clipping is required

   \param field Field specifies position(center) and size of subimage
   \param d_x shift in x direction
   \param d_y shift in y direction
*/
double compareSubImg(unsigned char* const I1, unsigned char* const I2,
                     const Field* field,
                     int width, int height, int bytesPerPixel, int d_x, int d_y)
{
    int k, j;
    unsigned char* p1 = NULL;
    unsigned char* p2 = NULL;
    int s2 = field->size / 2;
    double sum=0.0;

    p1=I1 + ((field->x - s2) + (field->y - s2)*width)*bytesPerPixel;

    p2=I2 + ((field->x - s2 + d_x) + (field->y - s2 + d_y)*width)*bytesPerPixel;
    // TODO: use some mmx or sse stuff here
#ifdef USE_SSE2
	__m128i A,B,C,D,E;
#endif
    for (j = 0; j < field->size; j++){
#ifdef USE_SSE2
			for (k = 0; k < (field->size * bytesPerPixel) - 16 ; k++) {
				A= _mm_loadu_si128((__m128i*)p1); //load unaligned data
				B= _mm_loadu_si128((__m128i*)p2);
				C= _mm_sad_epu8(A, B);//abd diff stored in 2 64 byte values
				D = _mm_srli_si128(C, 8); // shift value 8 byte right
				E = _mm_add_epi32(C, D); // add the 2 values (sum of all diffs)
				sum+= _mm_cvtsi128_si32(E); //convert _m128i to int

            p1+=16;
            p2+=16;
			k+=15;
#else
			for (k = 0; k < (field->size * bytesPerPixel); k++) {
            sum += abs((int)*p1 - (int)*p2);
            p1++;
            p2++;
#endif
        }
        p1 += ((width - field->size) * bytesPerPixel);
        p2 += ((width - field->size) * bytesPerPixel);
    }
    return sum/((double) field->size *field->size* bytesPerPixel);
}

/** \see contrastSubImg called with bytesPerPixel=1*/
double contrastSubImgYUV(StabData* sd, const Field* field){
    return contrastSubImg(sd->curr,field,sd->width,sd->height,1);
}

/**
    \see contrastSubImg three times called with bytesPerPixel=3
    for all channels
 */
double contrastSubImgRGB(StabData* sd, const Field* field){
    unsigned char* const I = sd->curr;
    return (  contrastSubImg(I,  field,sd->width,sd->height,3)
            + contrastSubImg(I+1,field,sd->width,sd->height,3)
            + contrastSubImg(I+2,field,sd->width,sd->height,3))/3;
}

/**
   calculates Michelson-contrast in the given small part of the given image

   \param I pointer to framebuffer
   \param field Field specifies position(center) and size of subimage
   \param width width of frame
   \param height height of frame
   \param bytesPerPixel calc contrast for only for first channel
*/
double contrastSubImg(unsigned char* const I, const Field* field,
                     int width, int height, int bytesPerPixel)
{
#if USE_SSE2
    int k, j;
    int s2 = field->size / 2;
    __m128i mini = _mm_set_epi64(_mm_set1_pi8(255), _mm_set1_pi8(255));
	__m128i maxi = _mm_set_epi64(_mm_set1_pi8(0)  , _mm_set1_pi8(0));
    unsigned char* p = I + ((field->x - s2) + (field->y - s2)*width)*bytesPerPixel;
	__m128i A;
    for (j = 0; j < field->size; j++){
        for (k = 0; k < (field->size * bytesPerPixel)-16 ; k++) {
			A= _mm_loadu_si128((__m128i*)p); //load unaligned data

			maxi = _mm_max_epu8(A,maxi);
			mini = _mm_min_epu8(A,mini);
            p+=16;
			k+=15;
        }
        p += (width - field->size) * bytesPerPixel;
    }
	int max=0;
	union {
		__m128i m;
		uint8_t t[16];
	} m;
	m.m=maxi;
	for (j=0;j<16;j++) {
		if (m.t[j] >max)
			max=m.t[j];
	}
	int min=255;
	m.m=mini;
	for (j=0;j<16;j++) {
		if (m.t[j] <min)
			min=m.t[j];
	}
	//printf("max=%d  min=%d\n", max,min);
    return (max-min)/(max+min+0.1); // +0.1 to avoid division by 0
#else

    int k, j;
    unsigned char* p = NULL;
    int s2 = field->size / 2;
    unsigned char mini = 255;
    unsigned char maxi = 0;
    p = I + ((field->x - s2) + (field->y - s2)*width)*bytesPerPixel;
    // TODO: use some mmx or sse stuff here

    for (j = 0; j < field->size; j++){
        for (k = 0; k < field->size * bytesPerPixel; k++) {
            mini = (mini < *p) ? mini : *p;
            maxi = (maxi > *p) ? maxi : *p;
            p += bytesPerPixel;
        }
        p += (width - field->size) * bytesPerPixel;
    }
    return (maxi-mini)/(maxi+mini+0.1); // +0.1 to avoid division by 0
#endif
}

/** tries to register current frame onto previous frame.
    This is the most simple algorithm:
    shift images to all possible positions and calc summed error
    Shift with minimal error is selected.
*/
Transform calcShiftRGBSimple(StabData* sd)
{
    int x = 0, y = 0;
    int i, j;
    double minerror = 1e20;
    for (i = -sd->maxshift; i <= sd->maxshift; i++) {
        for (j = -sd->maxshift; j <= sd->maxshift; j++) {
            double error = compareImg(sd->curr, sd->prev,
                                      sd->width, sd->height, 3, i, j);
            if (error < minerror) {
                minerror = error;
                x = i;
                y = j;
           }
        }
    }
    return new_transform(x, y, 0, 0, 0);
}


/** tries to register current frame onto previous frame.
    (only the luminance is used)
    This is the most simple algorithm:
    shift images to all possible positions and calc summed error
    Shift with minimal error is selected.
*/
Transform calcShiftYUVSimple(StabData* sd)
{
    int x = 0, y = 0;
    int i, j;
    unsigned char *Y_c, *Y_p;// , *Cb, *Cr;
#ifdef STABVERBOSE
    FILE *f = NULL;
    char buffer[32];
    tc_snprintf(buffer, sizeof(buffer), "f%04i.dat", sd->t);
    f = fopen(buffer, "w");
    fprintf(f, "# splot \"%s\"\n", buffer);
#endif

    // we only use the luminance part of the image
    Y_c  = sd->curr;
    //  Cb_c = sd->curr + sd->width*sd->height;
    //Cr_c = sd->curr + 5*sd->width*sd->height/4;
    Y_p  = sd->prev;
    //Cb_p = sd->prev + sd->width*sd->height;
    //Cr_p = sd->prev + 5*sd->width*sd->height/4;

    double minerror = 1e20;
    for (i = -sd->maxshift; i <= sd->maxshift; i++) {
        for (j = -sd->maxshift; j <= sd->maxshift; j++) {
            double error = compareImg(Y_c, Y_p,
                                      sd->width, sd->height, 1, i, j);
#ifdef STABVERBOSE
            fprintf(f, "%i %i %f\n", i, j, error);
#endif
            if (error < minerror) {
                minerror = error;
                x = i;
                y = j;
            }
        }
    }
#ifdef STABVERBOSE
    fclose(f);
    tc_log_msg(MOD_NAME, "Minerror: %f\n", minerror);
#endif
    return new_transform(x, y, 0, 0, 0);
}



/* calculates rotation angle for the given transform and
 * field with respect to the given center-point
 */
double calcAngle(StabData* sd, Field* field, Transform* t,
                 int center_x, int center_y)
{
    // we better ignore fields that are to close to the rotation center
    if (abs(field->x - center_x) + abs(field->y - center_y) < sd->maxshift) {
        return 0;
    } else {
        // double r = sqrt(field->x*field->x + field->y*field->y);
        double a1 = atan2(field->y - center_y, field->x - center_x);
        double a2 = atan2(field->y - center_y + t->y,
                          field->x - center_x + t->x);
        double diff = a2 - a1;
        return (diff>M_PI) ? diff - 2*M_PI
            : ( (diff<-M_PI) ? diff + 2*M_PI : diff);
    }
}


/* calculates the optimal transformation for one field in YUV frames
 * (only luminance)
 */
Transform calcFieldTransYUV(StabData* sd, const Field* field, int fieldnum)
{
    Transform t = null_transform();
    unsigned char *Y_c = sd->curr, *Y_p = sd->prev;
    // we only use the luminance part of the image
    int i, j;

/*     // check contrast in sub image */
/*     double contr = contrastSubImg(Y_c, field, sd->width, sd->height, 1); */
/*     if(contr < sd->contrast_threshold) { */
/*         t.extra=-1; */
/*         return t; */
/*     } */
#ifdef STABVERBOSE
    // printf("%i %i %f\n", sd->t, fieldnum, contr);
    FILE *f = NULL;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "f%04i_%02i.dat", sd->t, fieldnum);
    f = fopen(buffer, "w");
    fprintf(f, "# splot \"%s\"\n", buffer);
#endif

    double minerror = 1e10;
    double error = 1e10;
    for (i = -sd->maxshift; i <= sd->maxshift; i += sd->stepsize) {
        for (j = -sd->maxshift; j <= sd->maxshift; j += sd->stepsize) {
            error = compareSubImg(Y_c, Y_p, field,
                                         sd->width, sd->height, 1, i, j);
#ifdef STABVERBOSE
            fprintf(f, "%i %i %f\n", i, j, error);
#endif
            if (error < minerror) {
                minerror = error;
                t.x = i;
                t.y = j;
            }
        }
    }

    if (sd->stepsize > 1) {    // make fine grain check around the best match
        int r = sd->stepsize - 1;
        for (i = t.x - r; i <= t.x + r; i += 1) {
            for (j = -t.y - r; j <= t.y + r; j += 1) {
                if (i == t.x && j == t.y)
                    continue; //no need to check this since already done
                error = compareSubImg(Y_c, Y_p, field,
                                      sd->width, sd->height, 1, i, j);
#ifdef STABVERBOSE
                fprintf(f, "%i %i %f\n", i, j, error);
#endif
                if (error < minerror){
                    minerror = error;
                    t.x = i;
                    t.y = j;
                }
            }
        }
    }
#ifdef STABVERBOSE
    fclose(f);
    mlt_log_debug ( "Minerror: %f\n", minerror);
#endif

    if (!sd->allowmax && fabs(t.x) == sd->maxshift) {
#ifdef STABVERBOSE
        mlt_log_debug ( "maximal x shift ");
#endif
        t.x = 0;
    }
    if (!sd->allowmax && fabs(t.y) == sd->maxshift) {
#ifdef STABVERBOSE
        mlt_log_debug ("maximal y shift ");
#endif
        t.y = 0;
    }
    return t;
}

/* calculates the optimal transformation for one field in RGB
 *   slower than the YUV version because it uses all three color channels
 */
Transform calcFieldTransRGB(StabData* sd, const Field* field, int fieldnum)
{
    Transform t = null_transform();
    unsigned char *I_c = sd->curr, *I_p = sd->prev;
    int i, j;

    double minerror = 1e20;
    for (i = -sd->maxshift; i <= sd->maxshift; i += 2) {
        for (j=-sd->maxshift; j <= sd->maxshift; j += 2) {
            double error = compareSubImg(I_c, I_p, field,
                                         sd->width, sd->height, 3, i, j);
            if (error < minerror) {
                minerror = error;
                t.x = i;
                t.y = j;
            }
        }
    }
    for (i = t.x - 1; i <= t.x + 1; i += 2) {
        for (j = -t.y - 1; j <= t.y + 1; j += 2) {
            double error = compareSubImg(I_c, I_p, field,
                                         sd->width, sd->height, 3, i, j);
            if (error < minerror) {
                minerror = error;
                t.x = i;
                t.y = j;
            }
        }
    }
    if (!sd->allowmax && fabs(t.x) == sd->maxshift) {
        t.x = 0;
    }
    if (!sd->allowmax && fabs(t.y) == sd->maxshift) {
        t.y = 0;
    }
    return t;
}

/* compares contrast_idx structures respect to the contrast
   (for sort function)
*/
int cmp_contrast_idx(const void *ci1, const void* ci2)
{
    double a = ((contrast_idx*)ci1)->contrast;
    double b = ((contrast_idx*)ci2)->contrast;
    return a < b ? 1 : ( a > b ? -1 : 0 );
}

/* select only the best 'maxfields' fields
   first calc contrasts then select from each part of the
   frame a some fields
*/
tlist* selectfields(StabData* sd, contrastSubImgFunc contrastfunc)
{
    int i,j;
    tlist* goodflds = tlist_new(0);
    contrast_idx *ci = malloc(sizeof(contrast_idx) * sd->field_num);

    // we split all fields into row+1 segments and take from each segment
    // the best fields
    int numsegms = (sd->field_rows+1);
    int segmlen = sd->field_num/(sd->field_rows+1)+1;
    // split the frame list into rows+1 segments
    contrast_idx *ci_segms = malloc(sizeof(contrast_idx) * sd->field_num);
    int remaining   = 0;
    // calculate contrast for each field
    for (i = 0; i < sd->field_num; i++) {
        ci[i].contrast = contrastfunc(sd, &sd->fields[i]);
        ci[i].index=i;
        if(ci[i].contrast < sd->contrast_threshold) ci[i].contrast = 0;
        // else printf("%i %lf\n", ci[i].index, ci[i].contrast);
    }

    memcpy(ci_segms, ci, sizeof(contrast_idx) * sd->field_num);
    // get best fields from each segment
    for(i=0; i<numsegms; i++){
        int startindex = segmlen*i;
        int endindex   = segmlen*(i+1);
        endindex       = endindex > sd->field_num ? sd->field_num : endindex;
        //printf("Segment: %i: %i-%i\n", i, startindex, endindex);

        // sort within segment
        qsort(ci_segms+startindex, endindex-startindex,
              sizeof(contrast_idx), cmp_contrast_idx);
        // take maxfields/numsegms
        for(j=0; j<sd->maxfields/numsegms; j++){
            if(startindex+j >= endindex) continue;
            // printf("%i %lf\n", ci_segms[startindex+j].index,
            //                    ci_segms[startindex+j].contrast);
            if(ci_segms[startindex+j].contrast > 0){
               tlist_append(goodflds, &ci[ci_segms[startindex+j].index],sizeof(contrast_idx));
                // don't consider them in the later selection process
                ci_segms[startindex+j].contrast=0;
            }
        }
    }
    // check whether enough fields are selected
    // printf("Phase2: %i\n", tc_list_size(goodflds));
    remaining = sd->maxfields - tlist_size(goodflds);
    if(remaining > 0){
        // take the remaining from the leftovers
        qsort(ci_segms, sd->field_num,
              sizeof(contrast_idx), cmp_contrast_idx);
        for(j=0; j < remaining; j++){
            if(ci_segms[j].contrast > 0){
                tlist_append(goodflds, &ci_segms[j], sizeof(contrast_idx));
            }
        }
    }
    // printf("Ende: %i\n", tc_list_size(goodflds));
    free(ci);
    free(ci_segms);
    return goodflds;
}



/* tries to register current frame onto previous frame.
 *   Algorithm:
 *   check all fields for vertical and horizontal transformation
 *   use minimal difference of all possible positions
 *   discards fields with low contrast
 *   select maxfields field according to their contrast
 *   calculate shift as cleaned mean of all remaining fields
 *   calculate rotation angle of each field in respect to center of fields
 *   after shift removal
 *   calculate rotation angle as cleaned mean of all angles
 *   compensate for possibly off-center rotation
*/
Transform calcTransFields(StabData* sd, calcFieldTransFunc fieldfunc,
                          contrastSubImgFunc contrastfunc)
{
    Transform* ts  = malloc(sizeof(Transform) * sd->field_num);
    Field** fs     = malloc(sizeof(Field*) * sd->field_num);
    double *angles = malloc(sizeof(double) * sd->field_num);
    int i, index=0, num_trans;
    Transform t;
#ifdef STABVERBOSE
    FILE *f = NULL;
    char buffer[32];
    tc_snprintf(buffer, sizeof(buffer), "k%04i.dat", sd->t);
    f = fopen(buffer, "w");
    fprintf(f, "# plot \"%s\" w l, \"\" every 2:1:0\n", buffer);
#endif


    tlist* goodflds = selectfields(sd, contrastfunc);

    // use all "good" fields and calculate optimal match to previous frame
    contrast_idx* f;
    while((f = (contrast_idx*)tlist_pop(goodflds,0) ) != 0){
        int i = f->index;
        t =  fieldfunc(sd, &sd->fields[i], i); // e.g. calcFieldTransYUV
#ifdef STABVERBOSE
        fprintf(f, "%i %i\n%f %f %i\n \n\n", sd->fields[i].x, sd->fields[i].y,
                sd->fields[i].x + t.x, sd->fields[i].y + t.y, t.extra);
#endif
        if (t.extra != -1){ // ignore if extra == -1 (unused at the moment)
            ts[index] = t;
            fs[index] = sd->fields+i;
            index++;
        }
    }
    tlist_fini(goodflds);

    t = null_transform();
    num_trans = index; // amount of transforms we actually have
    if (num_trans < 1) {
        printf( "too low contrast! No field remains.\n"
                "(no translations are detected in frame %i)", sd->t);
        free(ts);
        free(fs);
        free(angles);
        return t;
    }

    int center_x = 0;
    int center_y = 0;
    // calc center point of all remaining fields
    for (i = 0; i < num_trans; i++) {
        center_x += fs[i]->x;
        center_y += fs[i]->y;
    }
    center_x /= num_trans;
    center_y /= num_trans;

    if (sd->show){ // draw fields and transforms into frame.
        // this has to be done one after another to handle possible overlap
        if (sd->show > 1) {
            for (i = 0; i < num_trans; i++)
                drawFieldScanArea(sd, fs[i], &ts[i]);
        }
        for (i = 0; i < num_trans; i++)
            drawField(sd, fs[i], &ts[i]);
        for (i = 0; i < num_trans; i++)
            drawFieldTrans(sd, fs[i], &ts[i]);
    }
    /* median over all transforms
       t= median_xy_transform(ts, sd->field_num);*/
    // cleaned mean
    t = cleanmean_xy_transform(ts, num_trans);

    // substract avg
    for (i = 0; i < num_trans; i++) {
        ts[i] = sub_transforms(&ts[i], &t);
    }
    // figure out angle
    if (sd->field_num < 6) {
        // the angle calculation is inaccurate for 5 and less fields
        t.alpha = 0;
    } else {
        for (i = 0; i < num_trans; i++) {
            angles[i] = calcAngle(sd, fs[i], &ts[i], center_x, center_y);
        }
        double min,max;
        t.alpha = -cleanmean(angles, num_trans, &min, &max);
        if(max-min>sd->maxanglevariation){
            t.alpha=0;
            printf( "too large variation in angle(%f)\n",
                        max-min);
        }
    }

    // compensate for off-center rotation
    double p_x = (center_x - sd->width/2);
    double p_y = (center_y - sd->height/2);
    t.x += (cos(t.alpha)-1)*p_x  - sin(t.alpha)*p_y;
    t.y += sin(t.alpha)*p_x  + (cos(t.alpha)-1)*p_y;

#ifdef STABVERBOSE
    fclose(f);
#endif
    free(ts);
    free(fs);
    free(angles);
    return t;
}

/** draws the field scanning area */
void drawFieldScanArea(StabData* sd, const Field* field, const Transform* t)
{
	if (sd->pixelformat != mlt_image_yuv420p) {
		mlt_log_warning (NULL, "format not usable\n");
        return;
	}
    drawBox(sd->curr, sd->width, sd->height, 1, field->x, field->y,
            field->size+2*sd->maxshift, field->size+2*sd->maxshift, 80);
}

/** draws the field */
void drawField(StabData* sd, const Field* field, const Transform* t)
{
	if (sd->pixelformat != mlt_image_yuv420p){
		mlt_log_warning (NULL, "format not usable\n");
        return;
	}
    drawBox(sd->curr, sd->width, sd->height, 1, field->x, field->y,
            field->size, field->size, t->extra == -1 ? 100 : 40);
}

/** draws the transform data of this field */
void drawFieldTrans(StabData* sd, const Field* field, const Transform* t)
{
	if (sd->pixelformat != mlt_image_yuv420p){
		mlt_log_warning (NULL, "format not usable\n");
        return;
	}
    drawBox(sd->curr, sd->width, sd->height, 1,
            field->x, field->y, 5, 5, 128);     // draw center
    drawBox(sd->curr, sd->width, sd->height, 1,
            field->x + t->x, field->y + t->y, 8, 8, 250); // draw translation
}

/**
 * draws a box at the given position x,y (center) in the given color
   (the same for all channels)
 */
void drawBox(unsigned char* I, int width, int height, int bytesPerPixel,
             int x, int y, int sizex, int sizey, unsigned char color){

    unsigned char* p = NULL;
    int j,k;
    p = I + ((x - sizex/2) + (y - sizey/2)*width)*bytesPerPixel;
    for (j = 0; j < sizey; j++){
        for (k = 0; k < sizex * bytesPerPixel; k++) {
            *p = color;
            p++;
        }
        p += (width - sizex) * bytesPerPixel;
    }
}

struct iterdata {
    FILE *f;
    int  counter;
};

/*************************************************************************/

/* Module interface routines and data. */

/*************************************************************************/

/*
 * stabilize_configure:  Configure this instance of the module.  See
 * tcmodule-data.h for function details.
 */
int stabilize_configure(StabData* instance
/*const char *options,
                               int pixelfmt */
                               /*TCModuleExtraData *xdata[]*/)
{
    StabData *sd = instance;
    /*    sd->framesize = sd->vob->im_v_width * MAX_PLANES *
          sizeof(char) * 2 * sd->vob->im_v_height * 2;     */
    /*TODO sd->framesize = sd->vob->im_v_size;    */
    sd->prev = calloc(1,sd->framesize);
		sd->grayimage = calloc(1,sd->width*sd->height);

    if (!sd->prev || !sd->grayimage) {
        printf( "malloc failed");
        return -1;
    }
    sd->currcopy = 0;
    sd->hasSeenOneFrame = 0;
    sd->transs = 0;
    sd->allowmax   = 0;
    sd->field_size  = MIN(sd->width, sd->height)/12;
    sd->maxanglevariation = 1;

    sd->shakiness = MIN(10,MAX(1,sd->shakiness));
    sd->accuracy  = MAX(sd->shakiness,MIN(15,MAX(1,sd->accuracy)));
    if (1) {
        mlt_log_debug (NULL, "Image Stabilization Settings:\n");
        mlt_log_debug (NULL, "     shakiness = %d\n", sd->shakiness);
        mlt_log_debug (NULL, "      accuracy = %d\n", sd->accuracy);
        mlt_log_debug (NULL, "      stepsize = %d\n", sd->stepsize);
        mlt_log_debug (NULL, "          algo = %d\n", sd->algo);
        mlt_log_debug (NULL, "   mincontrast = %f\n", sd->contrast_threshold);
        mlt_log_debug (NULL, "          show = %d\n", sd->show);
    }

#ifndef USE_SSE2
	mlt_log_info(NULL,"No SSE2 support enabled, this will slow down a lot\n");
#endif
    // shift and size: shakiness 1: height/40; 10: height/4
    sd->maxshift    = MIN(sd->width, sd->height)*sd->shakiness/40;
    sd->field_size   = MIN(sd->width, sd->height)*sd->shakiness/40;

    mlt_log_debug ( NULL,  "Fieldsize: %i, Maximal translation: %i pixel\n",
                sd->field_size, sd->maxshift);
    if (sd->algo==1) {
        // initialize measurement fields. field_num is set here.
        if (!initFields(sd)) {
            return -1;
        }
        sd->maxfields = (sd->accuracy) * sd->field_num / 15;
        mlt_log_debug ( NULL, "Number of used measurement fields: %i out of %i\n",
                    sd->maxfields, sd->field_num);
    }
    if (sd->show){
        sd->currcopy = calloc(1,sd->framesize);
	}

    /* load unsharp filter to smooth the frames. This allows larger stepsize.*/
    char unsharp_param[128];
    int masksize = MIN(13,sd->stepsize*1.8); // only works up to 13.
    sprintf(unsharp_param,"luma=-1:luma_matrix=%ix%i:pre=1",
            masksize, masksize);
    return 0;
}


/**
 * stabilize_filter_video: performs the analysis of subsequent frames
 * See tcmodule-data.h for function details.
 */

int stabilize_filter_video(StabData* instance,
                                  unsigned char *frame,mlt_image_format pixelformat)
{
    StabData *sd = instance;
    sd->pixelformat=pixelformat;
		int l=sd->width*sd->height;
		unsigned char* tmpgray=sd->grayimage;
		if (pixelformat == mlt_image_yuv422){
			while(l--){
				*tmpgray++=*frame++;
				frame++;
			};
		}

    if(sd->show) { // save the buffer to restore at the end for prev
			if (pixelformat == mlt_image_yuv420p){
				memcpy(sd->currcopy, sd->grayimage, sd->framesize);
			}
		}
    if (sd->hasSeenOneFrame) {
        sd->curr = sd->grayimage;
        if (pixelformat == mlt_image_rgb24) {
            if (sd->algo == 0)
                addTrans(sd, calcShiftRGBSimple(sd));
            else if (sd->algo == 1)
                addTrans(sd, calcTransFields(sd, calcFieldTransRGB,
                         contrastSubImgRGB));
        } else if (pixelformat == mlt_image_yuv420p ) {
            if (sd->algo == 0)
                addTrans(sd, calcShiftYUVSimple(sd));
            else if (sd->algo == 1)
                addTrans(sd, calcTransFields(sd, calcFieldTransYUV,
                                             contrastSubImgYUV));
        } else if (pixelformat == mlt_image_yuv422 ) {
            if (sd->algo == 0)
                addTrans(sd, calcShiftYUVSimple(sd));
            else if (sd->algo == 1)
                addTrans(sd, calcTransFields(sd, calcFieldTransYUV,
                                             contrastSubImgYUV));
        } else {
            mlt_log_warning (NULL,"unsupported Codec: %i\n",
                        pixelformat);
            return 0;
        }
    } else {
        sd->hasSeenOneFrame = 1;
        addTrans(sd, null_transform());
    }

    if(!sd->show) { // copy current frame to prev for next frame comparison
        memcpy(sd->prev, sd->grayimage, sd->framesize);
    } else { // use the copy because we changed the original frame
        memcpy(sd->prev, sd->currcopy, sd->framesize);
    }
    sd->t++;
    return 0;
}

/**
 * stabilize_stop:  Reset this instance of the module.  See tcmodule-data.h
 * for function details.
 */

int stabilize_stop(StabData* instance)
{
    StabData *sd = instance;
    free(sd->prev);
    sd->prev = NULL;
    free(sd->grayimage);
    sd->grayimage=NULL;
    return 0;
}



