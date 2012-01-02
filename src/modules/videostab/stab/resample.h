#ifndef RESAMPLE_H
#define RESAMPLE_H

#include "vector.h"

typedef struct {

    unsigned char *tf;
    int nc, nr;

} rs_ctx;

rs_ctx *rs_init(int, int);

void rs_resample(int*,rs_ctx *, unsigned char *, vc *);

void rs_free(rs_ctx *);

#endif

