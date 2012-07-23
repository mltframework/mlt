/*********************************************************************
 * klt.c
 *
 * Kanade-Lucas-Tomasi tracker
 *********************************************************************/

/* Standard includes */
#include <math.h>    /* logf() */
#include <stdlib.h>  /* malloc() */

/* Our includes */
#include "base.h"
#include "convolve.h"
#include "error.h"
#include "klt.h"
#include "pyramid.h"

/*********************************************************************
 * KLTCreateTrackingContext
 *
 */

KLT_TrackingContext KLTCreateTrackingContext()
{
  KLT_TrackingContext tc = NULL;

  /* Allocate memory */
  tc = (KLT_TrackingContext) calloc(1, sizeof(KLT_TrackingContextRec));

  /* Set values to default values */
  tc->mindist = 10;
  tc->window_width = 7;
  tc->window_height = 7;
  tc->sequentialMode = FALSE;
  tc->smoothBeforeSelecting = TRUE;
  tc->min_eigenvalue = 1;
  tc->min_determinant = 0.01;
  tc->max_iterations = 10;
  tc->min_displacement = 0.1;
  tc->max_residue = 10.0;
  tc->grad_sigma = 1.0;
  tc->smooth_sigma_fact = 0.1;
  tc->pyramid_sigma_fact = 0.9;
  tc->step_factor = 1.0;
  tc->nSkippedPixels = 0;
  tc->pyramid_last = NULL;
  tc->pyramid_last_gradx = NULL;
  tc->pyramid_last_grady = NULL;
  tc->verbose = TRUE;

  /* Change nPyramidLevels and subsampling */
  KLTChangeTCPyramid(tc, 31);
	
  /* Update border, which is dependent upon  */
  /* smooth_sigma_fact, pyramid_sigma_fact, window_size, and subsampling */
  KLTUpdateTCBorder(tc);

  return(tc);
}

/*********************************************************************
 * KLTCreateFeatureList
 *
 */

KLT_FeatureList KLTCreateFeatureList(
  int nFeatures)
{
  KLT_FeatureList fl;
  KLT_Feature first;
  int nbytes = sizeof(KLT_FeatureListRec) +
    nFeatures * sizeof(KLT_Feature) +
    nFeatures * sizeof(KLT_FeatureRec);
  int i;
	
  /* Allocate memory for feature list */
  fl = (KLT_FeatureList)  malloc(nbytes);
	
  /* Set parameters */
  fl->nFeatures = nFeatures; 

  /* Set pointers */
  fl->feature = (KLT_Feature *) (fl + 1);
  first = (KLT_Feature) (fl->feature + nFeatures);
  for (i = 0 ; i < nFeatures ; i++)
    fl->feature[i] = first + i;

  /* Return feature list */
  return(fl);
}

/*********************************************************************
 * KLTPrintTrackingContext
 */

void KLTPrintTrackingContext(
  KLT_TrackingContext tc)
{
  fprintf(stderr, "\n\nTracking context:\n\n");
  fprintf(stderr, "\tmindist = %d\n", tc->mindist);
  fprintf(stderr, "\twindow_width = %d\n", tc->window_width);
  fprintf(stderr, "\twindow_height = %d\n", tc->window_height);
  fprintf(stderr, "\tsequentialMode = %s\n",
          tc->sequentialMode ? "TRUE" : "FALSE");
  fprintf(stderr, "\tsmoothBeforeSelecting = %s\n",
          tc->smoothBeforeSelecting ? "TRUE" : "FALSE");

  fprintf(stderr, "\tmin_eigenvalue = %d\n", tc->min_eigenvalue);
  fprintf(stderr, "\tmin_determinant = %f\n", tc->min_determinant);
  fprintf(stderr, "\tmin_displacement = %f\n", tc->min_displacement);
  fprintf(stderr, "\tmax_iterations = %d\n", tc->max_iterations);
  fprintf(stderr, "\tmax_residue = %f\n", tc->max_residue);
  fprintf(stderr, "\tgrad_sigma = %f\n", tc->grad_sigma);
  fprintf(stderr, "\tsmooth_sigma_fact = %f\n", tc->smooth_sigma_fact);
  fprintf(stderr, "\tpyramid_sigma_fact = %f\n", tc->pyramid_sigma_fact);
  fprintf(stderr, "\tnSkippedPixels = %d\n", tc->nSkippedPixels);
  fprintf(stderr, "\tborderx = %d\n", tc->borderx);
  fprintf(stderr, "\tbordery = %d\n", tc->bordery);
  fprintf(stderr, "\tnPyramidLevels = %d\n", tc->nPyramidLevels);
  fprintf(stderr, "\tsubsampling = %d\n", tc->subsampling);

  fprintf(stderr, "\n\tpyramid_last = %s\n", (tc->pyramid_last!=NULL) ?
          "points to old image" : "NULL");
  fprintf(stderr, "\tpyramid_last_gradx = %s\n", 
          (tc->pyramid_last_gradx!=NULL) ?
          "points to old image" : "NULL");
  fprintf(stderr, "\tpyramid_last_grady = %s\n",
          (tc->pyramid_last_grady!=NULL) ?
          "points to old image" : "NULL");
  fprintf(stderr, "\n\n");
}


