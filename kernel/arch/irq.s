.section .text

.macro ISR index
	.global _isr\index
	.type _isr\index, @function
	_isr\index:
		pushq $0
		pushq $\index
		jmp isr_common
.endm

.macro ISR_ERR index
	.global _isr\index
	.type _isr\index, @function
	_isr\index:
		pushq $\index
		jmp isr_common
.endm

.macro IRQ index byte
	.global _irq\index
	.type _irq\index, @function
	_irq\index:
		pushq $0
		pushq $\byte
		jmp isr_common
.endm

ISR 0
ISR 1
ISR 2
ISR 3
ISR 4
ISR 5
ISR 6
ISR 7
ISR_ERR 8
ISR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR 15
ISR 16
ISR 17
ISR 18
ISR 19
ISR 20
ISR 21
ISR 22
ISR 23
ISR 24
ISR 25
ISR 26
ISR 27
ISR 28
ISR 29
ISR 30
ISR 31
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

.extern isr_handler
.type isr_handler, @function

.global isr_common
isr_common:
	pushq %rax
	pushq %rbx
	pushq %rcx
	pushq %rdx
	pushq %rbp
	pushq %rsi
	pushq %rdi
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	mov %rsp, %rdi
	call isr_handler

	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
    popq %rdi
	popq %rsi
	popq %rbp
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax

	add $16, %rsp
    iretq

.global switch_task
.type unlock, @function
.extern unlock
.extern spinlock
switch_task:
	push %rbx
	push %rbp
	push %r12
	push %r13
	push %r14
	push %r15

	mov %cr3, %r11
	mov %r11, 16(%rsi)
	mov %rsp, 8(%rsi)

	mov 16(%rdi), %r11
	mov %r11, %cr3
	mov 8(%rdi), %rsp

	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbp
	pop %rbx

	mov $spinlock, %rdi
	call unlock
	ret
