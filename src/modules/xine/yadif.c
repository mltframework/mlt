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
#include "yadif.h"
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>

#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define ABS(a) ((a) > 0 ? (a) : (-(a)))

#define MIN3(a,b,c) MIN(MIN(a,b),c)
#define MAX3(a,b,c) MAX(MAX(a,b),c)

static void (*filter_line)(int mode, uint8_t *dst, const uint8_t *prev, const uint8_t *cur, const uint8_t *next, int w, int refs, int parity);

#if defined(__GNUC__) && defined(USE_SSE)

#define LOAD4(mem,dst) \
            "movd      "mem", "#dst" \n\t"\
            "punpcklbw %%mm7, "#dst" \n\t"

#define PABS(tmp,dst) \
            "pxor     "#tmp", "#tmp" \n\t"\
            "psubw    "#dst", "#tmp" \n\t"\
            "pmaxsw   "#tmp", "#dst" \n\t"

#define CHECK(pj,mj) \
            "movq "#pj"(%[cur],%[mrefs]), %%mm2 \n\t" /* cur[x-refs-1+j] */\
            "movq "#mj"(%[cur],%[prefs]), %%mm3 \n\t" /* cur[x+refs-1-j] */\
            "movq      %%mm2, %%mm4 \n\t"\
            "movq      %%mm2, %%mm5 \n\t"\
            "pxor      %%mm3, %%mm4 \n\t"\
            "pavgb     %%mm3, %%mm5 \n\t"\
            "pand     %[pb1], %%mm4 \n\t"\
            "psubusb   %%mm4, %%mm5 \n\t"\
            "psrlq     $8,    %%mm5 \n\t"\
            "punpcklbw %%mm7, %%mm5 \n\t" /* (cur[x-refs+j] + cur[x+refs-j])>>1 */\
            "movq      %%mm2, %%mm4 \n\t"\
            "psubusb   %%mm3, %%mm2 \n\t"\
            "psubusb   %%mm4, %%mm3 \n\t"\
            "pmaxub    %%mm3, %%mm2 \n\t"\
            "movq      %%mm2, %%mm3 \n\t"\
            "movq      %%mm2, %%mm4 \n\t" /* ABS(cur[x-refs-1+j] - cur[x+refs-1-j]) */\
            "psrlq      $8,   %%mm3 \n\t" /* ABS(cur[x-refs  +j] - cur[x+refs  -j]) */\
            "psrlq     $16,   %%mm4 \n\t" /* ABS(cur[x-refs+1+j] - cur[x+refs+1-j]) */\
            "punpcklbw %%mm7, %%mm2 \n\t"\
            "punpcklbw %%mm7, %%mm3 \n\t"\
            "punpcklbw %%mm7, %%mm4 \n\t"\
            "paddw     %%mm3, %%mm2 \n\t"\
            "paddw     %%mm4, %%mm2 \n\t" /* score */

#define CHECK1 \
            "movq      %%mm0, %%mm3 \n\t"\
            "pcmpgtw   %%mm2, %%mm3 \n\t" /* if(score < spatial_score) */\
            "pminsw    %%mm2, %%mm0 \n\t" /* spatial_score= score; */\
            "movq      %%mm3, %%mm6 \n\t"\
            "pand      %%mm3, %%mm5 \n\t"\
            "pandn     %%mm1, %%mm3 \n\t"\
            "por       %%mm5, %%mm3 \n\t"\
            "movq      %%mm3, %%mm1 \n\t" /* spatial_pred= (cur[x-refs+j] + cur[x+refs-j])>>1; */

#define CHECK2 /* pretend not to have checked dir=2 if dir=1 was bad.\
                  hurts both quality and speed, but matches the C version. */\
            "paddw    %[pw1], %%mm6 \n\t"\
            "psllw     $14,   %%mm6 \n\t"\
            "paddsw    %%mm6, %%mm2 \n\t"\
            "movq      %%mm0, %%mm3 \n\t"\
            "pcmpgtw   %%mm2, %%mm3 \n\t"\
            "pminsw    %%mm2, %%mm0 \n\t"\
            "pand      %%mm3, %%mm5 \n\t"\
            "pandn     %%mm1, %%mm3 \n\t"\
            "por       %%mm5, %%mm3 \n\t"\
            "movq      %%mm3, %%mm1 \n\t"

