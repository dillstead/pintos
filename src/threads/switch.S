#include "switch.h"

        .globl switch_threads 
switch_threads:
	# Save caller's register state.
	# Note that the SVR4 ABI allows us to destroy %eax, %ecx, %edx.
	# This stack frame must match the one set up by thread_create().
	pushl %ebx
	pushl %ebp
	pushl %esi
	pushl %edi

	# Get offsetof (struct thread, stack).
	.globl thread_stack_ofs
	mov thread_stack_ofs, %edx

	# Save current stack pointer to old thread's stack, if any.
	movl SWITCH_CUR(%esp), %eax
	test %eax, %eax
	jz 1f
	movl %esp, (%eax,%edx,1)
1:

	# Restore stack pointer from new thread's stack.
	movl SWITCH_NEXT(%esp), %ecx
	movl (%ecx,%edx,1), %esp

	# Restore caller's register state.
	popl %edi
	popl %esi
	popl %ebp
	popl %ebx
        ret

	.globl switch_thunk
switch_thunk:
	addl $8, %esp
	ret
