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

/* For the body of this loop, %mm0, %mm1, %mm2, %mm3 hold the 4 adjoining
 * points we are interpolating between, as:
 *
 *  0000000000UV00YY
 */

/* Load initial values into %mm1, %mm3 */

	/* x_scaled = ( x >> 16 ) * stride */
	movl %ebx, %edx
	sarl $16,%edx
	sall $1, %edx

	/* load from src0 */
	movl 16(%ebp), %edi
	movzbl (%edi,%edx), %ecx

	/* x_aligned = x_scaled divided by 2 and multiplied by 4 */
	movl %ebx, %edx
	sarl $17, %edx
	sall $2, %edx

	/* uv_index = ( ( dest_x & 1 ) << 1 ) + 1; */
	movl 36(%ebp), %eax
	andl $1, %eax
	sall $1, %eax
	addl %eax, %edx
	movzbl 1(%edi,%edx), %eax
	shll $8, %eax
	orl %eax, %ecx

	movd %ecx, %mm1
	pxor %mm4, %mm4
	punpcklbw %mm4, %mm1

	/* x_scaled = ( x >> 16 ) * stride */
	movl %ebx, %edx
	sarl $16, %edx
	sall $1, %edx

	/* load from src1 */
	movl 20(%ebp), %edi
	movzbl (%edi,%edx), %ecx

	/* x_aligned = x_scaled divided by 2 and multiplied by 4 */
	movl %ebx, %edx
	sarl $17, %edx
	sall $2, %edx

	/* uv_index = ( ( dest_x & 1 ) << 1 ) + 1; */
	movl 36(%ebp), %eax
	andl $1, %eax
	sall $1, %eax
	addl %eax, %edx
	movzbl (%edi,%edx), %eax
	shll $8, %eax
	orl %eax, %ecx

	movd %ecx, %mm3
	punpcklbw %mm4, %mm3

	/* dest_x++; */
	movl 36(%ebp), %eax
	addl $1, %eax
	movl %eax, 36(%ebp)

	/* x_scaled = x >> 16 */
	addl $65536, %ebx
	movl %ebx, %edx
	sarl $16, %edx
	movl %edx, -24(%ebp)

	jmp .newx
	.p2align 4,,7
.loop:
/* short *pixel_weights = weights + ((x >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) * n_x * n_y
 *                                             16             4                  0xf            2     2
 */
	movl %ebx,%eax
	andl $0xf000,%eax
	shrl $7,%eax

/* At this point, %edi holds weights. Load the 4 weights into %mm4,%mm5,%mm6,%mm7, multiply and
 * accumulate.
 */
	movq (%edi,%eax),%mm4
	pmullw %mm0,%mm4
	movq 8(%edi,%eax),%mm5
	pmullw %mm1,%mm5
	movq 16(%edi,%eax),%mm6
	movq 24(%edi,%eax),%mm7
	pmullw %mm2,%mm6
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

	movb %al, 0(%esi)        # *dest = y
	shrl $8, %eax
	movb %al, 1(%esi)        # *dest = uv

	addl $2, %esi            # dest += 2

	cmpl %esi,28(%ebp)       # if dest == dest_end ?
	je   .out                # then exit

	movl 36(%ebp), %eax      # get dest_x
	addl $1, %eax            # dest_x++
	movl %eax, 36(%ebp)      # put dest_x

	addl 24(%ebp), %ebx      # x += x_step

	movl %ebx, %edx          # x_scaled = x ...
	sarl $16, %edx           # >> 16
	movl %edx, -24(%ebp)     # save x_scaled

.newx:

/*
 * Load the two new values into %mm1, %mm3, move old values into %mm0, %mm2
 */
	movq %mm1, %mm0
	movq %mm3, %mm2

	sall $1, %edx            # x_scaled *= channels

	movl 16(%ebp), %edi      # get src0
	movzbl (%edi,%edx), %ecx # y = src0[ x_scaled ]

	sarl $2, %edx            # x_aligned = ( x_scaled / channels ) >> 1 ...
	sall $2, %edx            # << 2

	movl 36(%ebp), %eax      # uv_index = dest_x ...
		#pushl %eax
	andl $1, %eax            # ( dest_x & 1 ) ...
	sall $1, %eax            # << 1
	addl %eax, %edx          # x_aligned += uv_index
		#pushl %edx
		#pushl $MSG
		#call printf
		#popl %edx
		#popl %edx
		#popl %edx
	movzbl 1(%edi,%edx), %eax # uv = src0[ x_aligned + 1 ]
	shll $8, %eax            # store uv
	orl %eax, %ecx

	movd %ecx, %mm1          # move to mmx1
	punpcklbw %mm4, %mm1

	movl %ebx, %edx          # x_scaled = x ...
	sarl $16, %edx           # >> 16
	sall $1, %edx            # x_scaled *= channels

	movl 20(%ebp), %edi      # get src1
	movzbl (%edi,%edx), %ecx # y = src1[ x_scaled ]

	sarl $2, %edx            # x_aligned = ( x_scaled / channels ) >> 1 ...
	sall $2, %edx            # << 2

	movl 36(%ebp), %eax      # uv_index = dest_x ...
	andl $1, %eax            # ( dest_x & 1 ) ...
	sall $1, %eax            # << 1
	addl %eax, %edx          # x_aligned += uv_index
	movzbl 1(%edi,%edx), %eax # uv = src1[ x_aligned + 1 ]
	shll $8, %eax            # store uv
	orl %eax, %ecx

	movd %ecx, %mm3          # move to mmx3
	punpcklbw %mm4, %mm3

	movl 8(%ebp), %edi       # get weights pointer
	
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