static void filter_line_mmx2(int mode, uint8_t *dst, const uint8_t *prev, const uint8_t *cur, const uint8_t *next, int w, int refs, int parity){
    static const uint64_t pw_1 = 0x0001000100010001ULL;
    static const uint64_t pb_1 = 0x0101010101010101ULL;
//    const int mode = p->mode;
    uint64_t tmp0, tmp1, tmp2, tmp3;
    int x;

#define FILTER\
    for(x=0; x<w; x+=4){\
        asm volatile(\
            "pxor      %%mm7, %%mm7 \n\t"\
            LOAD4("(%[cur],%[mrefs])", %%mm0) /* c = cur[x-refs] */\
            LOAD4("(%[cur],%[prefs])", %%mm1) /* e = cur[x+refs] */\
            LOAD4("(%["prev2"])", %%mm2) /* prev2[x] */\
            LOAD4("(%["next2"])", %%mm3) /* next2[x] */\
            "movq      %%mm3, %%mm4 \n\t"\
            "paddw     %%mm2, %%mm3 \n\t"\
            "psraw     $1,    %%mm3 \n\t" /* d = (prev2[x] + next2[x])>>1 */\
            "movq      %%mm0, %[tmp0] \n\t" /* c */\
            "movq      %%mm3, %[tmp1] \n\t" /* d */\
            "movq      %%mm1, %[tmp2] \n\t" /* e */\
            "psubw     %%mm4, %%mm2 \n\t"\
            PABS(      %%mm4, %%mm2) /* temporal_diff0 */\
            LOAD4("(%[prev],%[mrefs])", %%mm3) /* prev[x-refs] */\
            LOAD4("(%[prev],%[prefs])", %%mm4) /* prev[x+refs] */\
            "psubw     %%mm0, %%mm3 \n\t"\
            "psubw     %%mm1, %%mm4 \n\t"\
            PABS(      %%mm5, %%mm3)\
            PABS(      %%mm5, %%mm4)\
            "paddw     %%mm4, %%mm3 \n\t" /* temporal_diff1 */\
            "psrlw     $1,    %%mm2 \n\t"\
            "psrlw     $1,    %%mm3 \n\t"\
            "pmaxsw    %%mm3, %%mm2 \n\t"\
            LOAD4("(%[next],%[mrefs])", %%mm3) /* next[x-refs] */\
            LOAD4("(%[next],%[prefs])", %%mm4) /* next[x+refs] */\
            "psubw     %%mm0, %%mm3 \n\t"\
            "psubw     %%mm1, %%mm4 \n\t"\
            PABS(      %%mm5, %%mm3)\
            PABS(      %%mm5, %%mm4)\
            "paddw     %%mm4, %%mm3 \n\t" /* temporal_diff2 */\
            "psrlw     $1,    %%mm3 \n\t"\
            "pmaxsw    %%mm3, %%mm2 \n\t"\
            "movq      %%mm2, %[tmp3] \n\t" /* diff */\
\
            "paddw     %%mm0, %%mm1 \n\t"\
            "paddw     %%mm0, %%mm0 \n\t"\
            "psubw     %%mm1, %%mm0 \n\t"\
            "psrlw     $1,    %%mm1 \n\t" /* spatial_pred */\
            PABS(      %%mm2, %%mm0)      /* ABS(c-e) */\
\
            "movq -1(%[cur],%[mrefs]), %%mm2 \n\t" /* cur[x-refs-1] */\
            "movq -1(%[cur],%[prefs]), %%mm3 \n\t" /* cur[x+refs-1] */\
            "movq      %%mm2, %%mm4 \n\t"\
            "psubusb   %%mm3, %%mm2 \n\t"\
            "psubusb   %%mm4, %%mm3 \n\t"\
            "pmaxub    %%mm3, %%mm2 \n\t"\
            /*"pshufw $9,%%mm2, %%mm3 \n\t"*/\
            "movq %%mm2, %%mm3 \n\t" /* replace for "pshufw $9,%%mm2, %%mm3" - Fizick */\
            "psrlq $16, %%mm3 \n\t"/* replace for "pshufw $9,%%mm2, %%mm3" - Fizick*/\
            "punpcklbw %%mm7, %%mm2 \n\t" /* ABS(cur[x-refs-1] - cur[x+refs-1]) */\
            "punpcklbw %%mm7, %%mm3 \n\t" /* ABS(cur[x-refs+1] - cur[x+refs+1]) */\
            "paddw     %%mm2, %%mm0 \n\t"\
            "paddw     %%mm3, %%mm0 \n\t"\
            "psubw    %[pw1], %%mm0 \n\t" /* spatial_score */\
\
            CHECK(-2,0)\
            CHECK1\
            CHECK(-3,1)\
            CHECK2\
            CHECK(0,-2)\
            CHECK1\
            CHECK(1,-3)\
            CHECK2\
\
            /* if(p->mode<2) ... */\
            "movq    %[tmp3], %%mm6 \n\t" /* diff */\
            "cmpl      $2, %[mode] \n\t"\
            "jge       1f \n\t"\
            LOAD4("(%["prev2"],%[mrefs],2)", %%mm2) /* prev2[x-2*refs] */\
            LOAD4("(%["next2"],%[mrefs],2)", %%mm4) /* next2[x-2*refs] */\
            LOAD4("(%["prev2"],%[prefs],2)", %%mm3) /* prev2[x+2*refs] */\
            LOAD4("(%["next2"],%[prefs],2)", %%mm5) /* next2[x+2*refs] */\
            "paddw     %%mm4, %%mm2 \n\t"\
            "paddw     %%mm5, %%mm3 \n\t"\
            "psrlw     $1,    %%mm2 \n\t" /* b */\
            "psrlw     $1,    %%mm3 \n\t" /* f */\
            "movq    %[tmp0], %%mm4 \n\t" /* c */\
            "movq    %[tmp1], %%mm5 \n\t" /* d */\
            "movq    %[tmp2], %%mm7 \n\t" /* e */\
            "psubw     %%mm4, %%mm2 \n\t" /* b-c */\
            "psubw     %%mm7, %%mm3 \n\t" /* f-e */\
            "movq      %%mm5, %%mm0 \n\t"\
            "psubw     %%mm4, %%mm5 \n\t" /* d-c */\
            "psubw     %%mm7, %%mm0 \n\t" /* d-e */\
            "movq      %%mm2, %%mm4 \n\t"\
            "pminsw    %%mm3, %%mm2 \n\t"\
            "pmaxsw    %%mm4, %%mm3 \n\t"\
            "pmaxsw    %%mm5, %%mm2 \n\t"\
            "pminsw    %%mm5, %%mm3 \n\t"\
            "pmaxsw    %%mm0, %%mm2 \n\t" /* max */\
            "pminsw    %%mm0, %%mm3 \n\t" /* min */\
            "pxor      %%mm4, %%mm4 \n\t"\
            "pmaxsw    %%mm3, %%mm6 \n\t"\
            "psubw     %%mm2, %%mm4 \n\t" /* -max */\
            "pmaxsw    %%mm4, %%mm6 \n\t" /* diff= MAX3(diff, min, -max); */\
            "1: \n\t"\
\
            "movq    %[tmp1], %%mm2 \n\t" /* d */\
            "movq      %%mm2, %%mm3 \n\t"\
            "psubw     %%mm6, %%mm2 \n\t" /* d-diff */\
            "paddw     %%mm6, %%mm3 \n\t" /* d+diff */\
            "pmaxsw    %%mm2, %%mm1 \n\t"\
            "pminsw    %%mm3, %%mm1 \n\t" /* d = clip(spatial_pred, d-diff, d+diff); */\
            "packuswb  %%mm1, %%mm1 \n\t"\
\
            :[tmp0]"=m"(tmp0),\
             [tmp1]"=m"(tmp1),\
             [tmp2]"=m"(tmp2),\
             [tmp3]"=m"(tmp3)\
            :[prev] "r"(prev),\
             [cur]  "r"(cur),\
             [next] "r"(next),\
			 [prefs]"r"((intptr_t)refs),\
			 [mrefs]"r"((intptr_t)-refs),\
             [pw1]  "m"(pw_1),\
             [pb1]  "m"(pb_1),\
             [mode] "g"(mode)\
        );\
        asm volatile("movd %%mm1, %0" :"=m"(*dst));\
        dst += 4;\
        prev+= 4;\
        cur += 4;\
        next+= 4;\
    }

    if(parity){
#define prev2 "prev"
#define next2 "cur"
        FILTER
#undef prev2
#undef next2
    }else{
#define prev2 "cur"
#define next2 "next"
        FILTER
#undef prev2
#undef next2
    }
}
#undef LOAD4
#undef PABS
#undef CHECK
#undef CHECK1
#undef CHECK2
#undef FILTER

