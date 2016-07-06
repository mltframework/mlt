/*
 * composite_line_yuv_sse2_simple.c
 * Copyright (C) 2003-2014 Meltytech, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

const static unsigned char const1[] =
{
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00
};
const static unsigned char const2[] =
{
    0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00
};

#define LOAD_CONSTS     \
        "pxor           %%xmm0, %%xmm0          \n\t"   /* clear zero register */       \
        "movdqu         (%[const1]), %%xmm9     \n\t"   /* load const1 */               \
        "movdqu         (%[const2]), %%xmm10    \n\t"   /* load const2 */

#define LOAD_WEIGHT     \
        "movd           %[weight], %%xmm1       \n\t"   /* load weight and decompose */ \
        "movlhps        %%xmm1, %%xmm1          \n\t"                                   \
        "pshuflw        $0, %%xmm1, %%xmm1      \n\t"                                   \
        "pshufhw        $0, %%xmm1, %%xmm1      \n\t"

#define LOAD_SRC_A      \
        "movq           (%[src_a]), %%xmm2      \n\t"   /* load source alpha */         \
        "punpcklbw      %%xmm0, %%xmm2          \n\t"   /* unpack alpha 8 8-bits alphas to 8 16-bits values */

#define SRC_A_PREMUL    \
        "pmullw         %%xmm1, %%xmm2          \n\t"   /* premultiply source alpha */  \
        "psrlw          $8, %%xmm2              \n\t"

# define DST_A_CALC \
        /* DSTa = DSTa + (SRCa * (0xFF - DSTa)) >> 8  */ \
        "movq           (%[dest_a]), %%xmm3     \n\t"   /* load dst alpha */    \
        "punpcklbw      %%xmm0, %%xmm3          \n\t"   /* unpack dst 8 8-bits alphas to 8 16-bits values */    \
        "movdqa         %%xmm9, %%xmm4          \n\t"   \
        "psubw          %%xmm3, %%xmm4          \n\t"   \
        "pmullw         %%xmm2, %%xmm4          \n\t"   \
        "movdqa         %%xmm4, %%xmm5          \n\t"   \
        "psrlw          $8, %%xmm4              \n\t"   \
        "paddw          %%xmm5, %%xmm4          \n\t"   \
        "paddw          %%xmm10, %%xmm4         \n\t"   \
        "psrlw          $8, %%xmm4              \n\t"   \
        "paddw          %%xmm4, %%xmm3          \n\t"   \
        "packuswb       %%xmm0, %%xmm3          \n\t"   \
        "movq           %%xmm3, (%[dest_a])     \n\t"   /* save dst alpha */

#define DST_PIX_CALC \
        "movdqu         (%[src]), %%xmm3        \n\t"   /* load src */          \
        "movdqu         (%[dest]), %%xmm4       \n\t"   /* load dst */          \
        "movdqa         %%xmm3, %%xmm5          \n\t"   /* dub src */           \
        "movdqa         %%xmm4, %%xmm6          \n\t"   /* dub dst */           \
        "punpcklbw      %%xmm0, %%xmm5          \n\t"   /* unpack src low */    \
        "punpcklbw      %%xmm0, %%xmm6          \n\t"   /* unpack dst low */    \
        "punpckhbw      %%xmm0, %%xmm3          \n\t"   /* unpack src high */   \
        "punpckhbw      %%xmm0, %%xmm4          \n\t"   /* unpack dst high */   \
        "movdqa         %%xmm2, %%xmm7          \n\t"   /* dub alpha */         \
        "movdqa         %%xmm2, %%xmm8          \n\t"   /* dub alpha */         \
        "movlhps        %%xmm7, %%xmm7          \n\t"   /* dub low */           \
        "movhlps        %%xmm8, %%xmm8          \n\t"   /* dub high */          \
        "pshuflw        $0x50, %%xmm7, %%xmm7   \n\t"                           \
        "pshuflw        $0x50, %%xmm8, %%xmm8   \n\t"                           \
        "pshufhw        $0xFA, %%xmm7, %%xmm7   \n\t"                           \
        "pshufhw        $0xFA, %%xmm8, %%xmm8   \n\t"                           \
        "psubw          %%xmm4, %%xmm3          \n\t"   /* src = src - dst */   \
        "psubw          %%xmm6, %%xmm5          \n\t"                           \
        "pmullw         %%xmm8, %%xmm3          \n\t"   /* src = src * alpha */ \
        "pmullw         %%xmm7, %%xmm5          \n\t"                           \
        "pmullw         %%xmm9, %%xmm4          \n\t"   /* dst = dst * 0xFF */  \
        "pmullw         %%xmm9, %%xmm6          \n\t"                           \
        "paddw          %%xmm3, %%xmm4          \n\t"   /* dst = dst + src */   \
        "paddw          %%xmm5, %%xmm6          \n\t"                           \
        "movdqa         %%xmm4, %%xmm3          \n\t"   /* dst = ((dst >> 8) + dst + 128) >> 8 */ \
        "movdqa         %%xmm6, %%xmm5          \n\t"                           \
        "psrlw          $8, %%xmm4              \n\t"                           \
        "psrlw          $8, %%xmm6              \n\t"                           \
        "paddw          %%xmm3, %%xmm4          \n\t"                           \
        "paddw          %%xmm5, %%xmm6          \n\t"                           \
        "paddw          %%xmm10, %%xmm4         \n\t"                           \
        "paddw          %%xmm10, %%xmm6         \n\t"                           \
        "psrlw          $8, %%xmm4              \n\t"                           \
        "psrlw          $8, %%xmm6              \n\t"                           \
        "packuswb       %%xmm4, %%xmm6          \n\t"                           \
        "movdqu         %%xmm6, (%[dest])       \n\t"   /* store dst */

