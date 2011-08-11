/*********************************************************************
 * klt.h
 *
 * Kanade-Lucas-Tomasi tracker
 *********************************************************************/

#ifndef _KLT_H_
#define _KLT_H_

typedef float KLT_locType;
typedef unsigned char KLT_PixelType;

#define KLT_BOOL int

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef NULL
#define NULL  0
#endif

#define KLT_TRACKED           0
#define KLT_NOT_FOUND        -1
#define KLT_SMALL_DET        -2
#define KLT_MAX_ITERATIONS   -3
#define KLT_OOB              -4
#define KLT_LARGE_RESIDUE    -5

#include "klt_util.h" /* for affine mapping */

/*******************
 * Structures
 */

typedef struct  {
  /* Available to user */
  int mindist;			            /* min distance b/w features */
  int window_width, window_height;
  KLT_BOOL sequentialMode;          /* whether to save most recent image to save time */
                                    /* can set to TRUE manually, but don't set to */
                                    /* FALSE manually */
  KLT_BOOL smoothBeforeSelecting;	/* whether to smooth image before */
                                    /* selecting features */
  
  /* Available, but hopefully can ignore */
  int min_eigenvalue;		/* smallest eigenvalue allowed for selecting */
  float min_determinant;	/* th for determining lost */
  float min_displacement;	/* th for stopping tracking when pixel changes little */
  int max_iterations;		/* th for stopping tracking when too many iterations */
  float max_residue;		/* th for stopping tracking when residue is large */
  float grad_sigma;
  float smooth_sigma_fact;
  float pyramid_sigma_fact;
  float step_factor;        /* size of Newton steps; 2.0 comes from equations, 1.0 seems to avoid overshooting */
  int nSkippedPixels;		/* # of pixels skipped when finding features */
  int borderx;              /* border in which features will not be found */
  int bordery;
  int nPyramidLevels;       /* computed from search_ranges */
  int subsampling;          /* 		" */

  /* User must not touch these */
  void *pyramid_last;
  void *pyramid_last_gradx;
  void *pyramid_last_grady;
  
  int verbose;
  
}  KLT_TrackingContextRec, *KLT_TrackingContext;

typedef struct  {
  KLT_locType x;
  KLT_locType y;
  int val;	
}  KLT_FeatureRec, *KLT_Feature;

typedef struct  {
  int nFeatures;
  KLT_Feature *feature;
}  KLT_FeatureListRec, *KLT_FeatureList;

typedef struct  {
  int nFrames;
  KLT_Feature *feature;
}  KLT_FeatureHistoryRec, *KLT_FeatureHistory;

typedef struct  {
  int nFrames;
  int nFeatures;
  KLT_Feature **feature;
}  KLT_FeatureTableRec, *KLT_FeatureTable;

/*******************
 * Functions
 */

/* Create */
KLT_TrackingContext KLTCreateTrackingContext();
KLT_FeatureList KLTCreateFeatureList(int);

/* Free */
void KLTFreeTrackingContext(KLT_TrackingContext);
void KLTFreeFeatureList(KLT_FeatureList);

/* Processing */
void KLTSelectGoodFeatures(
  KLT_TrackingContext tc,
  KLT_PixelType *img,
  int ncols,
  int nrows,
  KLT_FeatureList fl);
void KLTTrackFeatures(
  KLT_TrackingContext tc,
  KLT_PixelType *img1,
  KLT_PixelType *img2,
  int ncols,
  int nrows,
  KLT_FeatureList fl);
void KLTReplaceLostFeatures(
  KLT_TrackingContext tc,
  KLT_PixelType *img,
  int ncols,
  int nrows,
  KLT_FeatureList fl);

/* Utilities */
int KLTCountRemainingFeatures(
  KLT_FeatureList fl);
void KLTPrintTrackingContext(
  KLT_TrackingContext tc);
void KLTChangeTCPyramid(
  KLT_TrackingContext tc,
  int search_range);
void KLTUpdateTCBorder(
  KLT_TrackingContext tc);
void KLTStopSequentialMode(
  KLT_TrackingContext tc);
float _KLTComputeSmoothSigma(
  KLT_TrackingContext tc);

#endif

