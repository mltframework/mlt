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

#include <math.h>

#include "vector.h"

vc vc_zero() {

    vc v3;

    v3.x = 0.0;
    v3.y = 0.0;
    
    return v3;
}

vc vc_one() {

    vc v3;

    v3.x = 1.0;
    v3.y = 1.0;
    
    return v3;
}

vc vc_set(float x, float y) {

    vc v3;

    v3.x = x;
    v3.y = y;
    
    return v3;
}

vc vc_add(vc v1, vc v2) {
    
    vc v3;

    v3.x = v1.x + v2.x;
    v3.y = v1.y + v2.y;
    
    return v3;
}

vc vc_sub(vc v1, vc v2) {
    
    vc v3;

    v3.x = v1.x - v2.x;
    v3.y = v1.y - v2.y;
    
    return v3;
}

vc vc_mul(vc v1, float v2) {
    
    vc v3;

    v3.x = v1.x * v2;
    v3.y = v1.y * v2;
    
    return v3;
}

vc vc_div(vc v1, float v2) {
    
    vc v3;

    v3.x = v1.x / v2;
    v3.y = v1.y / v2;
    
    return v3;
}

void vc_acc(vc *v1, vc v2) {
    
    v1->x += v2.x;
    v1->y += v2.y;
}

void vc_mul_acc(vc *v1, vc v2, float v3) {
    
    v1->x += v2.x * v3;
    v1->y += v2.y * v3;
}

float vc_len(vc v) {

    return sqrt(v.x * v.x + v.y * v.y);
}

float vc_ang(vc v1, vc v2) {

    float c = v1.x * v2.y - v1.y * v2.x;
    
    if (fabs(c) > 0.0) {

        float l = vc_len(v1) * vc_len(v2);
        float d = acos((v1.x * v2.x + v1.y * v2.y) / l);

        if (c > 0.0)
            return d;
        else
            return -d;
    }

    return 0.0;    
}

