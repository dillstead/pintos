/* This file is derived from source code used in MIT's 6.828
   course.  The original copyright notice is reproduced in full
   below. */

/*
 * Copyright (C) 1997 Massachusetts Institute of Technology 
 *
 * This software is being provided by the copyright holders under the
 * following license. By obtaining, using and/or copying this software,
 * you agree that you have read, understood, and will comply with the
 * following terms and conditions:
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose and without fee or royalty is
 * hereby granted, provided that the full text of this NOTICE appears on
 * ALL copies of the software and documentation or portions thereof,
 * including modifications, that you make.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
 * REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE,
 * BUT NOT LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR
 * WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR
 * THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY
 * THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS. COPYRIGHT
 * HOLDERS WILL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE OR
 * DOCUMENTATION.
 *
 * The name and trademarks of copyright holders may NOT be used in
 * advertising or publicity pertaining to the software without specific,
 * written prior permission. Title to copyright in this software and any
 * associated documentation will at all times remain with copyright
 * holders. See the file AUTHORS which should have accompanied this software
 * for a list of all copyright holders.
 *
 * This file may be derived from previously copyrighted software. This
 * copyright applies only to those changes made by the copyright
 * holders listed in the AUTHORS file. The rest of this file is covered by
 * the copyright notices, if any, listed below.
 */

#include "loader.h"
#include "mmu.h"
	
##############################################################################
# Kernel loader.
#
# This code should be stored in the first sector of the hard disk.  When the  
# BIOS runs, it loads this code at physical address 0x7c00-0x7e00 (512 bytes).
# Then it jumps to the beginning of it, in real mode.  
# This code switches into protected mode (32-bit mode) so that all of
# memory can accessed, loads the kernel into memory, and jumps to the
# first byte of the kernel, where start.S is linked.
##############################################################################
	
/* Flags in control register 0 */
#define CR0_PE 0x00000001      /* Protection Enable. */
#define CR0_EM 0x00000004      /* (Floating-point) Emulation. */
#define CR0_PG 0x80000000      /* Paging. */
#define CR0_WP 0x00010000      /* Write-Protect enable in kernel mode. */

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
	
	movb $0x88,%ah
	int $0x15
	jc panic		# Carry flag set on error
2:	addl $1024,%eax		# Total kB
	shrl $2,%eax		# Total 4 kB pages
	movl %eax, ram_pages
	
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
	### CPU magic: jump to relocation, flush prefetch queue, and
	### reload %cs Has the effect of just jmp to the next
	### instruction, but simultaneous loads CS with
	### $PROT_MODE_CSEG.
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

#### Load kernel starting at physical address LOADER_KERN_BASE by
#### frobbing the IDE controller directly.

	movl $1, %ebx
	movl $LOADER_KERN_BASE, %edi
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

##### Create temporary page directory and page table, set page
##### directory pointer, and turn on paging.
##### FIXME? We could use a single 4 MB page instead of 1024 4 kB pages.

	# Create page directory at 64 kB.
	movl $0x10000, %edi		
	movl %edi, %cr3

	# Fill page directory with zeroes.
	subl %eax, %eax
	movl $0x400, %ecx
	rep stosl

	# Set PDEs for 0 and LOADER_PHYS_BASE to point to the
	# page table.
	movl $0x11000 | PG_U | PG_W | PG_P, %eax
	movl %eax, 0x10000
	movl %eax, 0x10000 | (LOADER_PHYS_BASE >> 20)

	# Initialize page table.
	movl $PG_U | PG_W | PG_P, %eax
	movl $0x400, %ecx
1:	stosl
	addl $0x1000, %eax
	loop 1b

	# Turn on paging and kernel write-protect.
	movl %cr0, %eax
	orl $CR0_PG | CR0_WP, %eax
	movl %eax, %cr0
	jmp 1f
1:

##### Turn on EM bit in CR0, forcing most floating-point instructions
##### to trap.  We don't support floating-point or MMX.

	movl %cr0, %eax
	orl $CR0_EM, %eax
	movl %eax, %cr0
	
##### Jump to kernel entry point.

	movl $LOADER_PHYS_BASE + 0x20000, %esp
	movl $LOADER_PHYS_BASE + LOADER_KERN_BASE, %eax
	call *%eax
	jmp panic

##### GDT

gdt:
	.quad 0x0000000000000000	# null seg
	.quad 0x00cf9a000000ffff	# code seg
	.quad 0x00cf92000000ffff        # data seg
	
gdtdesc:
	.word	0x17			# sizeof (gdt) - 1
	.long	gdt			# address gdt

##### To panic, we print panicmsg (with help from the BIOS) and spin.
panic:  .code16			# We only panic in real mode.
	movw $panicmsg, %si
	movb $0xe, %ah
	xorb %bh, %bh
1:	lodsb
	test %al, %al
2:	jz 2b			# Spin.
	int $0x10
	jmp 1b

panicmsg:
	.ascii "Loader panic!\r\n"
	.byte 0

##### Memory size in 4 kB pages.
	.org LOADER_RAM_PAGES - LOADER_BASE
ram_pages:
	.long 0

##### Command-line arguments inserted by another utility.
##### The loader doesn't use these, but we note their
##### location here for easy reference.
	.org LOADER_CMD_LINE - LOADER_BASE
cmd_line:
	.fill 0x80, 1, 0

##### Boot-sector signature for BIOS inspection.
	.org LOADER_BIOS_SIG - LOADER_BASE
	.word 0xaa55
