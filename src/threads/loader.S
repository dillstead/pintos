#include "threads/loader.h"
	
	.intel_syntax noprefix
	
#### Kernel loader.

# Runs in real mode, which is a 16-bit segment.
	.code16

# Set up segment registers.
# Stack grows downward starting from us.

.globl start
start:
	sub ax, ax
	mov ds, ax
	mov ss, ax
	mov sp, 0x7c00
	cld

	mov ax, 0x00e3
	sub dx, dx
	int 0x14

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
1:	call outw
	call scan_partitions
	inc dl
	jnc 1b

1:	jmp 1b

scan_partitions:
	# EBX = sector number of partition table
	# DL = drive number
	# ES:0000 -> buffer for partition table
	# Returns CF set if drive does not exist, CF clear otherwise.
	call read_sector
	jc no_such_drive
	
2:	pusha
	mov edx, [es:508]
	call outw
	popa
	
	cmp word ptr [es:510], 0xaa55
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
	or dl, dl			# Floppy drives: DL >= 0
	jns read_floppy_sector

read_harddrv_sector:
	sub eax, eax
	push eax			# LBA sector number [32:63]
	push ebx			# LBA sector number [0:31]
	push es
	push 0
	push 1				# Transfer one sector
	push 16				# Packet size
	mov ah, 0x42			# Extended read
	mov si, sp			# DS:SI -> packet
	int 0x13			# Error code in CF
	lahf
	add sp, 16
	sahf
read_sector_done:
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
	mov di, 3			# Number of tries left.
	
try_read_floppy:
	pusha
	int 0x13
	popa
	jnc read_sector_done
	dec di
	jnz 1f
	stc
	jmp read_sector_done	

1:	# Reset floppy drive motor, try again.
	pusha
	sub ah, ah
	int 0x13
	popa
	jmp try_read_floppy

outw:	pushf
	pusha
	
	mov cx, 8
	mov ebx, edx
	mov dx, 0xe9
1:	rol ebx, 4
	mov al, bl
	and al, 15
	add al, '0'
	cmp al, '9'
	jbe 2f
	add al, 'a' - '0' - 10
2:	out dx, al
	loop 1b

	mov al, '\n'
	out dx, al

	popa
	popf
	ret
	
#### The partition table goes here.
	.org 446
part_tbl:
	

#### Boot-sector signature for BIOS inspection.
	.org 510
	.word 0xaa55
