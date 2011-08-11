/*********************************************************************
 * klt_util.c
 *********************************************************************/

/* Standard includes */
#include <stdlib.h>
#include <math.h>

/* Our includes */
#include "base.h"
#include "error.h"
#include "klt.h"
#include "klt_util.h"

/*********************************************************************/

float _KLTComputeSmoothSigma(
  KLT_TrackingContext tc)
{
  return (tc->smooth_sigma_fact * max(tc->window_width, tc->window_height));
}


/*********************************************************************
 * _KLTCreateFloatImage
 */

_KLT_FloatImage _KLTCreateFloatImage(int ncols, int nrows) {

  int nbytes = sizeof(_KLT_FloatImageRec) + ncols * nrows * sizeof(float);

  _KLT_FloatImage floatimg;

  floatimg = (_KLT_FloatImage)malloc(nbytes);
  floatimg->ncols = ncols;
  floatimg->nrows = nrows;
  floatimg->data = (float *)(floatimg + 1);

  return(floatimg);
}


/*********************************************************************
 * _KLTFreeFloatImage
 */

void _KLTFreeFloatImage(
  _KLT_FloatImage floatimg)
{
  free(floatimg);
}

