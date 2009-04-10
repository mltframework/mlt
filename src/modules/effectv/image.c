/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * image.c: utilities for image processing.
 *
 */

#include <string.h>
#include <stdlib.h>
#include "utils.h"


/*
 * Collection of background subtraction functions
 */

/* checks only fake-Y value */
/* In these function Y value is treated as R*2+G*4+B. */

int image_set_threshold_y(int threshold)
{
	int y_threshold = threshold * 7; /* fake-Y value is timed by 7 */

	return y_threshold;
}

void image_bgset_y(RGB32 *background, const RGB32 *src, int video_area, int y_threshold)
{
	int i;
	int R, G, B;
	const RGB32 *p;
	short *q;

	p = src;
	q = (short *)background;
	for(i=0; i<video_area; i++) {
		/* FIXME: endianess */

		R = ((*p)&0xff0000)>>(16-1);
		G = ((*p)&0xff00)>>(8-2);
		B = (*p)&0xff;
		*q = (short)(R + G + B);
		p++;
		q++;
	}
}

void image_bgsubtract_y(unsigned char *diff, const RGB32 *background, const RGB32 *src, int video_area, int y_threshold)
{
	int i;
	int R, G, B;
	const RGB32 *p;
	const short *q;
	unsigned char *r;
	int v;

	p = src;
	q = (const short *)background;
	r = diff;
	for(i=0; i<video_area; i++) {
		/* FIXME: endianess */

		R = ((*p)&0xff0000)>>(16-1);
		G = ((*p)&0xff00)>>(8-2);
		B = (*p)&0xff;
		v = (R + G + B) - (int)(*q);
		*r = ((v + y_threshold)>>24) | ((y_threshold - v)>>24);

		p++;
		q++;
		r++;
	}

/* The origin of subtraction function is;
 * diff(src, dest) = (abs(src - dest) > threshold) ? 0xff : 0;
 *
 * This functions is transformed to;
 * (threshold > (src - dest) > -threshold) ? 0 : 0xff;
 *
 * (v + threshold)>>24 is 0xff when v is less than -threshold.
 * (v - threshold)>>24 is 0xff when v is less than threshold.
 * So, ((v + threshold)>>24) | ((threshold - v)>>24) will become 0xff when
 * abs(src - dest) > threshold.
 */
}

/* Background image is refreshed every frame */
void image_bgsubtract_update_y(unsigned char *diff, RGB32 *background, const RGB32 *src, int video_area, int y_threshold)
{
	int i;
	int R, G, B;
	const RGB32 *p;
	short *q;
	unsigned char *r;
	int v;

	p = src;
	q = (short *)background;
	r = diff;
	for(i=0; i<video_area; i++) {
		/* FIXME: endianess */

		R = ((*p)&0xff0000)>>(16-1);
		G = ((*p)&0xff00)>>(8-2);
		B = (*p)&0xff;
		v = (R + G + B) - (int)(*q);
		*q = (short)(R + G + B);
		*r = ((v + y_threshold)>>24) | ((y_threshold - v)>>24);

		p++;
		q++;
		r++;
	}
}

/* checks each RGB value */

/* The range of r, g, b are [0..7] */
RGB32 image_set_threshold_RGB(int r, int g, int b)
{
	unsigned char R, G, B;
	RGB32 rgb_threshold;

	R = G = B = 0xff;
	R = R<<r;
	G = G<<g;
	B = B<<b;
	rgb_threshold = (RGB32)(R<<16 | G<<8 | B);

	return rgb_threshold;
}

void image_bgset_RGB(RGB32 *background, const RGB32 *src, int video_area)
{
	int i;
	RGB32 *p;

	p = background;
	for(i=0; i<video_area; i++) {
		*p++ = (*src++) & 0xfefefe;
	}
}

void image_bgsubtract_RGB(unsigned char *diff, const RGB32 *background, const RGB32 *src, int video_area, RGB32 rgb_threshold)
{
	int i;
	const RGB32 *p, *q;
	unsigned a, b;
	unsigned char *r;

	p = src;
	q = background;
	r = diff;
	for(i=0; i<video_area; i++) {
		a = (*p++)|0x1010100;
		b = *q++;
		a = a - b;
		b = a & 0x1010100;
		b = b - (b>>8);
		b = b ^ 0xffffff;
		a = a ^ b;
		a = a & rgb_threshold;
		*r++ = (0 - a)>>24;
	}
}

