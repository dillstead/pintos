#### The loader needs to have some way to know the kernel's entry
#### point, that is, the address to which it should jump to start the
#### kernel.  We handle this by writing the linker script kernel.lds.S
#### so that this module appears at the very beginning of the kernel
#### image, and then using that as the entry point.

#include "threads/loader.h"
	.intel_syntax noprefix

.section .start
	
# Code runs in real mode, which is a 16-bit segment.
	.code16

.globl start
start:

# Disable interrupts.
# String instructions go upward.

	cli
	cld

# Set up data segments.

	sub ax, ax
	mov ds, ax
	mov es, ax

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
#### for our purposes.  We only reserve enough memory for page tables
#### for 64 MB of RAM, so we cap it at that value.
	
	mov ah, 0x88
	int 0x15
	cli			# BIOS might have enabled interrupts
	add eax, 1024		# Total kB memory
	cmp eax, 64 * 1024
	jbe 1f
	mov eax, 64 * 1024
1:	shr eax, 2		# Total 4 kB pages
	mov [ram_pages], eax 
	
#### Create temporary page directory and page table and set page
#### directory base register.

# Create page directory at 64 kB and fill with zeroes.
	mov ax, LOADER_PD_BASE / 16
	mov es, ax
	sub eax, eax
	sub edi, edi
	mov ecx, 0x400
	rep stosd

# Add a PDE mapping both virtual addresses 0 and LOADER_PHYS_BASE
# to a single PTE.
# See [IA32-v3] section 3.7.6 for a description of the bits in eax.

	mov eax, LOADER_PT_BASE / 16 + 7
	mov es:[0], eax
	mov es:[LOADER_PHYS_BASE / 1024 / 1024], eax

# Set up one-to-map linear to physical map for the first 4 MB of RAM.
# See [IA32-v3] section 3.7.6 for a description of the bits in eax.

	mov ax, LOADER_PT_BASE / 16
	mov es, ax
	mov eax, 0x7
	mov cx, 0x400
1:	stosd
	add eax, 0x1000
	loop 1b

# Set page directory base register.

	mov eax, LOADER_PD_BASE
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
	
#define CR0_PE 0x00000001
#define CR0_EM 0x00000004
#define CR0_PG 0x80000000
#define CR0_WP 0x00010000
	
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
	add esp, LOADER_PHYS_BASE

#### Jump to kernel entry point.

	mov eax, main
	call eax
1:	jmp 1b

#### GDT

gdt:
	.quad 0x0000000000000000	# null seg
	.quad 0x00cf9a000000ffff	# code seg
	.quad 0x00cf92000000ffff        # data seg
	
gdtdesc:
	.word	0x17			# sizeof (gdt) - 1
	.long	gdt + LOADER_PHYS_BASE	# address gdt
