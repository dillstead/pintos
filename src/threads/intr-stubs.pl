#! /usr/bin/perl

print <<'EOF';
#include "mmu.h"

	.globl intr_stubs
intr_stubs:
EOF

for $i (0...255) {
    $x = sprintf ("%02x", $i);
    print "\t.long intr${x}_stub\n";
}
print "\n";

for $i (0...255) {
    $x = sprintf ("%02x", $i);
    print "\t.globl intr${x}_stub\n";
    print "intr${x}_stub:\n";
    print "\tpushl \$0\n" if $i != 8 && $i != 10 && $i != 11 && $i != 13 && $i != 14 && $i != 17;
    print "\tpushl \$0x$x\n";
    print "\tjmp intr_entry\n";
}

print <<'EOF';
intr_entry:
	# Save caller's registers.
	pushl %ds
	pushl %es
	pushal

	# Set up kernel environment.
	cld
	movl $SEL_KDSEG, %eax
	movl %eax, %ds
	movl %eax, %es

	# Call handler.
	pushl %esp
	.globl intr_handler
	call intr_handler
	addl $4, %esp

	# Restore caller's registers.
	popal
	popl %es
	popl %ds
	addl $8, %esp
	iret
EOF
