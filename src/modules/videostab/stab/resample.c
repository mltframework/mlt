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

#include "resample.h"
#include "utils.h"

rs_ctx *rs_init(int nc, int nr) {

    rs_ctx *rs = (rs_ctx *)malloc(sizeof(rs_ctx));

    rs->tf = (unsigned char *)malloc(nc * nr * 3 * sizeof(unsigned char));

    rs->nc = nc;
    rs->nr = nr;

    return rs;
}

void rs_resample(int* lanc_kernels,rs_ctx *rs, unsigned char *f, vc *p) {

    int i, x, y, c;

    for (y = 0; y < rs->nr; y ++) {

        int yp = y * rs->nc;
        int xd = floor(p[y].x);

        int *lk = select_lanc_kernel(lanc_kernels,p[y].x);

        for (x = 0; x < rs->nc; x ++) {

            int pd = (yp + x) * 3;
            int a[3];

            for (c = 0; c < 3; c ++)
                a[c] = 0;

            for (i = -3; i < 5; i ++) {

                int ps = (yp + clamp(x + xd + i, 0, rs->nc - 1)) * 3;

                for (c = 0; c < 3; c ++)
                    a[c] += f[ps + c] * lk[i + 3];
            }

            for (c = 0; c < 3; c ++)
                rs->tf[pd + c] = clamp(a[c] / 1024, 0, 255);
        }
    }

    for (y = 0; y < rs->nr; y ++) {

        int yp = y * rs->nc;
        int yd = floor(p[y].y);

        int *lk = select_lanc_kernel(lanc_kernels,p[y].y);

        for (x = 0; x < rs->nc; x ++) {

            int pd = (yp + x) * 3;
            int a[3];

            for (c = 0; c < 3; c ++)
                a[c] = 0;

            for (i = -3; i < 5; i ++) {

                int ps = (clamp(y + yd + i, 0, rs->nr - 1) * rs->nc + x) * 3;

                for (c = 0; c < 3; c ++)
                    a[c] += rs->tf[ps + c] * lk[i + 3];
            }

            for (c = 0; c < 3; c ++)
                f[pd + c] = clamp(a[c] / 1024, 0, 255);
        }
    }
}

void rs_free(rs_ctx *rs) {

    free(rs->tf);

    free(rs);
}

