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

#include "threads/loader.h"
	
	.intel_syntax noprefix
	
#### Kernel loader.

#### This code should be stored in the first sector of the hard disk.
#### When the BIOS runs, it loads this code at physical address
#### 0x7c00-0x7e00 (512 bytes).  Then it jumps to the beginning of it,
#### in real mode.  This code switches into protected mode (32-bit
#### mode) so that all of memory can accessed, loads the kernel into
#### memory, and jumps to the first byte of the kernel, where start.S
#### is linked.
	
/* Flags in control register 0. */
#define CR0_PE 0x00000001      /* Protection Enable. */
#define CR0_EM 0x00000004      /* (Floating-point) Emulation. */
#define CR0_PG 0x80000000      /* Paging. */
#define CR0_WP 0x00010000      /* Write-Protect enable in kernel mode. */

# Code runs in real mode, which is a 16-bit segment.

.globl start
start:
	.code16

# Disable interrupts.
# String instructions go upward.

	cli
	cld

# Set up data segments.

	sub ax, ax
	mov es, ax
	mov ds, ax

# Set up stack segment.
# Stack grows downward starting from us.
# We don't ever use the stack so this is strictly speaking
# unnecessary.

	mov ss, ax
	mov sp, 0x7c00
	
#### Enable A20.  Address line 20 is tied to low when the machine
#### boots, which prevents addressing memory about 1 MB.  This code
#### fixes it.
	
# Poll status register while busy.

1:	in al, 0x64
	test al, 0x2
	jnz 1b

# Send command for writing output port.

	mov al, 0xd1
	outb 0x64, al

# Poll status register while busy.

1:	in al, 0x64
	test al, 0x2
	jnz 1b

# Enable A20 line.

	mov al, 0xdf
	out 0x60, al

#### Get memory size, via interrupt 15h function 88h, which returns CF
#### clear if successful, with AX = (kB of physical memory) - 1024.
#### This only works for memory sizes <= 65 MB, which should be fine
#### for our purposes.  We cap memory at 64 MB because that's all we
#### prepare page tables for, below.
	
	mov ah, 0x88
	int 0x15
	jc panic
	cli			# BIOS might have enabled interrupts
	add eax, 1024		# Total kB memory
	cmp eax, 0x10000	# Cap at 64 MB
	jbe 1f
	mov eax, 0x10000
1:	shr eax, 2		# Total 4 kB pages
	mov ram_pages, eax 
	
#### Create temporary page directory and page table and set page
#### directory base register.

# Create page directory at 64 kB and fill with zeroes.
	mov ax, 0x1000
	mov es, ax
	sub eax, eax
	sub edi, edi
	mov ecx, 0x400
	rep stosd

# Add PDEs to point to PTEs for the first 64 MB of RAM.
# Also add identical PDEs starting at LOADER_PHYS_BASE.
# See [IA32-v3] section 3.7.6 for a description of the bits in eax.

# A bug in some versions of GAS prevents us from using the straightforward
#	mov es:[di + LOADER_PHYS_BASE / 1024 / 1024], eax
# so we calculate the displacement in bx instead.

	mov eax, 0x11007
	mov ecx, 0x11
	sub di, di
	mov ebx, LOADER_PHYS_BASE
	shr ebx, 20
1:	mov es:[di], eax
	mov es:[bx + di], eax 
	add di, 4
	add eax, 0x1000
	loop 1b

# Set up one-to-map linear to physical map for the first 64 MB of RAM.
# See [IA32-v3] section 3.7.6 for a description of the bits in eax.

	mov ax, 0x1100
	mov es, ax
	mov eax, 0x7
	mov cx, 0x4000
	sub di, di
1:	mov es:[di], eax 
	add di, 4 
	add eax, 0x1000
	loop 1b

# Set page directory base register.

	mov eax, 0x10000
	mov cr3, eax
	
#### Switch to protected mode.

# Then we point the GDTR to our GDT.  Protected mode requires a GDT.
# We need a data32 prefix to ensure that all 32 bits of the GDT
# descriptor are loaded (default is to load only 24 bits).

	data32 lgdt gdtdesc

