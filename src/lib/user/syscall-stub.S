.globl syscall
syscall:
	popl %ecx
	int $0x30
	jmpl *%ecx
