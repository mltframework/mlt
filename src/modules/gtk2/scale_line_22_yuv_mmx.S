	.file	"scale_line_22_yuv_mmx.S"
	.version	"01.01"

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
 * weights:	 8(%ebp)
 * p (dest):    12(%ebp)	%esi
 * q1 (src0):   16(%ebp)	
 * q2 (src1):   20(%ebp)	
 * xstep:       24(%ebp)	
 * p_end:       28(%ebp)
 * xinit:       32(%ebp)
 * destx:       36(%ebp)
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

	addl $65536, %ebx
	.p2align 4,,7
	pxor %mm4, %mm4

/* For the body of this loop, %mm0, %mm1, %mm2, %mm3 hold the 4 adjoining
 * points we are interpolating between, as:
 *
 *  00UV00Y200UV00Y1
 */

.loop:

/*
 * Load current values from pixel 1
 */
	movl %ebx, %edx          # x_scaled = x ...
	sarl $16, %edx           # >> 16
	sall $1, %edx            # x_scaled *= channels

	movl 16(%ebp), %edi      # get src0
	movzbl 2(%edi,%edx), %ecx # next y = src0[ x_scaled + 2 ]
	/* wish we had a register for this */
	movl %ecx, -24(%ebp)     # save next y
	movzbl (%edi,%edx), %ecx # y = src0[ x_scaled ]

	sarl $2, %edx            # x_aligned = ( x_scaled / channels ) >> 1 ...
	sall $2, %edx            # << 2

	movl 36(%ebp), %eax      # uv_index = dest_x ...
	andl $1, %eax            # ( dest_x & 1 ) ...
	sall $1, %eax            # << 1
	addl %eax, %edx          # x_aligned += uv_index

	movzbl 1(%edi,%edx), %eax # uv = src0[ x_aligned + 1 ]
	shll $8, %eax           # position uv
	orl %eax, %ecx           # store uv

	movd %ecx, %mm0          # move to mmx0
	punpcklbw %mm4, %mm0

	movl -24(%ebp), %ecx     # restore next y
	orl %eax, %ecx           # store uv

	movd %ecx, %mm1          # move to mmx1
	punpcklbw %mm4, %mm1

	movl 20(%ebp), %edi      # get src1

	/* do u/v first since we already have x_aligned */
	movzbl 1(%edi,%edx), %eax # uv = src1[ x_aligned + 1 ]
	shll $8, %eax            # position uv

	/* which is faster? 2 moves in and out of memory, or
       1 move between registers and 2 shifts? I wager the latter. */
	movl %ebx, %edx          # x_scaled = x ...
	sarl $16, %edx           # >> 16
	sall $1, %edx            # x_scaled *= channels

	movzbl 2(%edi,%edx), %ecx # next y = src1[ x_scaled + 2 ]
	movl %eax, -24(%ebp)     # save next y
	movzbl (%edi,%edx), %ecx # y = src1[ x_scaled ]
	orl %eax, %ecx           # store uv

	movd %ecx, %mm2          # move to mmx2
	punpcklbw %mm4, %mm2

	movl -24(%ebp), %ecx     # restore next y
	orl %eax, %ecx           # store uv

	movd %ecx, %mm3          # move to mmx3
	punpcklbw %mm4, %mm3

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

	movb %al, 0(%esi)        # dest[ 0 ] = y
	shrl $8, %eax
	movb %al, 1(%esi)        # dest[ 1 ] = uv

	addl $2, %esi            # dest += 2

	cmpl %esi,28(%ebp)       # if dest == dest_end
	je   .out                # then exit

	addl 24(%ebp), %ebx      # x += x_step
	addl $1, 36(%ebp)        # dest_x++

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
