/*
 * Copyright (C) 2006 Michael Niedermayer <michaelni@gmx.at>
 *
 * SSE2/SSSE3 version (custom optimization) by h.yamagata
 *
 * Small fix by Alexander Balakhnin (fizick@avisynth.org.ru)
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdint.h>

#define LOAD8(mem,dst) \
            "movq      "mem", "#dst" \n\t"\
            "punpcklbw %%xmm7, "#dst" \n\t"

#define CHECK(pj,mj) \
            "movdqu "#pj"(%[cur],%[mrefs]), %%xmm2 \n\t" /* cur[x-refs-1+j] */\
            "movdqu "#mj"(%[cur],%[prefs]), %%xmm3 \n\t" /* cur[x+refs-1-j] */\
            "movdqa      %%xmm2, %%xmm4 \n\t"\
            "movdqa      %%xmm2, %%xmm5 \n\t"\
            "pxor        %%xmm3, %%xmm4 \n\t"\
            "pavgb       %%xmm3, %%xmm5 \n\t"\
            "pand        %[pb1], %%xmm4 \n\t"\
            "psubusb     %%xmm4, %%xmm5 \n\t"\
            "psrldq      $1,    %%xmm5 \n\t"\
            "punpcklbw   %%xmm7, %%xmm5 \n\t" /* (cur[x-refs+j] + cur[x+refs-j])>>1 */\
            "movdqa      %%xmm2, %%xmm4 \n\t"\
            "psubusb     %%xmm3, %%xmm2 \n\t"\
            "psubusb     %%xmm4, %%xmm3 \n\t"\
            "pmaxub      %%xmm3, %%xmm2 \n\t"\
            "movdqa      %%xmm2, %%xmm3 \n\t"\
            "movdqa      %%xmm2, %%xmm4 \n\t" /* ABS(cur[x-refs-1+j] - cur[x+refs-1-j]) */\
            "psrldq      $1,   %%xmm3 \n\t" /* ABS(cur[x-refs  +j] - cur[x+refs  -j]) */\
            "psrldq      $2,   %%xmm4 \n\t" /* ABS(cur[x-refs+1+j] - cur[x+refs+1-j]) */\
            "punpcklbw   %%xmm7, %%xmm2 \n\t"\
            "punpcklbw   %%xmm7, %%xmm3 \n\t"\
            "punpcklbw   %%xmm7, %%xmm4 \n\t"\
            "paddw       %%xmm3, %%xmm2 \n\t"\
            "paddw       %%xmm4, %%xmm2 \n\t" /* score */

#define CHECK1 \
            "movdqa      %%xmm0, %%xmm3 \n\t"\
            "pcmpgtw     %%xmm2, %%xmm3 \n\t" /* if(score < spatial_score) */\
            "pminsw      %%xmm2, %%xmm0 \n\t" /* spatial_score= score; */\
            "movdqa      %%xmm3, %%xmm6 \n\t"\
            "pand        %%xmm3, %%xmm5 \n\t"\
            "pandn       %%xmm1, %%xmm3 \n\t"\
            "por         %%xmm5, %%xmm3 \n\t"\
            "movdqa      %%xmm3, %%xmm1 \n\t" /* spatial_pred= (cur[x-refs+j] + cur[x+refs-j])>>1; */

#define CHECK2 /* pretend not to have checked dir=2 if dir=1 was bad.\
                  hurts both quality and speed, but matches the C version. */\
            "paddw       %[pw1], %%xmm6 \n\t"\
            "psllw       $14,   %%xmm6 \n\t"\
            "paddsw      %%xmm6, %%xmm2 \n\t"\
            "movdqa      %%xmm0, %%xmm3 \n\t"\
            "pcmpgtw     %%xmm2, %%xmm3 \n\t"\
            "pminsw      %%xmm2, %%xmm0 \n\t"\
            "pand        %%xmm3, %%xmm5 \n\t"\
            "pandn       %%xmm1, %%xmm3 \n\t"\
            "por         %%xmm5, %%xmm3 \n\t"\
            "movdqa      %%xmm3, %%xmm1 \n\t"

/* mode argument mod - Fizick */

/* static  attribute_align_arg void FILTER_LINE_FUNC_NAME(YadifContext *yadctx, uint8_t *dst, uint8_t *prev, uint8_t *cur, uint8_t *next, int w, int refs, int parity){
     const int mode = yadctx->mode; */
