/*
 * Video stabilizer
 *
 * Copyright (c) 2008 Lenny <leonardo.masoni@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(_WIN32) && !defined(__NetBSD__)
#include <values.h>
#endif

#include "estimate.h"
#include "vector.h"
#include "utils.h"

#if !defined(MAXFLOAT)
#define MAXFLOAT HUGE_VAL
#endif

es_ctx *es_init(int nc, int nr) {

    es_ctx *es = (es_ctx *)malloc(sizeof(es_ctx));

    es->tc = KLTCreateTrackingContext();

    es->tc->sequentialMode = TRUE;
    es->tc->min_eigenvalue = 8;
    es->tc->verbose = FALSE;

    KLTChangeTCPyramid(es->tc, 31);

    KLTUpdateTCBorder(es->tc);

    es->fr[0] = (KLT_PixelType *)malloc(nc * nr * sizeof(KLT_PixelType));
    es->fr[1] = (KLT_PixelType *)malloc(nc * nr * sizeof(KLT_PixelType));

    es->fl = KLTCreateFeatureList(64);

    es->dv = (vc *)malloc(64 * sizeof(vc));
    es->nv = 0;

    es->nc = nc;
    es->nr = nr;
    
    es->ff = FALSE;

    return es;
}

vc es_estimate(es_ctx *es, unsigned char *fr) {

    KLT_PixelType *t;
    int is, id;

    t = es->fr[0]; es->fr[0] = es->fr[1]; es->fr[1] = t;

    for (is = 0, id = 0; id < es->nc * es->nr; is += 3, id ++)
        es->fr[1][id] = (fr[is + 0] * 30 + fr[is + 1] * 59 + fr[is + 2] * 11) / 100;

    if (es->ff == FALSE) {

        es->ff = TRUE;

    } else {

        vc bv = vc_set(0.0, 0.0);
        float be = MAXFLOAT;

        int i, i2;

        KLTSelectGoodFeatures(
            es->tc, es->fr[0], es->nc, es->nr, es->fl
            );
            
        for (i = 0; i < es->fl->nFeatures; i ++)
            es->dv[i] = vc_set(es->fl->feature[i]->x, es->fl->feature[i]->y);

        KLTTrackFeatures(
            es->tc, es->fr[0], es->fr[1], es->nc, es->nr, es->fl
            );

        es->nv = 0;

        for (i = 0; i < es->fl->nFeatures; i ++) {

            if (es->fl->feature[i]->val == KLT_TRACKED) {
            
                es->dv[es->nv] = vc_set(
                    es->fl->feature[i]->x - es->dv[i].x,
                    es->fl->feature[i]->y - es->dv[i].y
                    );            

                es->nv ++;
            }
        }

        for (i = 0; i < es->nv; i ++) {

            float ce = 0.0;

            for (i2 = 0; i2 < es->nv; i2 ++)
                ce += vc_len(vc_sub(es->dv[i2], es->dv[i]));
            
            if (ce < be) {
            
                bv = es->dv[i];
                be = ce;
            }
        }

        return bv;
    }
    
    return vc_zero();
}

void es_free(es_ctx *es) {

    free(es->dv);

    free(es->fr[0]);
    free(es->fr[1]);
    
    KLTFreeFeatureList(es->fl);
    KLTFreeTrackingContext(es->tc);
    
    free(es);
}