/*********************************************************************
 * KLTChangeTCPyramid
 *
 */

void KLTChangeTCPyramid(
  KLT_TrackingContext tc,
  int search_range)
{
  float window_halfwidth;
  float subsampling;

  /* Check window size (and correct if necessary) */
  if (tc->window_width % 2 != 1) {
    tc->window_width = tc->window_width+1;
    KLTWarning("(KLTChangeTCPyramid) Window width must be odd.  "
               "Changing to %d.\n", tc->window_width);
  }
  if (tc->window_height % 2 != 1) {
    tc->window_height = tc->window_height+1;
    KLTWarning("(KLTChangeTCPyramid) Window height must be odd.  "
               "Changing to %d.\n", tc->window_height);
  }
  if (tc->window_width < 3) {
    tc->window_width = 3;
    KLTWarning("(KLTChangeTCPyramid) Window width must be at least three.  \n"
               "Changing to %d.\n", tc->window_width);
  }
  if (tc->window_height < 3) {
    tc->window_height = 3;
    KLTWarning("(KLTChangeTCPyramid) Window height must be at least three.  \n"
               "Changing to %d.\n", tc->window_height);
  }
  window_halfwidth = min(tc->window_width,tc->window_height)/2.0f;

  subsampling = ((float) search_range) / window_halfwidth;

  if (subsampling < 1.0)  {		/* 1.0 = 0+1 */
    tc->nPyramidLevels = 1;
  } else if (subsampling <= 3.0)  {	/* 3.0 = 2+1 */
    tc->nPyramidLevels = 2;
    tc->subsampling = 2;
  } else if (subsampling <= 5.0)  {	/* 5.0 = 4+1 */
    tc->nPyramidLevels = 2;
    tc->subsampling = 4;
  } else if (subsampling <= 9.0)  {	/* 9.0 = 8+1 */
    tc->nPyramidLevels = 2;
    tc->subsampling = 8;
  } else {
    /* The following lines are derived from the formula:
       search_range = 
       window_halfwidth * \sum_{i=0}^{nPyramidLevels-1} 8^i,
       which is the same as:
       search_range = 
       window_halfwidth * (8^nPyramidLevels - 1)/(8 - 1).
       Then, the value is rounded up to the nearest integer. */
    float val = (float) (log(7.0*subsampling+1.0)/log(8.0));
    tc->nPyramidLevels = (int) (val + 0.99);
    tc->subsampling = 8;
  }
}


/*********************************************************************
 * NOTE:  Manually must ensure consistency with _KLTComputePyramid()
 */
 
float _pyramidSigma(
  KLT_TrackingContext tc)
{
  return (tc->pyramid_sigma_fact * tc->subsampling);
}


/*********************************************************************
 * Updates border, which is dependent upon 
 * smooth_sigma_fact, pyramid_sigma_fact, window_size, and subsampling
 */