#define PIX_POINTER_INC \
        "add            $0x10, %[src]           \n\t"   \
        "add            $0x10, %[dest]          \n\t"   \
        "dec            %[width]                \n\t"

static void blend_case7(uint8_t *dest, uint8_t *src, int width, uint8_t *src_a, uint8_t *dest_a, int weight)
{
    __asm__ volatile
    (
        LOAD_CONSTS
        LOAD_WEIGHT
        "loop_start7:                           \n\t"
        LOAD_SRC_A
        SRC_A_PREMUL
        DST_A_CALC
        DST_PIX_CALC
        "add            $0x08, %[src_a]         \n\t"
        "add            $0x08, %[dest_a]        \n\t"
        PIX_POINTER_INC
        "jnz            loop_start7             \n\t"
        :
        : [weight] "r" (weight), [src_a] "r" (src_a), [src] "r" (src), [dest] "r" (dest), [const1] "r" (const1) , [dest_a] "r" (dest_a), [width] "r" (width / 8), [const2] "r" (const2)
    );
};

//  | 3   | dest_a == NULL | src_a != NULL | weight != 256 | blend: premultiply src alpha
static void blend_case3(uint8_t *dest, uint8_t *src, int width, uint8_t *src_a, int weight)
{
    __asm__ volatile
    (
        LOAD_CONSTS
        LOAD_WEIGHT
        "loop_start3:                           \n\t"
        LOAD_SRC_A
        SRC_A_PREMUL
        DST_PIX_CALC
        "add            $0x08, %[src_a]         \n\t"
        PIX_POINTER_INC
        "jnz            loop_start3             \n\t"
        :
        : [weight] "r" (weight), [src_a] "r" (src_a), [src] "r" (src), [dest] "r" (dest), [const1] "r" (const1), [width] "r" (width / 8), [const2] "r" (const2)
    );
};

//  | 2   | dest_a == NULL | src_a != NULL | weight == 255 | blend: only src alpha
static void blend_case2(uint8_t *dest, uint8_t *src, int width, uint8_t *src_a)
{
    __asm__ volatile
    (
        LOAD_CONSTS
        "loop_start2:                           \n\t"
        LOAD_SRC_A
        DST_PIX_CALC
        "add            $0x08, %[src_a]         \n\t"
        PIX_POINTER_INC
        "jnz            loop_start2             \n\t"
        :
        : [src_a] "r" (src_a), [src] "r" (src), [dest] "r" (dest), [const1] "r" (const1) , [width] "r" (width / 8), [const2] "r" (const2)
    );
};


