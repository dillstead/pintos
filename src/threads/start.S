# This module gets linked first, so that the kernel entry point
# is the very beginning of its binary image.  All we need to do is
# jump to the real entry point.

	.globl start
start:	call main

	# If main returns, spin.
1:	jmp 1b