void KLTUpdateTCBorder(
  KLT_TrackingContext tc)
{
  float val;
  int pyramid_gauss_hw;
  int smooth_gauss_hw;
  int gauss_width, gaussderiv_width;
  int num_levels = tc->nPyramidLevels;
  int n_invalid_pixels;
  int window_hw;
  int ss = tc->subsampling;
  int ss_power;
  int border;
  int i;

  /* Check window size (and correct if necessary) */
  if (tc->window_width % 2 != 1) {
    tc->window_width = tc->window_width+1;
    KLTWarning("(KLTUpdateTCBorder) Window width must be odd.  "
               "Changing to %d.\n", tc->window_width);
  }
  if (tc->window_height % 2 != 1) {
    tc->window_height = tc->window_height+1;
    KLTWarning("(KLTUpdateTCBorder) Window height must be odd.  "
               "Changing to %d.\n", tc->window_height);
  }
  if (tc->window_width < 3) {
    tc->window_width = 3;
    KLTWarning("(KLTUpdateTCBorder) Window width must be at least three.  \n"
               "Changing to %d.\n", tc->window_width);
  }
  if (tc->window_height < 3) {
    tc->window_height = 3;
    KLTWarning("(KLTUpdateTCBorder) Window height must be at least three.  \n"
               "Changing to %d.\n", tc->window_height);
  }
  window_hw = max(tc->window_width, tc->window_height)/2;

  /* Find widths of convolution windows */
  _KLTGetKernelWidths(_KLTComputeSmoothSigma(tc),
                      &gauss_width, &gaussderiv_width);
  smooth_gauss_hw = gauss_width/2;
  _KLTGetKernelWidths(_pyramidSigma(tc),
                      &gauss_width, &gaussderiv_width);
  pyramid_gauss_hw = gauss_width/2;

  /* Compute the # of invalid pixels at each level of the pyramid.
     n_invalid_pixels is computed with respect to the ith level   
     of the pyramid.  So, e.g., if n_invalid_pixels = 5 after   
     the first iteration, then there are 5 invalid pixels in   
     level 1, which translated means 5*subsampling invalid pixels   
     in the original level 0. */
  n_invalid_pixels = smooth_gauss_hw;
  for (i = 1 ; i < num_levels ; i++)  {
    val = ((float) n_invalid_pixels + pyramid_gauss_hw) / ss;
    n_invalid_pixels = (int) (val + 0.99);  /* Round up */
  }

  /* ss_power = ss^(num_levels-1) */
  ss_power = 1;
  for (i = 1 ; i < num_levels ; i++)
    ss_power *= ss;

  /* Compute border by translating invalid pixels back into */
  /* original image */
  border = (n_invalid_pixels + window_hw) * ss_power;

  tc->borderx = border;
  tc->bordery = border;
}


/*********************************************************************
 * KLTFreeTrackingContext
 * KLTFreeFeatureList
 * KLTFreeFeatureHistory
 * KLTFreeFeatureTable
 */

void KLTFreeTrackingContext(
  KLT_TrackingContext tc)
{
  if (tc->pyramid_last)        
    _KLTFreePyramid((_KLT_Pyramid) tc->pyramid_last);
  if (tc->pyramid_last_gradx)  
    _KLTFreePyramid((_KLT_Pyramid) tc->pyramid_last_gradx);
  if (tc->pyramid_last_grady)  
    _KLTFreePyramid((_KLT_Pyramid) tc->pyramid_last_grady);
  free(tc);
}

void KLTFreeFeatureList(KLT_FeatureList fl) {

    free(fl);
}

/*********************************************************************
 * KLTStopSequentialMode
 */

void KLTStopSequentialMode(
  KLT_TrackingContext tc)
{
  tc->sequentialMode = FALSE;
  _KLTFreePyramid((_KLT_Pyramid) tc->pyramid_last);
  _KLTFreePyramid((_KLT_Pyramid) tc->pyramid_last_gradx);
  _KLTFreePyramid((_KLT_Pyramid) tc->pyramid_last_grady);
  tc->pyramid_last = NULL;
  tc->pyramid_last_gradx = NULL;
  tc->pyramid_last_grady = NULL;
}


/*********************************************************************
 * KLTCountRemainingFeatures
 */

int KLTCountRemainingFeatures(
  KLT_FeatureList fl)
{
  int count = 0;
  int i;

  for (i = 0 ; i < fl->nFeatures ; i++)
    if (fl->feature[i]->val >= 0)
      count++;

  return count;
}

