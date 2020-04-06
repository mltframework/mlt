/*
 * scale_line_22_yuv_mmx.S -- scale line in YUY2 format
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
	.file	"scale_line_22_yuv_mmx.S"
	.version	"01.01"
#if !defined(__MINGW32__) && !defined(__CYGWIN__)
	.section .note.GNU-stack,"",%progbits
#endif

.extern printf

gcc2_compiled.:
.data
MSG: .ascii "scale_line_22_yuv_mmx: %d %d\n"

.text
	.align 16

#if !defined(__MINGW32__) && !defined(__CYGWIN__)	

.globl pixops_scale_line_22_yuv_mmx
	.type	 pixops_scale_line_22_yuv_mmx,@function
pixops_scale_line_22_yuv_mmx:
	
#else
	
.globl _pixops_scale_line_22_yuv_mmx
_pixops_scale_line_22_yuv_mmx:
	
#endif
/*
 * Arguments
 *		
 * weights:	     8(%ebp)
 * p (dest):    12(%ebp)	%esi
 * q1 (src0):   16(%ebp)	
 * q2 (src1):   20(%ebp)	
 * xstep:       24(%ebp)	
 * p_end:       28(%ebp)
 * xinit:       32(%ebp)
 * dest_x:      36(%ebp)
 *
 */

/*
 * Function call entry
 */
	pushl %ebp
	movl %esp,%ebp
	subl $28,%esp
	pushl %edi
	pushl %esi
	pushl %ebx
/* Locals:
 * int x                      %ebx
 * int x_scaled             -24(%ebp)
 * int dest_x               36(%ebp)
 */

/*
 * Setup
 */
/* Initialize variables */
	movl 36(%ebp),%eax # destx
	movl %eax,36(%ebp)
	movl 32(%ebp),%ebx # x
	movl 12(%ebp),%esi # dest

	cmpl 28(%ebp),%esi # dest == dest_end ?
	jnb  .out

/* For the body of this loop, %mm0, %mm1, %mm2, %mm3 hold the 4 adjoining
 * points we are interpolating between, as:
 *
 *  00VV00Y200UU00Y1
 */

	pxor %mm4, %mm4
/*
 * Load next component values into mm1 (src0) and mm3 (src1)
 */
	movl %ebx, %eax          # x_scaled
	sarl $15, %eax
	andl $0xfffffffe, %eax
	movl %eax, %edx          # x_aligned
	andl $0xfffffffc, %edx

	movl 16(%ebp), %edi      # get src0
	movl (%edi,%eax), %ecx   # get y
	andl $0x00ff00ff, %ecx   # mask off y
	movl (%edi,%edx), %eax   # get uv
	andl $0xff00ff00, %eax   # mask off uv
	orl %eax, %ecx           # composite y, uv
	movd %ecx, %mm1          # move to mmx1
	punpcklbw %mm4, %mm1

	movl 20(%ebp), %edi      # get src1
	movl (%edi,%edx), %ecx   # get y
	andl $0x00ff00ff, %ecx   # mask off y
	movl (%edi,%edx), %eax   # get uv
	andl $0xff00ff00, %eax   # mask off uv
	orl %eax, %ecx           # composite y, uv
	movd %ecx, %mm3          # move to mmx3
	punpcklbw %mm4, %mm3

	jmp .newx

	.p2align 4,,7
.loop:

/* short *pixel_weights = weights + ((x >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) * n_x * n_y
 *                                             16             4                  0xf            2     2
 */
	movl 8(%ebp), %edi       # get weights pointer
	movl %ebx, %eax
	andl $0xf000, %eax
	shrl $7, %eax

/* At this point, %edi holds weights. Load the 4 weights into 
 * %mm4,%mm5,%mm6,%mm7, multiply and accumulate.
 */
	movq (%edi,%eax), %mm4
	pmullw %mm0, %mm4
	movq 8(%edi,%eax), %mm5
	pmullw %mm1, %mm5
	movq 16(%edi,%eax), %mm6
	pmullw %mm2,%mm6
	movq 24(%edi,%eax), %mm7
	pmullw %mm3,%mm7

	paddw %mm4, %mm5
	paddw %mm6, %mm7
	paddw %mm5, %mm7

/* %mm7 holds the accumulated sum. Compute (C + 0x80) / 256
 */
	pxor %mm4, %mm4
	movl $0x80808080, %eax
	movd %eax, %mm6
	punpcklbw %mm4, %mm6
	paddw %mm6, %mm7
	psrlw $8, %mm7

/* Pack into %eax and store result
 */
	packuswb %mm7, %mm7
	movd %mm7, %eax

	movb %al, (%esi)         # *dest = y
	
	movl 36(%ebp), %ecx      # get dest_x
	andl $1, %ecx            # select u or v
	sall $1, %ecx            # determine offset
	addl $1, %ecx            # relative to x_aligned
	sall $3, %ecx            # offset * 8 bits/byte

	movd %mm7, %eax
	shrl %cl, %eax
	movb %al, 1(%esi)        # *dest = uv

	addl $2, %esi            # dest += 2
	cmpl %esi,28(%ebp)       # if dest == dest_end
	je   .out                # then exit

	addl $1, 36(%ebp)        # dest_x++

.newx:

	addl 24(%ebp), %ebx      # x += x_step
/*
 * Load current component values into mm0 (src0) and mm2 (src1)
 */
	movq %mm1, %mm0
	movq %mm3, %mm2

/*
 * Load next component values into mm1 (src0) and mm3 (src1)
 */
	movl %ebx, %eax          # x_scaled
	sarl $15, %eax
	andl $0xfffffffe, %eax
	movl %eax, %edx          # x_aligned
	andl $0xfffffffc, %edx

	movl 16(%ebp), %edi      # get src0
	movl (%edi,%eax), %ecx   # get y
	andl $0x00ff00ff, %ecx   # mask off y
	movl (%edi,%edx), %eax   # get uv
	andl $0xff00ff00, %eax   # mask off uv
	orl %eax, %ecx           # composite y, uv
	movd %ecx, %mm1          # move to mmx1
	punpcklbw %mm4, %mm1

	movl 20(%ebp), %edi      # get src1
	movl (%edi,%edx), %ecx   # get y
	andl $0x00ff00ff, %ecx   # mask off y
	movl (%edi,%edx), %eax   # get uv
	andl $0xff00ff00, %eax   # mask off uv
	orl %eax, %ecx           # composite y, uv
	movd %ecx, %mm3          # move to mmx3
	punpcklbw %mm4, %mm3

	jmp .loop

.out:
	movl %esi,%eax
	emms
	leal -40(%ebp),%esp
	popl %ebx
	popl %esi
	popl %edi
	movl %ebp,%esp
	popl %ebp
	ret
