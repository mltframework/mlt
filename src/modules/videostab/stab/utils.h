#ifndef UTILS_H
#define UTILS_H

#include "vector.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


float lanc(float, float);
float hann(float, float);

int clamp(int, int, int);

void lopass(vc *, vc *, int, int);
void hipass(vc *, vc *, int, int);

int* prepare_lanc_kernels();
int *select_lanc_kernel(int*,float);
void free_lanc_kernels(int*);

vc interp(int*,vc *, int, float);

#endif