#ifndef attribute_align_arg
#if defined(__GNUC__) && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__>1)
#    define attribute_align_arg __attribute__((force_align_arg_pointer))
#else
#    define attribute_align_arg
#endif
#endif

// for proper alignment SSE2 we need in GCC 4.2 and above
#if (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__>1)

#ifndef DECLARE_ALIGNED
#define DECLARE_ALIGNED(n,t,v)       t v __attribute__ ((aligned (n)))
#endif

// ================= SSE2 =================
#if defined(USE_SSE2) && defined(ARCH_X86_64)
#define PABS(tmp,dst) \
            "pxor     "#tmp", "#tmp" \n\t"\
            "psubw    "#dst", "#tmp" \n\t"\
            "pmaxsw   "#tmp", "#dst" \n\t"

#define FILTER_LINE_FUNC_NAME filter_line_sse2
#include "vf_yadif_template.h"
#endif

// ================ SSSE3 =================
#ifdef USE_SSE3
#define PABS(tmp,dst) \
            "pabsw     "#dst", "#dst" \n\t"

#define FILTER_LINE_FUNC_NAME filter_line_ssse3
#include "vf_yadif_template.h"
#endif

#endif // GCC 4.2+
#endif // GNUC, USE_SSE

static void filter_line_c(int mode, uint8_t *dst, const uint8_t *prev, const uint8_t *cur, const uint8_t *next, int w, int refs, int parity){
    int x;
    const uint8_t *prev2= parity ? prev : cur ;
    const uint8_t *next2= parity ? cur  : next;
    for(x=0; x<w; x++){
        int c= cur[-refs];
        int d= (prev2[0] + next2[0])>>1;
        int e= cur[+refs];
        int temporal_diff0= ABS(prev2[0] - next2[0]);
        int temporal_diff1=( ABS(prev[-refs] - c) + ABS(prev[+refs] - e) )>>1;
        int temporal_diff2=( ABS(next[-refs] - c) + ABS(next[+refs] - e) )>>1;
        int diff= MAX3(temporal_diff0>>1, temporal_diff1, temporal_diff2);
        int spatial_pred= (c+e)>>1;
        int spatial_score= ABS(cur[-refs-1] - cur[+refs-1]) + ABS(c-e)
                         + ABS(cur[-refs+1] - cur[+refs+1]) - 1;

#define CHECK(j)\
    {   int score= ABS(cur[-refs-1+ j] - cur[+refs-1- j])\
                 + ABS(cur[-refs  + j] - cur[+refs  - j])\
                 + ABS(cur[-refs+1+ j] - cur[+refs+1- j]);\
        if(score < spatial_score){\
            spatial_score= score;\
            spatial_pred= (cur[-refs  + j] + cur[+refs  - j])>>1;\

        CHECK(-1) CHECK(-2) }} }}
        CHECK( 1) CHECK( 2) }} }}

        if(mode<2){
            int b= (prev2[-2*refs] + next2[-2*refs])>>1;
            int f= (prev2[+2*refs] + next2[+2*refs])>>1;
#if 0
            int a= cur[-3*refs];
            int g= cur[+3*refs];
            int max= MAX3(d-e, d-c, MIN3(MAX(b-c,f-e),MAX(b-c,b-a),MAX(f-g,f-e)) );
            int min= MIN3(d-e, d-c, MAX3(MIN(b-c,f-e),MIN(b-c,b-a),MIN(f-g,f-e)) );
#else
            int max= MAX3(d-e, d-c, MIN(b-c, f-e));
            int min= MIN3(d-e, d-c, MAX(b-c, f-e));
#endif

            diff= MAX3(diff, min, -max);
        }

        if(spatial_pred > d + diff)
           spatial_pred = d + diff;
        else if(spatial_pred < d - diff)
           spatial_pred = d - diff;

        dst[0] = spatial_pred;

        dst++;
        cur++;
        prev++;
        next++;
        prev2++;
        next2++;
    }
}

