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

# Set up segment registers.
# Stack grows downward starting from us.

	sub ax, ax
	mov ds, ax
	mov ss, ax
	mov sp, 0x7c00
	
# Scan floppy disks.

	sub dl, dl
	sub ebx, ebx
	mov eax, 0x7e00
	mov es, ax
1:	call scan_partitions
	inc dl
	jnc 1b

# Scan hard disks.
	mov dl, 0x80
1:	call scan_partitions
	inc dl
	jnc 1b

	call panic
	.asciz "No boot partition"

scan_partitions:
	# EBX = sector number of partition table
	# DL = drive number
	# ES:0000 -> buffer for partition table
	# Returns CF set if drive exists, CF clear otherwise.
	call read_sector
	jnc no_such_drive
2:	cmp word ptr [es:510], 0xaa55
	jnz no_boot_partition
	
	mov si, 446
1:	mov al, [es:si+4]
	cmp al, 0x20
	jz found_boot_partition
	cmp al, 0x05
	jz found_extended_partition
	cmp al, 0x0f
	jz found_extended_partition
	cmp al, 0x85
	jz found_extended_partition
	cmp al, 0xc5
	jz found_extended_partition
next_parttbl_entry:	
	add si, 16
	cmp si, 510
	jb 1b

no_boot_partition:	
	clc
	ret
no_such_drive:
	stc
	ret

found_extended_partition:
	# DL = drive number.
	# ES:SI -> partition table entry for extended partition.
	# Recursively examine it.
	pusha
	mov ebx, es:[si+8]
	mov ax, es
	add ax, 0x20
	mov es, ax
	call scan_partitions
	popa
	jmp next_parttbl_entry

found_boot_partition:
	mov ebx, [es:si+8]		# EBX = first sector
	mov cx, [es:si+12]		# CX = number of sectors
	mov ax, 0x1000			# ES:0000 -> load address
	mov es, ax
1:	call read_sector
	add ax, 0x20
	mov es, ax
	loop 1b

	ljmp 0x1000, 0

	# ebx: sector number
	# dl: drive number
	# es:0000: destination buffer
	# returns error flag in CF
read_sector:
	pusha
	or dl, dl			# Floppy drives: DL < 0
	js read_floppy_sector

read_harddrv_sector:
	sub eax, eax
	push eax			# LBA sector number [32:63]
	push ebx			# LBA sector number [0:31]
	mov ax, es
	shl eax, 4
	push eax			# Buffer linear address
	push 1				# Transfer one sector
	push 16				# Packet size
	mov ah, 0x42			# Extended read
	mov si, sp			# DS:SI -> packet
	int 0x13			# Error code in CF
success:	
	popa
	ret				# Error code in CF

read_floppy_sector:
	#define HEADS 2
	#define SECTORS 36

	# In: BX = LBA sector number, DL = drive.
	# Out: BL = drive, DX = cylinder, AL = head, AH = sector.
	sub ax, ax
	xchg dx, bx			# DX = LBA sector number, BL = drive
	mov cx, HEADS * SECTORS
	div cx				# AX = cyl, DX = hd + (sec-1) * SECTORS
	xchg ax, dx			# DX = cyl, AX = hd + (sec-1) * SECTORS
	mov cl, SECTORS
	div cl				# AL = head, AH = sector - 1
	inc ah

	# Read sector.
	mov ch, dl			# CH = cylinder
	mov cl, ah			# CL = sector
	mov dh, al			# DH = head
	mov dl, bl			# DL = drive
	mov ax, 0x0201			# AH = function, AL = sectors to read
	sub bx, bx			# ES:BX -> buffer
	pusha
	int 0x13
	popa
	jnc success

	# Reset floppy drive motor, try again.
	sub ah, ah
	int 0x13
	popa
	jmp read_sector
	
#### The partition table goes here.
	.org 446
part_tbl:	

#### Boot-sector signature for BIOS inspection.
	.org 510
	.word 0xaa55
