#include "threads/switch.h"

#### struct thread *switch_threads (struct thread *cur, struct thread *next);
####
#### Switches from CUR, which must be the running thread, to NEXT,
#### which must also be running switch_threads(), returning CUR in
#### NEXT's context.
####
#### This function works by assuming that the thread we're switching
#### into is also running switch_threads().  Thus, all it has to do is
#### preserve a few registers on the stack, then switch stacks and
#### restore the registers.  As part of switching stacks we record the
#### current stack pointer in CUR's thread structure.

	.intel_syntax noprefix

.globl switch_threads 
switch_threads:
	# Save caller's register state.
	#
	# Note that the SVR4 ABI allows us to destroy eax, ecx, edx,
	# but requires us to preserve ebx, ebp, esi, edi.  See
	# [SysV-ABI-386] pages 3-11 and 3-12 for details.
	#
	# This stack frame must match the one set up by thread_create().
	push ebx
	push ebp
	push esi
	push edi

	# Get offsetof (struct thread, stack).
.globl thread_stack_ofs
	mov edx, thread_stack_ofs

	# Save current stack pointer to old thread's stack, if any.
	mov eax, SWITCH_CUR[esp]
	test eax, eax
	jz 1f
	mov [eax + edx], esp
1:

	# Restore stack pointer from new thread's stack.
	mov ecx, SWITCH_NEXT[esp]
	mov esp, [ecx + edx]

	# Restore caller's register state.
	pop edi
	pop esi
	pop ebp
	pop ebx
        ret

.globl switch_entry
switch_entry:
	# Discard switch_threads() arguments.
	add esp, 8

	# Call schedule_tail(prev).
	push eax
.globl schedule_tail
	call schedule_tail
	add esp, 4

	# Start thread proper.
	ret