static void interpolate(uint8_t *dst, const uint8_t *cur0,  const uint8_t *cur2, int w)
{
    int x;
    for (x=0; x<w; x++) {
        dst[x] = (cur0[x] + cur2[x] + 1)>>1; // simple average
    }
}

void filter_plane(int mode, uint8_t *dst, int dst_stride, const uint8_t *prev0, const uint8_t *cur0, const uint8_t *next0, int refs, int w, int h, int parity, int tff, int cpu){

	int y;
	filter_line = filter_line_c;
#ifdef __GNUC__
#if (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__>1)
#ifdef USE_SSE3
	if (cpu & AVS_CPU_SSSE3)
		filter_line = filter_line_ssse3;
	else
#endif
#if defined(USE_SSE2) && defined(ARCH_X86_64)
	if (cpu & AVS_CPU_SSE2)
		filter_line = filter_line_sse2;
	else
#endif
#endif // GCC 4.2+
#ifdef USE_SSE
	if (cpu & AVS_CPU_INTEGER_SSE)
		filter_line = filter_line_mmx2;
#endif
#endif // GNUC
        y=0;
        if(((y ^ parity) & 1)){
            memcpy(dst, cur0 + refs, w);// duplicate 1
        }else{
            memcpy(dst, cur0, w);
        }
        y=1;
        if(((y ^ parity) & 1)){
            interpolate(dst + dst_stride, cur0, cur0 + refs*2, w);   // interpolate 0 and 2
        }else{
            memcpy(dst + dst_stride, cur0 + refs, w); // copy original
        }
        for(y=2; y<h-2; y++){
            if(((y ^ parity) & 1)){
                const uint8_t *prev= prev0 + y*refs;
                const uint8_t *cur = cur0 + y*refs;
                const uint8_t *next= next0 + y*refs;
                uint8_t *dst2= dst + y*dst_stride;
                filter_line(mode, dst2, prev, cur, next, w, refs, (parity ^ tff));
            }else{
                memcpy(dst + y*dst_stride, cur0 + y*refs, w); // copy original
            }
        }
       y=h-2;
        if(((y ^ parity) & 1)){
            interpolate(dst + (h-2)*dst_stride, cur0 + (h-3)*refs, cur0 + (h-1)*refs, w);   // interpolate h-3 and h-1
        }else{
            memcpy(dst + (h-2)*dst_stride, cur0 + (h-2)*refs, w); // copy original
        }
        y=h-1;
        if(((y ^ parity) & 1)){
            memcpy(dst + (h-1)*dst_stride, cur0 + (h-2)*refs, w); // duplicate h-2
        }else{
            memcpy(dst + (h-1)*dst_stride, cur0 + (h-1)*refs, w); // copy original
        }

#if defined(__GNUC__) && defined(USE_SSE)
	if (cpu >= AVS_CPU_INTEGER_SSE)
		asm volatile("emms");
#endif
}

