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
#include <string.h>
#include <math.h>

#include "utils.h"

float lanc(float x, float r) {

    float t = x * M_PI;

    if (x == 0.0)
        return 1.0;

    if (x <= -r || x >= r)
        return 0.0;

    return r * sin(t) * sin(t / r) / (t * t);
}

float hann(float x, float d) {

    if (x < 0.0 || x > d)
        return 0.0;

    return 0.5 * (1.0 - cos((M_PI * 2.0 * x) / (d - 1.0)));
}

int clamp(int a, int b, int c) {

    if (a < b)
        a = b;

    if (a > c)
        a = c;

    return a;
}

void lopass(vc *vi, vc *vo, int l, int r) {

    int d = r * 2 + 1;
    float *ck = (float *)malloc(d * sizeof(float));
    float cw = 0.0;

    int i, j;

    for (i = 0; i < d; i ++)
        cw += ck[i] = hann(i, d - 1);

    for (i = 0; i < l; i ++) {

        vc a = vc_zero();

        for (j = i - r; j <= i + r; j ++) {

            int jc = clamp(j, 0, l - 1);

            vc_mul_acc(&a, vi[jc], ck[j - i + r]);
        }

        vo[i] = vc_div(a, cw);
    }

    free(ck);
}

void hipass(vc *vi, vc *vo, int l, int r) {

    int i;

    lopass(vi, vo, l, r);

    for (i = 0; i < l; i ++)
        vo[i] = vc_sub(vi[i], vo[i]);
}

int* prepare_lanc_kernels() {

    int i, j;

    int* lanc_kernels = (int *)malloc(256 * 8 * sizeof(int));

    for (i = 0; i < 256; i ++)
        for (j = -3; j < 5; j ++)
            lanc_kernels[i * 8 + j + 3] = lanc(j - i / 256.0, 4) * 1024.0;
    return lanc_kernels;
}

int *select_lanc_kernel(int* lanc_kernels,float x) {

    return lanc_kernels + (int)((x - floor(x)) * 256.0) * 8;
}

void free_lanc_kernels(int *lanc_kernels) {

    free(lanc_kernels);
}

vc interp(int* lanc_kernels, vc *vi, int l, float x) {

    vc a = vc_zero();
    int xd = floor(x);
    int *lk = select_lanc_kernel(lanc_kernels,x);

    int i;

    for (i = -3; i < 5; i ++) {

        int ic = clamp(xd + i, 0, l - 1);

        vc_mul_acc(&a, vi[ic], lk[i + 3]);
    }

    return vc_div(a, 1024.0);
}

