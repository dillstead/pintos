.globl syscall
syscall:
	int $0x30
	retl
