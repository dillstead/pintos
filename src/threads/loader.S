#include "mmu.h"
	
###################################################################################
# ENTRY POINT	
#   This code should be stored in the first sector of the hard disk.  When the  
#   BIOS runs, it loads this code at physical address 0x7c00 - 0x7e00 (512 bytes).
#   Then jumps to the beginning of it, in real-mode (BIOS runs in real mode).  
#	
# This code switches into protected mode (32-bit mode) so that all of
# memory can accessed, then calls into C.
###################################################################################
	
.globl start				# Entry point	
start:	.code16				# This runs in real mode
	cli				# Disable interrupts
	cld				# String ops inc
	xorw %ax,%ax			# Zero
	movw %ax,%es			# Address
	movw %ax,%ds			#  data
	movw %ax,%ss			# Set up
	movw $start,%sp         	#  stack (grows down)
	
#### Enable A20:
####   Address line 20 is tied to low when the machine boots, 
####   obviously this a bit of a drag, such as when trying to
####   address memory above 1MB.  This code undoes this.
	
1:	inb $0x64,%al		# Get status
	testb $0x2,%al		# Busy?
	jnz 1b			# Yes
	movb $0xd1,%al		# Command: Write
	outb %al,$0x64		#  output port
2:	inb $0x64,%al		# Get status
	testb $0x2,%al		# Busy?
	jnz 2b			# Yes
	movb $0xdf,%al		# Enable
	outb %al,$0x60		#  A20

#### Get memory size, via interrupt 15h function 88h.
#### Returns CF clear if successful, with AX = (kB of physical memory) - 1024.
#### This only works for memory sizes <= 65 MB, which should be fine for our purposes.
	
	movb	$0x88,%ah
	int	$0x15
	jc	panic			# Carry flag set on error
	addl	$1024,%eax		# Total kB
	shrl	$2,%eax			# Total 4 kB pages
	movl	%eax, ram_pages
	
#### switch from real to protected mode	
####     The segments in GDT allow all of physical memory to be accessed.
####     Furthermore, the segments have base addresses of 0, so that the
####     segment translation is a NOP, ie. virtual addresses are identical to
####     their physical addresses.  With this setup, it appears to this code
####     that it is running directly on physical memory.
	
	cli			# Mandatory since we dont set up an IDT
	lgdt gdtdesc		# load GDT -- mandatory in protected mode
	movl %cr0, %eax		# turn on protected mode
	orl $CR0_PE, %eax	# 
	movl %eax, %cr0		# 
	### CPU magic: jump to relocation, flush prefetch queue, and reload %cs
	### Has the effect of just jmp to the next instruction, but simultaneous
	### loads CS with $PROT_MODE_CSEG.
	ljmp $SEL_KCSEG, $protcseg
	
#### We are in protected mode in a 32-bit segment (hence the .code32)
protcseg:
	.code32
	movw $SEL_KDSEG, %ax	# set up data segment registers
	movw %ax, %ds		
	movw %ax, %es		
	movw %ax, %fs		
	movw %ax, %gs		
	movw %ax, %ss

#### Load kernel at 1 MB by frobbing the IDE controller directly.

	movl $1, %ebx
	movl $0x100000, %edi
read_sector:

	### Poll status register while controller busy.
	movl $0x1f7, %edx
1:	inb %dx, %al
	testb $0x80, %al
	jnz 1b

	### Read a single sector.
	movl $0x1f2, %edx
	movb $1, %al
	outb %al, %dx

	### Sector number to write in low 28 bits.
	### LBA mode, device 0 in top 4 bits.
	movl %ebx, %eax
	andl $0x0fffffff, %eax
	orl $0xe0000000, %eax

	### Dump %eax to ports 0x1f3...0x1f6.
	movl $4, %ecx
2:	incw %dx
	outb %al, %dx
	shrl $8, %eax
	loop 2b

	### READ command to command register.
	incw %dx
	movb $0x20, %al
	outb %al, %dx

	### Poll status register while controller busy.
1:	inb %dx, %al
	testb $0x80, %al
	jnz 1b

	### Poll status register until data ready.
1:	inb %dx, %al
	testb $0x08, %al
	jz 1b

	### Transfer sector.
	movl $512 / 4, %ecx
	movl $0x1f0, %edx
	rep insl

	### Next sector.
	incl %ebx
	cmpl $KERNEL_LOAD_PAGES*8 + 1, %ebx
	jnz read_sector

##### Create temporary PDE and PTE and set page directory pointer
##### FIXME? We could use a single 4 MB page instead of 1024 4 kB pages.

	movl $0x10000, %edi
	movl %edi, %cr3
	subl %eax, %eax
	movl $0x400, %ecx
	rep stosl
	movl $0x11000 | PG_U | PG_W | PG_P, %eax
	movl %eax, 0x10000
	movl %eax, 0x10c00
	movl $PG_U | PG_W | PG_P, %eax
	movl $0x400, %ecx
1:	stosl
	addl $0x1000, %eax
	loop 1b

##### Enable paging.

	movl %cr0, %eax
	orl $CR0_PG, %eax
	movl %eax, %cr0
	jmp 1f
1:
	
##### Jump to kernel entry point.

	movl $0xc0007c00, %esp
	movl $0xc0100000, %eax
	jmp *%eax

##### GDT

gdt:
	.quad 0x0000000000000000	# null seg
	.quad 0x00cf9a000000ffff	# code seg
	.quad 0x00cf92000000ffff        # data seg
	
gdtdesc:
	.word	0x17			# sizeof (gdt) - 1
	.long	gdt			# address gdt

##### Arrive here on error
panic:	jmp panic

##### Memory size in 4 kB pages.
	.org 0x200 - 8
ram_pages:
	.long 0

##### Boot-sector signature for BIOS inspection.
	.org 0x200 - 2
	.word 0xaa55
