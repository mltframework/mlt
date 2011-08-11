#ifndef VECTOR_H
#define VECTOR_H

typedef struct {

    float x, y;

} vc;

vc vc_zero();
vc vc_one();
vc vc_set(float, float);

vc vc_add(vc, vc);
vc vc_sub(vc, vc);

vc vc_mul(vc, float);
vc vc_div(vc, float);

void vc_acc(vc *, vc);
void vc_mul_acc(vc *, vc, float);

float vc_len(vc);
float vc_ang(vc, vc);

#endif

