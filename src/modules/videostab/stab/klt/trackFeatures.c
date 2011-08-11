/*********************************************************************
 * trackFeatures.c
 *
 *********************************************************************/

/* Standard includes */
#include <math.h>		/* fabs() */
#include <stdlib.h>		/* malloc() */
#include <stdio.h>		/* fflush() */

/* Our includes */
#include "base.h"
#include "error.h"
#include "convolve.h"	/* for computing pyramid */
#include "klt.h"
#include "klt_util.h"	/* _KLT_FloatImage */
#include "pyramid.h"	/* _KLT_Pyramid */

typedef float *_FloatWindow;

/*********************************************************************
 * _interpolate
 * 
 * Given a point (x,y) in an image, computes the bilinear interpolated 
 * gray-level value of the point in the image.  
 */

float _interpolate(float x, float y, _KLT_FloatImage img) {

    int xt = x;
    int yt = y;

    float ax = x - xt;
    float ay = y - yt;

    float *ptr = img->data + (img->ncols * yt) + xt;

    return (
        (1 - ax) * (1 - ay) * (*ptr) +
        ax * (1 - ay) * (*(ptr + 1)) +
        (1 - ax) * ay * (*(ptr + img->ncols)) +
        ax * ay * (*(ptr + img->ncols + 1))
        );
}

/*********************************************************************
 * _computeIntensityDifference
 *
 * Given two images and the window center in both images,
 * aligns the images wrt the window and computes the difference 
 * between the two overlaid images.
 */

void _computeIntensityDifference(
  _KLT_FloatImage img1,   /* images */
  _KLT_FloatImage img2,
  float x1, float y1,     /* center of window in 1st img */
  float x2, float y2,     /* center of window in 2nd img */
  int width, int height,  /* size of window */
  _FloatWindow imgdiff)   /* output */
{
  int hw = width/2, hh = height/2;
  float g1, g2;
  int i, j;

  /* Compute values */
  for (j = -hh ; j <= hh ; j++)
    for (i = -hw ; i <= hw ; i++)  {
      g1 = _interpolate(x1+i, y1+j, img1);
      g2 = _interpolate(x2+i, y2+j, img2);
      *imgdiff++ = g1 - g2;
    }
}


/*********************************************************************
 * _computeGradientSum
 *
 * Given two gradients and the window center in both images,
 * aligns the gradients wrt the window and computes the sum of the two 
 * overlaid gradients.
 */

void _computeGradientSum(
  _KLT_FloatImage gradx1,  /* gradient images */
  _KLT_FloatImage grady1,
  _KLT_FloatImage gradx2,
  _KLT_FloatImage grady2,
  float x1, float y1,      /* center of window in 1st img */
  float x2, float y2,      /* center of window in 2nd img */
  int width, int height,   /* size of window */
  _FloatWindow gradx,      /* output */
  _FloatWindow grady)      /*   " */
{
  int hw = width/2, hh = height/2;
  float g1, g2;
  int i, j;

  /* Compute values */
  for (j = -hh ; j <= hh ; j++)
    for (i = -hw ; i <= hw ; i++)  {
      g1 = _interpolate(x1+i, y1+j, gradx1);
      g2 = _interpolate(x2+i, y2+j, gradx2);
      *gradx++ = g1 + g2;
      g1 = _interpolate(x1+i, y1+j, grady1);
      g2 = _interpolate(x2+i, y2+j, grady2);
      *grady++ = g1 + g2;
    }
}

/*********************************************************************
 * _compute2by2GradientMatrix
 *
 */

void _compute2by2GradientMatrix(
  _FloatWindow gradx,
  _FloatWindow grady,
  int width,   /* size of window */
  int height,
  float *gxx,  /* return values */
  float *gxy, 
  float *gyy) 

