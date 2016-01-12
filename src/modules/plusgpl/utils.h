/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * utils.h: header file for utils
 *
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <inttypes.h>

typedef uint32_t RGB32;

/* DEFINE's by nullset@dookie.net */
#define RED(n)  ((n>>16) & 0x000000FF)
#define GREEN(n) ((n>>8) & 0x000000FF)
#define BLUE(n)  ((n>>0) & 0x000000FF)
#define RGB(r,g,b) ((0<<24) + (r<<16) + (g <<8) + (b))
#define INTENSITY(n)	( ( (RED(n)+GREEN(n)+BLUE(n))/3))

/* utils.c */
void HSItoRGB(double H, double S, double I, int *r, int *g, int *b);

#ifndef __APPLE__
extern unsigned int fastrand_val;
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)
#endif
unsigned int fastrand(void);
void fastsrand(unsigned int);

/* image.c */
int image_set_threshold_y(int threshold);
void image_bgset_y(RGB32 *background, const RGB32 *src, int video_area, int y_threshold);
void image_bgsubtract_y(unsigned char *diff, const RGB32 *background, const RGB32 *src, int video_area, int y_threshold);
void image_bgsubtract_update_y(unsigned char *diff, RGB32 *background, const RGB32 *src, int video_area, int y_threshold);
RGB32 image_set_threshold_RGB(int r, int g, int b);
void image_bgset_RGB(RGB32 *background, const RGB32 *src, int video_area);
void image_bgsubtract_RGB(unsigned char *diff, const RGB32 *background, const RGB32 *src, int video_area, RGB32 rgb_threshold);
void image_bgsubtract_update_RGB(unsigned char *diff, RGB32 *background, const RGB32 *src, int video_area, RGB32 rgb_threshold);
void image_diff_filter(unsigned char *diff2, const unsigned char *diff, int width, int height);
void image_y_over(unsigned char *diff, const RGB32 *src, int video_area, int y_threshold);
void image_y_under(unsigned char *diff, const RGB32 *src, int video_area, int y_threshold);
void image_edge(unsigned char *diff2, const RGB32 *src, int width, int height, int y_threshold);
void image_hflip(const RGB32 *src, RGB32 *dest, int width, int height);

#endif /* __UTILS_H__ */
