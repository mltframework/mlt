/*
	Yadif C-plugin for Avisynth 2.5 - Yet Another DeInterlacing Filter
	Copyright (C)2007 Alexander G. Balakhnin aka Fizick  http://avisynth.org.ru
    Port of YADIF filter from MPlayer
	Copyright (C) 2006 Michael Niedermayer <michaelni@gmx.at>

    This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Avisynth_C plugin
	Assembler optimized for GNU C compiler

*/

#ifndef YADIF_H_
#define YADIF_H_

#include <stdint.h>

#define AVS_CPU_INTEGER_SSE 0x1
#define AVS_CPU_SSE2 0x2
#define AVS_CPU_SSSE3 0x4

typedef struct yadif_filter  {
	int cpu; // optimization
	int yheight;
	int ypitch;
	int uvpitch;
	int ywidth;
	int uvwidth;
	unsigned char *ysrc;
	unsigned char *usrc;
	unsigned char *vsrc;
	unsigned char *yprev;
	unsigned char *uprev;
	unsigned char *vprev;
	unsigned char *ynext;
	unsigned char *unext;
	unsigned char *vnext;
	unsigned char *ydest;
	unsigned char *udest;
	unsigned char *vdest;
} yadif_filter;

void filter_plane(int mode, uint8_t *dst, int dst_stride, const uint8_t *prev0, const uint8_t *cur0, const uint8_t *next0, int refs, int w, int h, int parity, int tff, int cpu);
void YUY2ToPlanes(const unsigned char *pSrcYUY2, int nSrcPitchYUY2, int nWidth, int nHeight,
							   unsigned char * pSrcY, int srcPitchY,
							   unsigned char * pSrcU,  unsigned char * pSrcV, int srcPitchUV, int cpu);
void YUY2FromPlanes(unsigned char *pSrcYUY2, int nSrcPitchYUY2, int nWidth, int nHeight,
							  const unsigned char * pSrcY, int srcPitchY,
							  const unsigned char * pSrcU, const unsigned char * pSrcV, int srcPitchUV, int cpu);

#endif