{
  float gx, gy;
  int i;

  /* Compute values */
  *gxx = 0.0;  *gxy = 0.0;  *gyy = 0.0;
  for (i = 0 ; i < width * height ; i++)  {
    gx = *gradx++;
    gy = *grady++;
    *gxx += gx*gx;
    *gxy += gx*gy;
    *gyy += gy*gy;
  }
}
	
	
/*********************************************************************
 * _compute2by1ErrorVector
 *
 */

void _compute2by1ErrorVector(
  _FloatWindow imgdiff,
  _FloatWindow gradx,
  _FloatWindow grady,
  int width,   /* size of window */
  int height,
  float step_factor, /* 2.0 comes from equations, 1.0 seems to avoid overshooting */
  float *ex,   /* return values */
  float *ey)
{
  float diff;
  int i;

  /* Compute values */
  *ex = 0;  *ey = 0;  
  for (i = 0 ; i < width * height ; i++)  {
    diff = *imgdiff++;
    *ex += diff * (*gradx++);
    *ey += diff * (*grady++);
  }
  *ex *= step_factor;
  *ey *= step_factor;
}


/*********************************************************************
 * _solveEquation
 *
 * Solves the 2x2 matrix equation
 *         [gxx gxy] [dx] = [ex]
 *         [gxy gyy] [dy] = [ey]
 * for dx and dy.
 *
 * Returns KLT_TRACKED on success and KLT_SMALL_DET on failure
 */

int _solveEquation(
  float gxx, float gxy, float gyy,
  float ex, float ey,
  float small,
  float *dx, float *dy)
{
  float det = gxx*gyy - gxy*gxy;

	
  if (det < small)  return KLT_SMALL_DET;

  *dx = (gyy*ex - gxy*ey)/det;
  *dy = (gxx*ey - gxy*ex)/det;
  return KLT_TRACKED;
}


/*********************************************************************
 * _allocateFloatWindow
 */
	
_FloatWindow _allocateFloatWindow(int width, int height) {

  return (_FloatWindow)malloc(width * height * sizeof(float));
}

/*********************************************************************
 * _sumAbsFloatWindow
 */

float _sumAbsFloatWindow(
  _FloatWindow fw,
  int width,
  int height)
{
  float sum = 0.0;
  int w;

  for ( ; height > 0 ; height--)
    for (w=0 ; w < width ; w++)
      sum += (float) fabs(*fw++);

  return sum;
}


/*********************************************************************
 * _trackFeature
 *
 * Tracks a feature point from one image to the next.
 *
 * RETURNS
 * KLT_SMALL_DET if feature is lost,
 * KLT_MAX_ITERATIONS if tracking stopped because iterations timed out,
 * KLT_TRACKED otherwise.
 */