//  | 1   | dest_a == NULL | src_a == NULL | weight != 256 | blend: with given alpha
static void blend_case1(uint8_t *dest, uint8_t *src, int width, int weight)
{
    __asm__ volatile
    (
        LOAD_CONSTS
        LOAD_WEIGHT
        "loop_start1:                           \n\t"
        "movdqa         %%xmm1, %%xmm2          \n\t"   /* src alpha cames from weight */
        DST_PIX_CALC
        PIX_POINTER_INC
        "jnz            loop_start1             \n\t"
        :
        : [weight] "r" (weight), [src] "r" (src), [dest] "r" (dest), [const1] "r" (const1), [width] "r" (width / 8), [const2] "r" (const2)
    );
};

//  | 5   | dest_a != NULL | src_a == NULL | weight != 256 | blend: with given alpha
static void blend_case5(uint8_t *dest, uint8_t *src, int width, uint8_t *dest_a, int weight)
{
    __asm__ volatile
    (
        LOAD_CONSTS
        LOAD_WEIGHT
        "loop_start5:                           \n\t"
        "movdqa         %%xmm1, %%xmm2          \n\t"   /* source alpha comes from weight */
        DST_A_CALC
        DST_PIX_CALC
        "add            $0x08, %[dest_a]        \n\t"
        PIX_POINTER_INC
        "jnz            loop_start5             \n\t"

        :
        : [weight] "r" (weight), [src] "r" (src), [dest] "r" (dest), [const1] "r" (const1) , [dest_a] "r" (dest_a), [width] "r" (width / 8), [const2] "r" (const2)
    );
};

//  | 6   | dest_a != NULL | src_a != NULL | weight == 256 | blend: full blend without src alpha premutiply
static void blend_case6(uint8_t *dest, uint8_t *src, int width, uint8_t *src_a, uint8_t *dest_a)
{
    __asm__ volatile
    (
        LOAD_CONSTS
        "loop_start6:                           \n\t"
        LOAD_SRC_A
        DST_A_CALC
        DST_PIX_CALC
        "add            $0x08, %[src_a]         \n\t"
        "add            $0x08, %[dest_a]        \n\t"
        PIX_POINTER_INC
        "jnz            loop_start6             \n\t"
        :
        : [src_a] "r" (src_a), [src] "r" (src), [dest] "r" (dest), [const1] "r" (const1) , [dest_a] "r" (dest_a), [width] "r" (width / 8), [const2] "r" (const2)
    );
};


void composite_line_yuv_sse2_simple(uint8_t *dest, uint8_t *src, int width, uint8_t *src_a, uint8_t *dest_a, int weight)
{
    weight >>= 8;

    /*
        | 0   | dest_a == NULL | src_a == NULL | weight == 256 | blit
        | 1   | dest_a == NULL | src_a == NULL | weight != 256 | blend: with given alpha
        | 2   | dest_a == NULL | src_a != NULL | weight == 256 | blend: only src alpha
        | 3   | dest_a == NULL | src_a != NULL | weight != 256 | blend: premultiply src alpha
        | 4   | dest_a != NULL | src_a == NULL | weight == 256 | blit: blit and set dst alpha to FF
        | 5   | dest_a != NULL | src_a == NULL | weight != 256 | blend: with given alpha
        | 6   | dest_a != NULL | src_a != NULL | weight == 256 | blend: full blend without src alpha premutiply
        | 7   | dest_a != NULL | src_a != NULL | weight != 256 | blend: full (origin version)
    */

    int cond = ((dest_a != NULL)?4:0) + ((src_a != NULL)?2:0) + ((weight != 256)?1:0);

    switch(cond)
    {
        case 0:
            memcpy(dest, src, 2 * width);
            break;
        case 1: blend_case1(dest, src, width, weight); break;
        case 2: blend_case2(dest, src, width, src_a); break;
        case 3: blend_case3(dest, src, width, src_a, weight); break;
        case 4:
            memcpy(dest, src, 2 * width);
            memset(dest_a, 0xFF, width);
            break;
        case 5: blend_case5(dest, src, width, dest_a, weight); break;
        case 6: blend_case6(dest, src, width, src_a, dest_a); break;
        case 7: blend_case7(dest, src, width, src_a, dest_a, weight); break;
    };
};