#if defined(__GNUC__) && defined(USE_SSE) && !defined(PIC)
static attribute_align_arg void  YUY2ToPlanes_mmx(const unsigned char *srcYUY2, int pitch_yuy2, int width, int height,
                    unsigned char *py, int pitch_y,
                    unsigned char *pu, unsigned char *pv,  int pitch_uv)
{ /* process by 16 bytes (8 pixels), so width is assumed mod 8 */
    int widthdiv2 = width>>1;
//    static unsigned __int64 Ymask = 0x00FF00FF00FF00FFULL;
    int h;
    for (h=0; h<height; h++)
    {
        asm (\
        "pcmpeqb %%mm5, %%mm5 \n\t"  /* prepare Ymask FFFFFFFFFFFFFFFF */\
        "psrlw $8, %%mm5 \n\t" /* Ymask = 00FF00FF00FF00FF */\
        "xor %%eax, %%eax \n\t"\
        "xloop%= : \n\t"\
        "prefetchnta 0xc0(%%edi,%%eax,4) \n\t"\
        "movq (%%edi,%%eax,4), %%mm0 \n\t" /* src VYUYVYUY - 1 */\
        "movq 8(%%edi,%%eax,4), %%mm1 \n\t" /* src VYUYVYUY - 2 */\
        "movq %%mm0, %%mm2 \n\t" /* VYUYVYUY - 1 */\
        "movq %%mm1, %%mm3 \n\t" /* VYUYVYUY - 2 */\
        "pand %%mm5, %%mm0 \n\t" /* 0Y0Y0Y0Y - 1 */\
        "psrlw $8, %%mm2 \n\t" /* 0V0U0V0U - 1 */\
        "pand %%mm5, %%mm1 \n\t" /* 0Y0Y0Y0Y - 2 */\
        "psrlw $8, %%mm3 \n\t" /* 0V0U0V0U - 2 */\
        "packuswb %%mm1, %%mm0 \n\t" /* YYYYYYYY */\
        "packuswb %%mm3, %%mm2 \n\t" /* VUVUVUVU */\
        "movntq %%mm0, (%%ebx,%%eax,2) \n\t" /* store y */\
        "movq %%mm2, %%mm4 \n\t" /* VUVUVUVU */\
        "pand %%mm5, %%mm2 \n\t" /* 0U0U0U0U */\
        "psrlw $8, %%mm4 \n\t" /* 0V0V0V0V */\
        "packuswb %%mm2, %%mm2 \n\t" /* xxxxUUUU */\
        "packuswb %%mm4, %%mm4 \n\t" /* xxxxVVVV */\
        "movd %%mm2, (%%edx,%%eax) \n\t" /* store u */\
        "add $4, %%eax \n\t" \
        "cmpl %%ecx, %%eax \n\t" \
        "movd %%mm4, -4(%%esi,%%eax) \n\t" /* store v */\
        "jl xloop%= \n\t"\
        : : "D"(srcYUY2), "b"(py), "d"(pu), "S"(pv), "c"(widthdiv2) : "%eax");

        srcYUY2 += pitch_yuy2;
        py += pitch_y;
        pu += pitch_uv;
        pv += pitch_uv;
    }
    asm ("sfence \n\t emms");
}

