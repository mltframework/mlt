#ifndef ESTIMATE_H
#define ESTIMATE_H

#include "klt/klt.h"

#include "vector.h"

typedef struct {

    KLT_TrackingContext tc;
    KLT_PixelType *fr[2];
    KLT_FeatureList fl;

    vc *dv;
    int nv;

    int nc, nr;

    int ff;

} es_ctx;

es_ctx *es_init(int, int);

vc es_estimate(es_ctx *, unsigned char *);

void es_free(es_ctx *);

#endif