void image_bgsubtract_update_RGB(unsigned char *diff, RGB32 *background, const RGB32 *src, int video_area, RGB32 rgb_threshold)
{
	int i;
	const RGB32 *p;
	RGB32 *q;
	unsigned a, b;
	unsigned char *r;

	p = src;
	q = background;
	r = diff;
	for(i=0; i<video_area; i++) {
		a = *p|0x1010100;
		b = *q&0xfefefe;
		*q++ = *p++;
		a = a - b;
		b = a & 0x1010100;
		b = b - (b>>8);
		b = b ^ 0xffffff;
		a = a ^ b;
		a = a & rgb_threshold;
		*r++ = (0 - a)>>24;
	}
}

/* noise filter for subtracted image. */
void image_diff_filter(unsigned char *diff2, const unsigned char *diff, int width, int height)
{
	int x, y;
	const unsigned char *src;
	unsigned char *dest;
	unsigned int count;
	unsigned int sum1, sum2, sum3;

	src = diff;
	dest = diff2 + width +1;
	for(y=1; y<height-1; y++) {
		sum1 = src[0] + src[width] + src[width*2];
		sum2 = src[1] + src[width+1] + src[width*2+1];
		src += 2;
		for(x=1; x<width-1; x++) {
			sum3 = src[0] + src[width] + src[width*2];
			count = sum1 + sum2 + sum3;
			sum1 = sum2;
			sum2 = sum3;
			*dest++ = (0xff*3 - count)>>24;
			src++;
		}
		dest += 2;
	}
}

/* Y value filters */
void image_y_over(unsigned char *diff, const RGB32 *src, int video_area, int y_threshold)
{
	int i;
	int R, G, B, v;
	unsigned char *p = diff;

	for(i = video_area; i>0; i--) {
		R = ((*src)&0xff0000)>>(16-1);
		G = ((*src)&0xff00)>>(8-2);
		B = (*src)&0xff;
		v = y_threshold - (R + G + B);
		*p = (unsigned char)(v>>24);
		src++;
		p++;
	}
}

void image_y_under(unsigned char *diff, const RGB32 *src, int video_area, int y_threshold)
{
	int i;
	int R, G, B, v;
	unsigned char *p = diff;

	for(i = video_area; i>0; i--) {
		R = ((*src)&0xff0000)>>(16-1);
		G = ((*src)&0xff00)>>(8-2);
		B = (*src)&0xff;
		v = (R + G + B) - y_threshold;
		*p = (unsigned char)(v>>24);
		src++;
		p++;
	}
}

/* tiny edge detection */
void image_edge(unsigned char *diff2, const RGB32 *src, int width, int height, int y_threshold)
{
	int x, y;
	const unsigned char *p;
	unsigned char *q;
	int r, g, b;
	int ar, ag, ab;
	int w;

	p = (const unsigned char *)src;
	q = diff2;
	w = width * sizeof(RGB32);

	for(y=0; y<height - 1; y++) {
		for(x=0; x<width - 1; x++) {
			b = p[0];
			g = p[1];
			r = p[2];
			ab = abs(b - p[4]);
			ag = abs(g - p[5]);
			ar = abs(r - p[6]);
			ab += abs(b - p[w]);
			ag += abs(g - p[w+1]);
			ar += abs(r - p[w+2]);
			b = ab+ag+ar;
			if(b > y_threshold) {
				*q = 255;
			} else {
				*q = 0;
			}
			q++;
			p += 4;
		}
		p += 4;
		*q++ = 0;
	}
	memset(q, 0, width);
}

/* horizontal flipping */
void image_hflip(const RGB32 *src, RGB32 *dest, int width, int height)
{
	int x, y;

	src += width - 1;
	for(y=0; y<height; y++) {
		for(x=0; x<width; x++) {
			*dest++ = *src--;
		}
		src += width * 2;
	}
}

