	.file "composite_line_yuv_mmx"
	.version "01.01"
	
gcc2_compiled.:
.data

.text
	.align 16

#if !defined(__MINGW32__) && !defined(__CYGWIN__)
.globl composite_line_yuv_mmx
	.type	 composite_line_yuv_mmx,@function
composite_line_yuv_mmx:
#else
.globl _composite_line_yuv_mmx
_composite_line_yuv_mmx:
#endif

/*
 * Arguments
 *		
 * dest:	     8(%ebp)		%esi
 * src:         12(%ebp)
 * width_src:   16(%ebp)	
 * alpha:       20(%ebp)	
 * weight:      24(%ebp)	
 * luma:        28(%ebp)
 * softness:    32(%ebp)
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

/* Initialise */
	movl 8(%ebp), %esi	      # get dest
	movl $0, %edx			  # j = 0
	
.loop:

	movl $0xffff, %ecx           # a = 255
	cmpl $0, 20(%ebp)         # if alpha == NULL
	je .noalpha
	movl 20(%ebp), %edi       # a = alpha[ j ]
	movb (%edi,%edx), %cl
.noalpha:
	movl %ecx, -24(%ebp)      # save ecx

	movl 24(%ebp), %eax       # mix = weight
	cmpl $0, 28(%ebp)         # if luma == NULL
	je .noluma
	movl 28(%ebp), %edi       # mix = ...
	movl %edx, %ebx
	sall $1, %ebx
	movw (%edi,%ebx), %bx # luma[ j*2 ]
	cmpl %ebx, %eax
	jl .luma0
	movl %ebx, %ecx
	addl 32(%ebp), %ecx       # + softness
	cmpl %ecx, %eax
	jge .luma1
	/* TODO: linear interpolate between edges */
	subw %bx, %ax
	sall $8, %eax
	subw %bx, %cx
	movl %edx, %ebx
	divw %cx
	movl %ebx, %edx
	jmp .noluma
.luma0:
	movl $0, %eax
	jmp .noluma
.luma1:
	movl $0xffff, %eax
.noluma:
	shrl $8, %eax

	movl %edx, %ebx           # edx will be destroyed by mulw
	movl -24(%ebp), %ecx      # restore ecx
	mull %ecx                  # mix = mix * a...
	movl %ebx, %edx           # restore edx
	shrl $8, %eax             # >>8
	andl $0xff, %eax
	
/* put alpha and (1-alpha) into mm0 */
/* 0 aa 0 1-a 0 aa 0 1-a */

	/* duplicate word */
	movl %eax, %ecx
	shll $16, %ecx
	orl %eax, %ecx
	
	movd %ecx, %mm1
	
	/* (1 << 16) - mix */
	movl $0x000000ff, %ecx
	subl %eax, %ecx
	andl $0xff, %ecx
	
	/* duplicate word */
	movl %ecx, %eax
	shll $16, %eax
	orl %eax, %ecx
	
	movd %ecx, %mm0
	
	/* unpack words into double words */
	punpcklwd %mm1, %mm0
	
/* put src yuv and dest yuv into mm1 */
/* 0 UVs 0 UVd 0 Ys 0 Yd */

	movl 12(%ebp), %edi       # get src
	movb (%edi), %cl
	shll $8, %ecx
	movb 1(%edi), %al
	shll $24, %eax
	orl %eax, %ecx
	
	movb (%esi), %al         # get dest
	orl %eax, %ecx
	movb 1(%esi), %al
	shll $16, %eax
	orl %eax, %ecx
	
	movd %ecx, %mm1
	punpcklbw %mm4, %mm1
	
/* alpha composite */
	pmaddwd %mm1, %mm0
	psrld $8, %mm0

/* store result */
	movd %mm0, %eax
	movb %al, (%esi)
	pextrw $2, %mm0, %eax
		movl $128, %eax
	movb %al, 1(%esi)

/* for..next */
	addl $1, %edx             # j++
	cmpl 16(%ebp), %edx       # if ( j == width_src )
	jge .out
	
	addl $2, %esi
	addl $2, 12(%ebp)
	
	jmp .loop

.out:
	emms
	leal -40(%ebp),%esp
	popl %ebx
	popl %esi
	popl %edi
	movl %ebp,%esp
	popl %ebp
	ret


/********************************************/

.align 8
#if !defined(__MINGW32__) && !defined(__CYGWIN__)	
.globl composite_have_mmx
	.type	 composite_have_mmx,@function
composite_have_mmx:
#else
.globl _composite_have_mmx
_composite_have_mmx:
#endif
	
	push	%ebx

# Check if bit 21 in flags word is writeable

	pushfl	
	popl	%eax
	movl	%eax,%ebx
	xorl	$0x00200000, %eax
	pushl   %eax
	popfl
	pushfl
	popl	%eax

	cmpl	%eax, %ebx

	je .notfound

# OK, we have CPUID

	movl	$1, %eax
	cpuid
	
	test	$0x00800000, %edx
	jz	.notfound

	movl	$1, %eax
	jmp	.out2

.notfound:
	movl  	$0, %eax
.out2:	
	popl	%ebx
	ret
