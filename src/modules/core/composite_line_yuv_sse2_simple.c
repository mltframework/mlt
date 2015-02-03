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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

void composite_line_yuv_sse2_simple(uint8_t *dest, uint8_t *src, int width, uint8_t *src_a, uint8_t *dest_a, int weight)
{
    __asm__ volatile
    (
        LOAD_CONSTS
        LOAD_WEIGHT
        "loop_start:                            \n\t"
        LOAD_SRC_A
        SRC_A_PREMUL
        DST_A_CALC
        DST_PIX_CALC
        "add            $0x08, %[src_a]         \n\t"
        "add            $0x08, %[dest_a]        \n\t"
        PIX_POINTER_INC
        "jnz            loop_start              \n\t"
        :
        : [weight] "r" (weight >> 8), [src_a] "r" (src_a), [src] "r" (src), [dest] "r" (dest), [const1] "r" (const1) , [dest_a] "r" (dest_a), [width] "r" (width / 8), [const2] "r" (const2)
        //: "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9", "memory"
    );
};
