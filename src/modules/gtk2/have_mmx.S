	.file	"have_mmx.S"
	.version	"01.01"
gcc2_compiled.:
.text
	.align 16

#if !defined(__MINGW32__) && !defined(__CYGWIN__)	

.globl pixops_have_mmx
	.type	 pixops_have_mmx,@function
pixops_have_mmx:

#else

.globl _pixops_have_mmx
_pixops_have_mmx:

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
	jmp	.out

.notfound:
	movl  	$0, %eax
.out:	
	popl	%ebx
	ret