int _trackFeature(
  float x1,  /* location of window in first image */
  float y1,
  float *x2, /* starting location of search in second image */
  float *y2,
  _KLT_FloatImage img1, 
  _KLT_FloatImage gradx1,
  _KLT_FloatImage grady1,
  _KLT_FloatImage img2, 
  _KLT_FloatImage gradx2,
  _KLT_FloatImage grady2,
  int width,           /* size of window */
  int height,
  float step_factor, /* 2.0 comes from equations, 1.0 seems to avoid overshooting */
  int max_iterations,
  float small,         /* determinant threshold for declaring KLT_SMALL_DET */
  float th,            /* displacement threshold for stopping               */
  float max_residue)   /* residue threshold for declaring KLT_LARGE_RESIDUE */
{
  _FloatWindow imgdiff, gradx, grady;
  float gxx, gxy, gyy, ex, ey, dx, dy;
  int iteration = 0;
  int status;
  int hw = width / 2;
  int hh = height / 2;
  int nc = img1->ncols;
  int nr = img1->nrows;
  float one_plus_eps = 1.001f;   /* To prevent rounding errors */

	
  /* Allocate memory for windows */
  imgdiff = _allocateFloatWindow(width, height);
  gradx   = _allocateFloatWindow(width, height);
  grady   = _allocateFloatWindow(width, height);

  /* Iteratively update the window position */
  do  {

    /* If out of bounds, exit loop */
    if (  x1-hw < 0.0f || nc-( x1+hw) < one_plus_eps ||
         *x2-hw < 0.0f || nc-(*x2+hw) < one_plus_eps ||
          y1-hh < 0.0f || nr-( y1+hh) < one_plus_eps ||
         *y2-hh < 0.0f || nr-(*y2+hh) < one_plus_eps) {
      status = KLT_OOB;
      break;
    }

    /* Compute gradient and difference windows */
    _computeIntensityDifference(img1, img2, x1, y1, *x2, *y2, 
                              width, height, imgdiff);
    _computeGradientSum(gradx1, grady1, gradx2, grady2, 
	      x1, y1, *x2, *y2, width, height, gradx, grady);
		

    /* Use these windows to construct matrices */
    _compute2by2GradientMatrix(gradx, grady, width, height, 
                               &gxx, &gxy, &gyy);
    _compute2by1ErrorVector(imgdiff, gradx, grady, width, height, step_factor,
                            &ex, &ey);
				
    /* Using matrices, solve equation for new displacement */
    status = _solveEquation(gxx, gxy, gyy, ex, ey, small, &dx, &dy);
    if (status == KLT_SMALL_DET)  break;

    *x2 += dx;
    *y2 += dy;
    iteration++;

  }  while ((fabs(dx)>=th || fabs(dy)>=th) && iteration < max_iterations);

  /* Check whether window is out of bounds */
  if (*x2-hw < 0.0f || nc-(*x2+hw) < one_plus_eps || 
      *y2-hh < 0.0f || nr-(*y2+hh) < one_plus_eps)
    status = KLT_OOB;

  /* Check whether residue is too large */
  if (status == KLT_TRACKED)  {

    _computeIntensityDifference(
        img1, img2, x1, y1, *x2, *y2,
        width, height, imgdiff);

    if (_sumAbsFloatWindow(imgdiff, width, height)/(width*height) > max_residue)
      status = KLT_LARGE_RESIDUE;
  }

  /* Free memory */
  free(imgdiff);
  free(gradx);
  free(grady);

  /* Return appropriate value */
  if (status == KLT_SMALL_DET)
    return KLT_SMALL_DET;
  else if (status == KLT_OOB)
    return KLT_OOB;
  else if (status == KLT_LARGE_RESIDUE)
    return KLT_LARGE_RESIDUE;
  else if (iteration >= max_iterations)
    return KLT_MAX_ITERATIONS;
  else
    return KLT_TRACKED;

}


/*********************************************************************/

KLT_BOOL _outOfBounds(
  float x,
  float y,
  int ncols,
  int nrows,
  int borderx,
  int bordery)
{
  return (x < borderx || x > ncols-1-borderx ||
          y < bordery || y > nrows-1-bordery );
}

/*********************************************************************
 * KLTTrackFeatures
 *
 * Tracks feature points from one image to the next.
 */

