	.file	"sphere.c"
	.text
	.p2align 4
	.globl	sphere
	.type	sphere, @function
sphere:
.LFB39:
	.cfi_startproc
	endbr64
	testq	%rsi, %rsi
	je	.L7
	cmpq	$1, %rsi
	je	.L8
	movq	%rsi, %rdx
	movq	%rdi, %rax
	pxor	%xmm0, %xmm0
	shrq	%rdx
	salq	$4, %rdx
	addq	%rdi, %rdx
	.p2align 4,,10
	.p2align 3
.L4:
	movupd	(%rax), %xmm1
	addq	$16, %rax
	mulpd	%xmm1, %xmm1
	addsd	%xmm1, %xmm0
	unpckhpd	%xmm1, %xmm1
	addsd	%xmm1, %xmm0
	cmpq	%rax, %rdx
	jne	.L4
	movq	%rsi, %rax
	andq	$-2, %rax
	andl	$1, %esi
	je	.L11
.L3:
	movsd	(%rdi,%rax,8), %xmm1
	mulsd	%xmm1, %xmm1
	addsd	%xmm1, %xmm0
	ret
	.p2align 4,,10
	.p2align 3
.L11:
	ret
	.p2align 4,,10
	.p2align 3
.L7:
	pxor	%xmm0, %xmm0
	ret
.L8:
	xorl	%eax, %eax
	pxor	%xmm0, %xmm0
	jmp	.L3
	.cfi_endproc
.LFE39:
	.size	sphere, .-sphere
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC1:
	.string	"%f\n"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB40:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r15
	.cfi_offset 15, -24
	movq	%rsi, %r15
	pushq	%r14
	.cfi_offset 14, -32
	leal	-1(%rdi), %r14d
	pushq	%r13
	movslq	%r14d, %r14
	pushq	%r12
	pushq	%rbx
	subq	$40, %rsp
	.cfi_offset 13, -40
	.cfi_offset 12, -48
	.cfi_offset 3, -56
	movl	%edi, -68(%rbp)
	movq	%rsp, %rsi
	movq	%fs:40, %rdx
	movq	%rdx, -56(%rbp)
	xorl	%edx, %edx
	leaq	15(,%r14,8), %rdx
	movq	%rdx, %rcx
	andq	$-4096, %rdx
	subq	%rdx, %rsi
	andq	$-16, %rcx
	cmpq	%rsi, %rsp
	je	.L14
.L31:
	subq	$4096, %rsp
	orq	$0, 4088(%rsp)
	cmpq	%rsi, %rsp
	jne	.L31
.L14:
	andl	$4095, %ecx
	subq	%rcx, %rsp
	testq	%rcx, %rcx
	jne	.L32
.L15:
	movq	%rsp, %r12
	testq	%r14, %r14
	je	.L23
	movslq	-68(%rbp), %r13
	xorl	%ebx, %ebx
	subq	$1, %r13
	.p2align 4,,10
	.p2align 3
.L17:
	addq	$1, %rbx
	xorl	%esi, %esi
	movq	(%r15,%rbx,8), %rdi
	call	strtod@PLT
	movsd	%xmm0, -8(%r12,%rbx,8)
	cmpq	%r13, %rbx
	jne	.L17
	cmpl	$2, -68(%rbp)
	je	.L24
	movq	%r14, %rdx
	movq	%r12, %rax
	pxor	%xmm0, %xmm0
	shrq	%rdx
	salq	$4, %rdx
	addq	%r12, %rdx
	.p2align 4,,10
	.p2align 3
.L19:
	movupd	(%rax), %xmm1
	addq	$16, %rax
	mulpd	%xmm1, %xmm1
	addsd	%xmm1, %xmm0
	unpckhpd	%xmm1, %xmm1
	addsd	%xmm1, %xmm0
	cmpq	%rax, %rdx
	jne	.L19
	movq	%r14, %rax
	andq	$-2, %rax
	andl	$1, %r14d
	je	.L16
.L18:
	movsd	(%r12,%rax,8), %xmm1
	mulsd	%xmm1, %xmm1
	addsd	%xmm1, %xmm0
.L16:
	leaq	.LC1(%rip), %rsi
	movl	$1, %edi
	movl	$1, %eax
	call	__printf_chk@PLT
	movq	-56(%rbp), %rax
	subq	%fs:40, %rax
	jne	.L33
	leaq	-40(%rbp), %rsp
	xorl	%eax, %eax
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	.cfi_remember_state
	.cfi_def_cfa 7, 8
	ret
.L32:
	.cfi_restore_state
	orq	$0, -8(%rsp,%rcx)
	jmp	.L15
.L23:
	pxor	%xmm0, %xmm0
	jmp	.L16
.L24:
	pxor	%xmm0, %xmm0
	xorl	%eax, %eax
	jmp	.L18
.L33:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE40:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