static attribute_align_arg void YUY2FromPlanes_mmx(unsigned char *dstYUY2, int pitch_yuy2, int width, int height,
                    const unsigned char *py, int pitch_y,
                    const unsigned char *pu, const unsigned char *pv,  int pitch_uv)
{
    int widthdiv2 = width >> 1;
    int h;
    for (h=0; h<height; h++)
    {
        asm (\
        "xor %%eax, %%eax \n\t"\
        "xloop%=: \n\t"\
        "movd (%%edx,%%eax), %%mm1 \n\t" /* 0000UUUU */\
        "movd (%%esi,%%eax), %%mm2 \n\t" /* 0000VVVV */\
        "movq (%%ebx,%%eax,2), %%mm0 \n\t" /* YYYYYYYY */\
        "punpcklbw %%mm2,%%mm1 \n\t" /* VUVUVUVU */\
        "movq %%mm0, %%mm3 \n\t" /* YYYYYYYY */\
        "punpcklbw %%mm1, %%mm0 \n\t" /* VYUYVYUY */\
        "add $4, %%eax \n\t"\
        "punpckhbw %%mm1, %%mm3 \n\t" /* VYUYVYUY */\
        "movntq %%mm0, -16(%%edi,%%eax,4) \n\t" /*store */\
        "movntq %%mm3, -8(%%edi,%%eax,4) \n\t" /*  store */\
        "cmpl %%ecx, %%eax \n\t"\
        "jl xloop%= \n\t"\
        : : "b"(py), "d"(pu), "S"(pv), "D"(dstYUY2), "c"(widthdiv2) : "%eax");
        py += pitch_y;
        pu += pitch_uv;
        pv += pitch_uv;
        dstYUY2 += pitch_yuy2;
    }
    asm ("sfence \n\t emms");
}
#endif // GNUC, USE_SSE, !PIC