# Then we turn on the following bits in CR0:
#    PE (Protect Enable): this turns on protected mode.
#    PG (Paging): turns on paging.
#    WP (Write Protect): if unset, ring 0 code ignores
#       write-protect bits in page tables (!).
#    EM (Emulation): forces floating-point instructions to trap.
#       We don't support floating point. 
	
	mov eax, cr0
	or eax, CR0_PE + CR0_PG + CR0_WP + CR0_EM
	mov cr0, eax
	
# We're now in protected mode in a 16-bit segment.  The CPU still has
# the real-mode code segment cached in cs's segment descriptor.  We
# need to reload cs, and the easiest way is to use a far jump.
# Because we're not in a 32-bit segment the data32 prefix is needed to
# jump to a 32-bit offset.

	data32 ljmp SEL_KCSEG, 1f + LOADER_PHYS_BASE

# We're now in protected mode in a 32-bit segment.

	.code32

# Reload all the other segment registers and the stack pointer to
# point into our new GDT.

1:	mov ax, SEL_KDSEG
	mov ds, ax		
	mov es, ax		
	mov fs, ax		
	mov gs, ax		
	mov ss, ax
	mov esp, LOADER_PHYS_BASE + 0x30000

#### Load kernel starting at physical address LOADER_KERN_BASE by
#### frobbing the IDE controller directly.

	mov ebx, 1
	mov edi, LOADER_KERN_BASE + LOADER_PHYS_BASE
read_sector:

# Poll status register while controller busy.

	mov edx, 0x1f7
1:	in al, dx
	test al, 0x80
	jnz 1b

# Read a single sector.

	mov edx, 0x1f2
	mov al, 1 
	out dx, al

# Sector number to write in low 28 bits.
# LBA mode, device 0 in top 4 bits.

	mov eax, ebx
	and eax, 0x0fffffff
	or eax, 0xe0000000

# Dump eax to ports 0x1f3...0x1f6.

	mov ecx, 4
1:	inc dx
	out dx, al
	shr eax, 8
	loop 1b

# READ command to command register.

	inc dx
	mov al, 0x20
	out dx, al

# Poll status register while controller busy.

1:	in al, dx
	test al, 0x80
	jnz 1b

# Poll status register until data ready.

1:	in al, dx
	test al, 0x08
	jz 1b

# Transfer sector.

	mov ecx, 256
	mov edx, 0x1f0
	rep insw

# Next sector.

	inc ebx
	cmp ebx, KERNEL_LOAD_PAGES*8 + 1
	jnz read_sector

#### Jump to kernel entry point.

	mov eax, LOADER_PHYS_BASE + LOADER_KERN_BASE
	call eax
	jmp panic

#### GDT

gdt:
	.quad 0x0000000000000000	# null seg
	.quad 0x00cf9a000000ffff	# code seg
	.quad 0x00cf92000000ffff        # data seg
	
gdtdesc:
	.word	0x17			# sizeof (gdt) - 1
	.long	gdt + LOADER_PHYS_BASE	# address gdt

#### Fatal error.
#### Print panicmsg (with help from the BIOS) and spin.

panic:  .code16			# We only panic in real mode.
	mov si, offset panicmsg
	mov ah, 0xe
	sub bh, bh
1:	lodsb
	test al, al
2:	jz 2b			# Spin.
	int 0x10
	jmp 1b

panicmsg:
	.ascii "Loader panic!\r\n"
	.byte 0

#### Memory size in 4 kB pages.
	.org LOADER_RAM_PAGES - LOADER_BASE
ram_pages:
	.long 0

#### Command-line arguments inserted by another utility.
#### The loader doesn't use these, but we note their
#### location here for easy reference.
	.org LOADER_CMD_LINE - LOADER_BASE
cmd_line:
	.fill 0x80, 1, 0

#### Boot-sector signature for BIOS inspection.
	.org LOADER_BIOS_SIG - LOADER_BASE
	.word 0xaa55
