	.intel_syntax noprefix
	.globl syscall
syscall:
	pop ecx
	int 0x30
	jmp ecx
