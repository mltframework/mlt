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
void composite_line_yuv_sse2_simple(uint8_t *dest, uint8_t *src, int width, uint8_t *src_a, uint8_t *dest_a, int weight)
{
    const static unsigned char const1[] =
    {
        0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00
    };
    const static unsigned char const2[] =
    {
        0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00
    };

    __asm__ volatile
    (
        "pxor           %%xmm0, %%xmm0          \n\t"   /* clear zero register */
        "movdqu         (%[const1]), %%xmm9     \n\t"   /* load const1 */
        "movdqu         (%[const2]), %%xmm10    \n\t"   /* load const2 */
        "movd           %[weight], %%xmm1       \n\t"   /* load weight and decompose */
        "movlhps        %%xmm1, %%xmm1          \n\t"
        "pshuflw        $0, %%xmm1, %%xmm1      \n\t"
        "pshufhw        $0, %%xmm1, %%xmm1      \n\t"

        /*
            xmm1 (weight)

                    00  W 00  W 00  W 00  W 00  W 00  W 00  W 00  W
        */
        "loop_start:                            \n\t"
        "movq           (%[src_a]), %%xmm2      \n\t"   /* load source alpha */
        "punpcklbw      %%xmm0, %%xmm2          \n\t"   /* unpack alpha 8 8-bits alphas to 8 16-bits values */

        /*
            xmm2 (src alpha)
            xmm3 (dst alpha)

                    00 A8 00 A7 00 A6 00 A5 00 A4 00 A3 00 A2 00 A1
        */
        "pmullw         %%xmm1, %%xmm2          \n\t"   /* premultiply source alpha */
        "psrlw          $8, %%xmm2              \n\t"

        /*
            xmm2 (premultiplied)

                    00 A8 00 A7 00 A6 00 A5 00 A4 00 A3 00 A2 00 A1
        */


        /*
            DSTa = DSTa + (SRCa * (0xFF - DSTa)) >> 8
        */
        "movq           (%[dest_a]), %%xmm3     \n\t"   /* load dst alpha */
        "punpcklbw      %%xmm0, %%xmm3          \n\t"   /* unpack dst 8 8-bits alphas to 8 16-bits values */
        "movdqa         %%xmm9, %%xmm4          \n\t"
        "psubw          %%xmm3, %%xmm4          \n\t"
        "pmullw         %%xmm2, %%xmm4          \n\t"
        "movdqa         %%xmm4, %%xmm5          \n\t"
        "psrlw          $8, %%xmm4              \n\t"
        "paddw          %%xmm5, %%xmm4          \n\t"
        "paddw          %%xmm10, %%xmm4         \n\t"
        "psrlw          $8, %%xmm4              \n\t"
        "paddw          %%xmm4, %%xmm3          \n\t"
        "packuswb       %%xmm0, %%xmm3          \n\t"
        "movq           %%xmm3, (%[dest_a])     \n\t"   /* save dst alpha */

        "movdqu         (%[src]), %%xmm3        \n\t"   /* load src */
        "movdqu         (%[dest]), %%xmm4       \n\t"   /* load dst */
        "movdqa         %%xmm3, %%xmm5          \n\t"   /* dub src */
        "movdqa         %%xmm4, %%xmm6          \n\t"   /* dub dst */

        /*
            xmm3 (src)
            xmm4 (dst)
            xmm5 (src)
            xmm6 (dst)

                    U8 V8 U7 V7 U6 V6 U5 V5 U4 V4 U3 V3 U2 V2 U1 V1
        */

        "punpcklbw      %%xmm0, %%xmm5          \n\t"   /* unpack src low */
        "punpcklbw      %%xmm0, %%xmm6          \n\t"   /* unpack dst low */
        "punpckhbw      %%xmm0, %%xmm3          \n\t"   /* unpack src high */
        "punpckhbw      %%xmm0, %%xmm4          \n\t"   /* unpack dst high */

        /*
            xmm5 (src_l)
            xmm6 (dst_l)

                    00 U4 00 V4 00 U3 00 V3 00 U2 00 V2 00 U1 00 V1

            xmm3 (src_u)
            xmm4 (dst_u)

                    00 U8 00 V8 00 U7 00 V7 00 U6 00 V6 00 U5 00 V5
        */

        "movdqa         %%xmm2, %%xmm7          \n\t"   /* dub alpha */
        "movdqa         %%xmm2, %%xmm8          \n\t"   /* dub alpha */
        "movlhps        %%xmm7, %%xmm7          \n\t"   /* dub low */
        "movhlps        %%xmm8, %%xmm8          \n\t"   /* dub high */

        /*
            xmm7 (src alpha)

                    00 A4 00 A3 00 A2 00 A1 00 A4 00 A3 00 A2 00 A1
            xmm8 (src alpha)

                    00 A8 00 A7 00 A6 00 A5 00 A8 00 A7 00 A6 00 A5
        */

        "pshuflw        $0x50, %%xmm7, %%xmm7     \n\t"
        "pshuflw        $0x50, %%xmm8, %%xmm8     \n\t"
        "pshufhw        $0xFA, %%xmm7, %%xmm7     \n\t"
        "pshufhw        $0xFA, %%xmm8, %%xmm8     \n\t"

        /*
            xmm7 (src alpha lower)

                    00 A4 00 A4 00 A3 00 A3 00 A2 00 A2 00 A1 00 A1

            xmm8 (src alpha upper)
                    00 A8 00 A8 00 A7 00 A7 00 A6 00 A6 00 A5 00 A5
        */


        /*
            DST = SRC * ALPHA + DST * (0xFF - ALPHA)
                SRC * ALPHA + DST * 0xFF - DST * ALPHA
                (SRC - DST) * ALPHA + DST * 0xFF

        */
        "psubw          %%xmm4, %%xmm3          \n\t"   /* src = src - dst */
        "psubw          %%xmm6, %%xmm5          \n\t"
        "pmullw         %%xmm8, %%xmm3          \n\t"   /* src = src * alpha */
        "pmullw         %%xmm7, %%xmm5          \n\t"
        "pmullw         %%xmm9, %%xmm4          \n\t"   /* dst = dst * 0xFF */
        "pmullw         %%xmm9, %%xmm6          \n\t"
        "paddw          %%xmm3, %%xmm4          \n\t"   /* dst = dst + src */
        "paddw          %%xmm5, %%xmm6          \n\t"
        "movdqa         %%xmm4, %%xmm3          \n\t"   /* dst = ((dst >> 8) + dst + 128) >> 8 */
        "movdqa         %%xmm6, %%xmm5          \n\t"
        "psrlw          $8, %%xmm4              \n\t"
        "psrlw          $8, %%xmm6              \n\t"
        "paddw          %%xmm3, %%xmm4          \n\t"
        "paddw          %%xmm5, %%xmm6          \n\t"
        "paddw          %%xmm10, %%xmm4         \n\t"
        "paddw          %%xmm10, %%xmm6         \n\t"
        "psrlw          $8, %%xmm4              \n\t"
        "psrlw          $8, %%xmm6              \n\t"
//        "pminsw         %%xmm9, %%xmm4          \n\t"   /* clamp values */
//        "pminsw         %%xmm9, %%xmm6          \n\t"

        /*
            xmm6 (dst_l)

                    00 U4 00 V4 00 U3 00 V3 00 U2 00 V2 00 U1 00 V1

            xmm4 (dst_u)

                    00 U8 00 V8 00 U7 00 V7 00 U6 00 V6 00 U5 00 V5
        */
        "packuswb       %%xmm4, %%xmm6          \n\t"

        /*
            xmm6 (dst)

                    U8 V8 U7 V7 U6 V6 U5 V5 U4 V4 U3 V3 U2 V2 U1 V1
        */
        "movdqu         %%xmm6, (%[dest])       \n\t"   /* store dst */

        /*
            increment pointers
        */
        "add            $0x08, %[src_a]         \n\t"
        "add            $0x08, %[dest_a]        \n\t"
        "add            $0x10, %[src]           \n\t"
        "add            $0x10, %[dest]          \n\t"

        "dec            %[width]                \n\t"
        "jnz            loop_start              \n\t"

        :
        : [weight] "r" (weight >> 8), [src_a] "r" (src_a), [src] "r" (src), [dest] "r" (dest), [const1] "r" (const1) , [dest_a] "r" (dest_a), [width] "r" (width / 8), [const2] "r" (const2)
        //: "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9", "memory"
    );
};