//----------------------------------------------------------------------------------------------

void YUY2ToPlanes(const unsigned char *pSrcYUY2, int nSrcPitchYUY2, int nWidth, int nHeight,
							   unsigned char * pSrcY, int srcPitchY,
							   unsigned char * pSrcU,  unsigned char * pSrcV, int srcPitchUV, int cpu)
{

    int h,w;
    int w0 = 0;
#if defined(__GNUC__) && defined(USE_SSE) && !defined(PIC)
    if (cpu & AVS_CPU_INTEGER_SSE) {
        w0 = (nWidth/8)*8;
        YUY2ToPlanes_mmx(pSrcYUY2, nSrcPitchYUY2, w0, nHeight, pSrcY, srcPitchY, pSrcU, pSrcV, srcPitchUV);
    }
#endif
	for (h=0; h<nHeight; h++)
	{
		for (w=w0; w<nWidth; w+=2)
		{
			int w2 = w+w;
			pSrcY[w] = pSrcYUY2[w2];
			pSrcY[w+1] = pSrcYUY2[w2+2];
			pSrcU[(w>>1)] = pSrcYUY2[w2+1];
			pSrcV[(w>>1)] = pSrcYUY2[w2+3];
		}
		pSrcY += srcPitchY;
		pSrcU += srcPitchUV;
		pSrcV += srcPitchUV;
		pSrcYUY2 += nSrcPitchYUY2;
	}
}

//----------------------------------------------------------------------------------------------

void YUY2FromPlanes(unsigned char *pSrcYUY2, int nSrcPitchYUY2, int nWidth, int nHeight,
							  const unsigned char * pSrcY, int srcPitchY,
							  const unsigned char * pSrcU, const unsigned char * pSrcV, int srcPitchUV, int cpu)
{
    int h,w;
    int w0 = 0;
#if defined(__GNUC__) && defined(USE_SSE) && !defined(PIC)
    if (cpu & AVS_CPU_INTEGER_SSE) {
        w0 = (nWidth/8)*8;
        YUY2FromPlanes_mmx(pSrcYUY2, nSrcPitchYUY2, w0, nHeight, pSrcY, srcPitchY, pSrcU, pSrcV, srcPitchUV);
    }
#endif
  for (h=0; h<nHeight; h++)
	{
		for (w=w0; w<nWidth; w+=2)
		{
			int w2 = w+w;
			pSrcYUY2[w2] = pSrcY[w];
			pSrcYUY2[w2+1] = pSrcU[(w>>1)];
			pSrcYUY2[w2+2] = pSrcY[w+1];
			pSrcYUY2[w2+3] = pSrcV[(w>>1)];
		}
		pSrcY += srcPitchY;
		pSrcU += srcPitchUV;
		pSrcV += srcPitchUV;
		pSrcYUY2 += nSrcPitchYUY2;
	}
}