static attribute_align_arg void FILTER_LINE_FUNC_NAME(int mode, uint8_t *dst, const uint8_t *prev, const uint8_t *cur, const uint8_t *next, int w, int refs, int parity){
    DECLARE_ALIGNED(16, uint8_t, tmp0[16]);
    DECLARE_ALIGNED(16, uint8_t, tmp1[16]);
    DECLARE_ALIGNED(16, uint8_t, tmp2[16]);
    DECLARE_ALIGNED(16, uint8_t, tmp3[16]);
    int x;
    static DECLARE_ALIGNED(16, const unsigned short, pw_1[]) =
    {
        0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001
    };

    static DECLARE_ALIGNED(16, const unsigned short, pb_1[]) =
    {
        0x0101,0x0101,0x0101,0x0101,0x0101,0x0101,0x0101,0x0101
    };


#define FILTER\
    for(x=0; x<w; x+=8){\
        __asm__ volatile(\
            "pxor        %%xmm7, %%xmm7 \n\t"\
            LOAD8("(%[cur],%[mrefs])", %%xmm0) /* c = cur[x-refs] */\
            LOAD8("(%[cur],%[prefs])", %%xmm1) /* e = cur[x+refs] */\
            LOAD8("(%["prev2"])", %%xmm2) /* prev2[x] */\
            LOAD8("(%["next2"])", %%xmm3) /* next2[x] */\
            "movdqa      %%xmm3, %%xmm4 \n\t"\
            "paddw       %%xmm2, %%xmm3 \n\t"\
            "psraw       $1,    %%xmm3 \n\t" /* d = (prev2[x] + next2[x])>>1 */\
            "movdqa      %%xmm0, %[tmp0] \n\t" /* c */\
            "movdqa      %%xmm3, %[tmp1] \n\t" /* d */\
            "movdqa      %%xmm1, %[tmp2] \n\t" /* e */\
            "psubw       %%xmm4, %%xmm2 \n\t"\
            PABS(        %%xmm4, %%xmm2) /* temporal_diff0 */\
            LOAD8("(%[prev],%[mrefs])", %%xmm3) /* prev[x-refs] */\
            LOAD8("(%[prev],%[prefs])", %%xmm4) /* prev[x+refs] */\
            "psubw       %%xmm0, %%xmm3 \n\t"\
            "psubw       %%xmm1, %%xmm4 \n\t"\
            PABS(        %%xmm5, %%xmm3)\
            PABS(        %%xmm5, %%xmm4)\
            "paddw       %%xmm4, %%xmm3 \n\t" /* temporal_diff1 */\
            "psrlw       $1,    %%xmm2 \n\t"\
            "psrlw       $1,    %%xmm3 \n\t"\
            "pmaxsw      %%xmm3, %%xmm2 \n\t"\
            LOAD8("(%[next],%[mrefs])", %%xmm3) /* next[x-refs] */\
            LOAD8("(%[next],%[prefs])", %%xmm4) /* next[x+refs] */\
            "psubw       %%xmm0, %%xmm3 \n\t"\
            "psubw       %%xmm1, %%xmm4 \n\t"\
            PABS(        %%xmm5, %%xmm3)\
            PABS(        %%xmm5, %%xmm4)\
            "paddw       %%xmm4, %%xmm3 \n\t" /* temporal_diff2 */\
            "psrlw       $1,    %%xmm3 \n\t"\
            "pmaxsw      %%xmm3, %%xmm2 \n\t"\
            "movdqa      %%xmm2, %[tmp3] \n\t" /* diff */\
\
            "paddw       %%xmm0, %%xmm1 \n\t"\
            "paddw       %%xmm0, %%xmm0 \n\t"\
            "psubw       %%xmm1, %%xmm0 \n\t"\
            "psrlw       $1,    %%xmm1 \n\t" /* spatial_pred */\
            PABS(        %%xmm2, %%xmm0)      /* ABS(c-e) */\
\
            "movdqu      -1(%[cur],%[mrefs]), %%xmm2 \n\t" /* cur[x-refs-1] */\
            "movdqu      -1(%[cur],%[prefs]), %%xmm3 \n\t" /* cur[x+refs-1] */\
            "movdqa      %%xmm2, %%xmm4 \n\t"\
            "psubusb     %%xmm3, %%xmm2 \n\t"\
            "psubusb     %%xmm4, %%xmm3 \n\t"\
            "pmaxub      %%xmm3, %%xmm2 \n\t"\
            /*"pshuflw      $9,%%xmm2, %%xmm3 \n\t"*/\
            /*"pshufhw      $9,%%xmm2, %%xmm3 \n\t"*/\
            "movdqa %%xmm2, %%xmm3 \n\t" /* correct replacement (here)  */\
            "psrldq $2, %%xmm3 \n\t"/* for "pshufw $9,%%mm2, %%mm3" - fix by Fizick */\
            "punpcklbw   %%xmm7, %%xmm2 \n\t" /* ABS(cur[x-refs-1] - cur[x+refs-1]) */\
            "punpcklbw   %%xmm7, %%xmm3 \n\t" /* ABS(cur[x-refs+1] - cur[x+refs+1]) */\
            "paddw       %%xmm2, %%xmm0 \n\t"\
            "paddw       %%xmm3, %%xmm0 \n\t"\
            "psubw       %[pw1], %%xmm0 \n\t" /* spatial_score */\
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
            /* if(yadctx->mode<2) ... */\
            "movdqa      %[tmp3], %%xmm6 \n\t" /* diff */\
            "cmpl        $2, %[mode] \n\t"\
            "jge         1f \n\t"\
            LOAD8("(%["prev2"],%[mrefs],2)", %%xmm2) /* prev2[x-2*refs] */\
            LOAD8("(%["next2"],%[mrefs],2)", %%xmm4) /* next2[x-2*refs] */\
            LOAD8("(%["prev2"],%[prefs],2)", %%xmm3) /* prev2[x+2*refs] */\
            LOAD8("(%["next2"],%[prefs],2)", %%xmm5) /* next2[x+2*refs] */\
            "paddw       %%xmm4, %%xmm2 \n\t"\
            "paddw       %%xmm5, %%xmm3 \n\t"\
            "psrlw       $1,    %%xmm2 \n\t" /* b */\
            "psrlw       $1,    %%xmm3 \n\t" /* f */\
            "movdqa      %[tmp0], %%xmm4 \n\t" /* c */\
            "movdqa      %[tmp1], %%xmm5 \n\t" /* d */\
            "movdqa      %[tmp2], %%xmm7 \n\t" /* e */\
            "psubw       %%xmm4, %%xmm2 \n\t" /* b-c */\
            "psubw       %%xmm7, %%xmm3 \n\t" /* f-e */\
            "movdqa      %%xmm5, %%xmm0 \n\t"\
            "psubw       %%xmm4, %%xmm5 \n\t" /* d-c */\
            "psubw       %%xmm7, %%xmm0 \n\t" /* d-e */\
            "movdqa      %%xmm2, %%xmm4 \n\t"\
            "pminsw      %%xmm3, %%xmm2 \n\t"\
            "pmaxsw      %%xmm4, %%xmm3 \n\t"\
            "pmaxsw      %%xmm5, %%xmm2 \n\t"\
            "pminsw      %%xmm5, %%xmm3 \n\t"\
            "pmaxsw      %%xmm0, %%xmm2 \n\t" /* max */\
            "pminsw      %%xmm0, %%xmm3 \n\t" /* min */\
            "pxor        %%xmm4, %%xmm4 \n\t"\
            "pmaxsw      %%xmm3, %%xmm6 \n\t"\
            "psubw       %%xmm2, %%xmm4 \n\t" /* -max */\
            "pmaxsw      %%xmm4, %%xmm6 \n\t" /* diff= MAX3(diff, min, -max); */\
            "1: \n\t"\
\
            "movdqa      %[tmp1], %%xmm2 \n\t" /* d */\
            "movdqa      %%xmm2, %%xmm3 \n\t"\
            "psubw       %%xmm6, %%xmm2 \n\t" /* d-diff */\
            "paddw       %%xmm6, %%xmm3 \n\t" /* d+diff */\
            "pmaxsw      %%xmm2, %%xmm1 \n\t"\
            "pminsw      %%xmm3, %%xmm1 \n\t" /* d = clip(spatial_pred, d-diff, d+diff); */\
            "packuswb    %%xmm1, %%xmm1 \n\t"\
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
             [pw1]  "m"(*pw_1),\
             [pb1]  "m"(*pb_1),\
             [mode] "g"(mode)\
        );\
        __asm__ volatile("movq %%xmm1, %0" :"=m"(*dst));\
        dst += 8;\
        prev+= 8;\
        cur += 8;\
        next+= 8;\
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
#undef LOAD8
#undef PABS
#undef CHECK
#undef CHECK1
#undef CHECK2
#undef FILTER
#undef FILTER_LINE_FUNC_NAME
