.text
.syntax unified
.code 32

.globl __syscall1
__syscall1:
	mov r7, r0
	mov r0, r1
	swi #0
	bx lr

.globl _start
_start:
	bl __libc_main
	bx lr

	