void KLTTrackFeatures(
					  KLT_TrackingContext tc,
					  KLT_PixelType *img1,
					  KLT_PixelType *img2,
					  int ncols,
					  int nrows,
					  KLT_FeatureList featurelist)
{
	_KLT_FloatImage tmpimg = NULL;
	_KLT_FloatImage floatimg1 = NULL;
	_KLT_FloatImage floatimg2 = NULL;

	_KLT_Pyramid pyramid1, pyramid1_gradx, pyramid1_grady;
	_KLT_Pyramid pyramid2, pyramid2_gradx, pyramid2_grady;

	int val = 0;
	KLT_BOOL floatimg1_created = FALSE;

	float subsampling = (float)tc->subsampling;
	float xloc, yloc, xlocout, ylocout;
	int i, indx, r;

	if (tc->verbose >= 1)  {
		fprintf(stderr, "(KLT) Tracking %d features in a %d by %d image...  ",
			KLTCountRemainingFeatures(featurelist), ncols, nrows);
		fflush(stderr);
	}

	/* Check window size (and correct if necessary) */
	if (tc->window_width % 2 != 1) {
		tc->window_width = tc->window_width+1;
		KLTWarning("Tracking context's window width must be odd.  "
			"Changing to %d.\n", tc->window_width);
	}
	if (tc->window_height % 2 != 1) {
		tc->window_height = tc->window_height+1;
		KLTWarning("Tracking context's window height must be odd.  "
			"Changing to %d.\n", tc->window_height);
	}
	if (tc->window_width < 3) {
		tc->window_width = 3;
		KLTWarning("Tracking context's window width must be at least three.  \n"
			"Changing to %d.\n", tc->window_width);
	}
	if (tc->window_height < 3) {
		tc->window_height = 3;
		KLTWarning("Tracking context's window height must be at least three.  \n"
			"Changing to %d.\n", tc->window_height);
	}

	/* Create temporary image */
	tmpimg = _KLTCreateFloatImage(ncols, nrows);

	/* Process first image by converting to float, smoothing, computing */
	/* pyramid, and computing gradient pyramids */
	if (tc->sequentialMode && tc->pyramid_last != NULL)  {
		pyramid1 = (_KLT_Pyramid) tc->pyramid_last;
		pyramid1_gradx = (_KLT_Pyramid) tc->pyramid_last_gradx;
		pyramid1_grady = (_KLT_Pyramid) tc->pyramid_last_grady;
		if (pyramid1->ncols[0] != ncols || pyramid1->nrows[0] != nrows)
			KLTError("(KLTTrackFeatures) Size of incoming image (%d by %d) "
			"is different from size of previous image (%d by %d)\n",
			ncols, nrows, pyramid1->ncols[0], pyramid1->nrows[0]);
	} else  {
		floatimg1_created = TRUE;
		floatimg1 = _KLTCreateFloatImage(ncols, nrows);
		_KLTToFloatImage(img1, ncols, nrows, tmpimg);
		_KLTComputeSmoothedImage(tmpimg, _KLTComputeSmoothSigma(tc), floatimg1);
		pyramid1 = _KLTCreatePyramid(ncols, nrows, (int) subsampling, tc->nPyramidLevels);
		_KLTComputePyramid(floatimg1, pyramid1, tc->pyramid_sigma_fact);
		pyramid1_gradx = _KLTCreatePyramid(ncols, nrows, (int) subsampling, tc->nPyramidLevels);
		pyramid1_grady = _KLTCreatePyramid(ncols, nrows, (int) subsampling, tc->nPyramidLevels);
		for (i = 0 ; i < tc->nPyramidLevels ; i++)
			_KLTComputeGradients(pyramid1->img[i], tc->grad_sigma, 
			pyramid1_gradx->img[i],
			pyramid1_grady->img[i]);
	}

	/* Do the same thing with second image */
	floatimg2 = _KLTCreateFloatImage(ncols, nrows);
	_KLTToFloatImage(img2, ncols, nrows, tmpimg);
	_KLTComputeSmoothedImage(tmpimg, _KLTComputeSmoothSigma(tc), floatimg2);
	pyramid2 = _KLTCreatePyramid(ncols, nrows, (int) subsampling, tc->nPyramidLevels);
	_KLTComputePyramid(floatimg2, pyramid2, tc->pyramid_sigma_fact);
	pyramid2_gradx = _KLTCreatePyramid(ncols, nrows, (int) subsampling, tc->nPyramidLevels);
	pyramid2_grady = _KLTCreatePyramid(ncols, nrows, (int) subsampling, tc->nPyramidLevels);
	for (i = 0 ; i < tc->nPyramidLevels ; i++)
		_KLTComputeGradients(pyramid2->img[i], tc->grad_sigma, 
		pyramid2_gradx->img[i],
		pyramid2_grady->img[i]);

	/* For each feature, do ... */
	for (indx = 0 ; indx < featurelist->nFeatures ; indx++)  {

		/* Only track features that are not lost */
		if (featurelist->feature[indx]->val >= 0)  {

			xloc = featurelist->feature[indx]->x;
			yloc = featurelist->feature[indx]->y;

			/* Transform location to coarsest resolution */
			for (r = tc->nPyramidLevels - 1 ; r >= 0 ; r--)  {
				xloc /= subsampling;  yloc /= subsampling;
			}
			xlocout = xloc;  ylocout = yloc;

			/* Beginning with coarsest resolution, do ... */
			for (r = tc->nPyramidLevels - 1 ; r >= 0 ; r--)  {

				/* Track feature at current resolution */
				xloc *= subsampling;  yloc *= subsampling;
				xlocout *= subsampling;  ylocout *= subsampling;

				val = _trackFeature(xloc, yloc, 
					&xlocout, &ylocout,
					pyramid1->img[r], 
					pyramid1_gradx->img[r], pyramid1_grady->img[r], 
					pyramid2->img[r], 
					pyramid2_gradx->img[r], pyramid2_grady->img[r],
					tc->window_width, tc->window_height,
					tc->step_factor,
					tc->max_iterations,
					tc->min_determinant,
					tc->min_displacement,
					tc->max_residue);

				if (val==KLT_SMALL_DET || val==KLT_OOB)
					break;
			}

			/* Record feature */
			if (val == KLT_OOB) {
				featurelist->feature[indx]->x   = -1.0;
				featurelist->feature[indx]->y   = -1.0;
				featurelist->feature[indx]->val = KLT_OOB;
			} else if (_outOfBounds(xlocout, ylocout, ncols, nrows, tc->borderx, tc->bordery))  {
				featurelist->feature[indx]->x   = -1.0;
				featurelist->feature[indx]->y   = -1.0;
				featurelist->feature[indx]->val = KLT_OOB;
			} else if (val == KLT_SMALL_DET)  {
				featurelist->feature[indx]->x   = -1.0;
				featurelist->feature[indx]->y   = -1.0;
				featurelist->feature[indx]->val = KLT_SMALL_DET;
			} else if (val == KLT_LARGE_RESIDUE)  {
				featurelist->feature[indx]->x   = -1.0;
				featurelist->feature[indx]->y   = -1.0;
				featurelist->feature[indx]->val = KLT_LARGE_RESIDUE;
			} else if (val == KLT_MAX_ITERATIONS)  {
				featurelist->feature[indx]->x   = -1.0;
				featurelist->feature[indx]->y   = -1.0;
				featurelist->feature[indx]->val = KLT_MAX_ITERATIONS;
			} else {
				featurelist->feature[indx]->x = xlocout;
				featurelist->feature[indx]->y = ylocout;
				featurelist->feature[indx]->val = KLT_TRACKED;
			}
		}
	}

	if (tc->sequentialMode)  {
		tc->pyramid_last = pyramid2;
		tc->pyramid_last_gradx = pyramid2_gradx;
		tc->pyramid_last_grady = pyramid2_grady;
	} else  {
		_KLTFreePyramid(pyramid2);
		_KLTFreePyramid(pyramid2_gradx);
		_KLTFreePyramid(pyramid2_grady);
	}

	/* Free memory */
	_KLTFreeFloatImage(tmpimg);
	if (floatimg1_created)  _KLTFreeFloatImage(floatimg1);
	_KLTFreeFloatImage(floatimg2);
	_KLTFreePyramid(pyramid1);
	_KLTFreePyramid(pyramid1_gradx);
	_KLTFreePyramid(pyramid1_grady);

	if (tc->verbose >= 1)
		fprintf(
		    stderr,
		    "\n\t%d features successfully tracked.\n",
			KLTCountRemainingFeatures(featurelist)
			);
}

