#! /usr/bin/perl

print <<'EOF';
#include "threads/loader.h"

	.data
	.intel_syntax noprefix
.globl intr_stubs
intr_stubs:
EOF

for $i (0...255) {
    $x = sprintf ("%02x", $i);
    print "\t.long intr${x}_stub\n";
}

print <<'EOF';

	.text
EOF

for $i (0...255) {
    $x = sprintf ("%02x", $i);
    print ".globl intr${x}_stub\n";
    print "intr${x}_stub:\n";
    print "\tpush 0\n"
	if ($i != 8 && $i != 10 && $i != 11
	    && $i != 13 && $i != 14 && $i != 17);
    print "\tpush 0x$x\n";
    print "\tjmp intr_entry\n";
}

print <<'EOF';
intr_entry:
	# Save caller's registers.
	push ds
	push es
	push fs
	push gs
	pusha

	# Set up kernel environment.
	cld
	mov eax, SEL_KDSEG
	mov ds, eax
	mov es, eax

	# Call interrupt handler.
	push esp
.globl intr_handler
	call intr_handler
	add esp, 4

.globl intr_exit
intr_exit:
	# Restore caller's registers.
	popa
	pop gs
	pop fs
	pop es
	pop ds
	add esp, 8

        # Return to caller.
	iret
EOF
