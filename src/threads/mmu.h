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


#ifndef _MMU_H_
#define _MMU_H_

#ifndef __ASSEMBLER__
#include <stdint.h>
#include "debug.h"
#endif

#define MASK(SHIFT, CNT) (((1ul << (CNT)) - 1) << (SHIFT))

/* Page offset (bits 0:11). */
#define PGSHIFT         0                  /* First offset bit. */
#define PGBITS          12                 /* Number of offset bits. */
#define PGMASK          MASK(PGSHIFT, PGBITS)
#define PGSIZE          (1 << PGBITS)

/* Page table (bits 12:21). */
#define	PTSHIFT		PGBITS		   /* First page table bit. */
#define PTBITS          10                 /* Number of page table bits. */
#define PTMASK          MASK(PTSHIFT, PTBITS)

/* Page directory (bits 22:31). */
#define PDSHIFT         (PTSHIFT + PTBITS) /* First page dir bit. */
#define PDBITS          10                 /* Number of page dir bits. */
#define PDMASK          MASK(PDSHIFT, PDBITS)

#ifndef __ASSEMBLER__
/* Offset within a page. */
static inline unsigned pg_ofs (void *va) { return (uintptr_t) va & PGMASK; }

/* Page number. */
static inline uintptr_t pg_no (void *va) { return (uintptr_t) va >> PTSHIFT; }

/* Page table number. */
static inline unsigned pt_no (void *va) {
  return ((uintptr_t) va & PTMASK) >> PTSHIFT;
}

/* Page directory number. */
static inline uintptr_t pd_no (void *va) { return (uintptr_t) va >> PDSHIFT; }

/* Round up to nearest page boundary. */
static inline void *pg_round_up (void *va) {
  return (void *) (((uintptr_t) va + PGSIZE - 1) & ~PGMASK);
}

/* Round down to nearest page boundary. */
static inline void *pg_round_down (void *va) {
  return (void *) ((uintptr_t) va & ~PGMASK);
}

#define	PHYS_BASE ((void *) 0xc0000000) /* Physical memory mapped here. */
#define KERN_BASE ((void *) 0xc0100000) /* Kernel loaded here. */

/* Returns kernel virtual address at which physical address PADDR
   is mapped. */
static inline void *
ptov (uintptr_t paddr) 
{
  ASSERT ((void *) paddr < PHYS_BASE);

  return (void *) (paddr + PHYS_BASE);
}

/* Returns physical address at which kernel virtual address VADDR
   is mapped. */
static inline uintptr_t
vtop (void *vaddr) 
{
  ASSERT (vaddr >= PHYS_BASE);

  return (uintptr_t) vaddr - (uintptr_t) PHYS_BASE;
}
#endif

/* Page Table/Directory Entry flags
 *   these are defined by the hardware
 */
#define PG_P 0x1               /* Present */
#define PG_W 0x2               /* Writeable */
#define PG_U 0x4               /* User */
#define PG_PWT 0x8             /* Write-Through */
#define PG_PCD 0x10            /* Cache-Disable */
#define PG_A 0x20              /* Accessed */
#define PG_D 0x40              /* Dirty */
#define PG_PS 0x80             /* Page Size */
#define PG_MBZ 0x180           /* Bits must be zero */
#define PG_USER 0xe00          /* Bits for user processes */
/*
 * The PG_USER bits are not used by the kernel and they are
 * not interpreted by the hardware.  The kernel allows 
 * user processes to set them arbitrarily.
 */



/* Control Register flags */
#define CR0_PE 0x1             /* Protection Enable */
#define CR0_MP 0x2             /* Monitor coProcessor */
#define CR0_EM 0x4             /* Emulation */
#define CR0_TS 0x8             /* Task Switched */
#define CR0_ET 0x10            /* Extension Type */
#define CR0_NE 0x20            /* Numeric Errror */
#define CR0_WP 0x10000         /* Write Protect */
#define CR0_AM 0x40000         /* Alignment Mask */
#define CR0_NW 0x20000000      /* Not Writethrough */
#define CR0_CD 0x40000000      /* Cache Disable */
#define CR0_PG 0x80000000      /* Paging */

#define CR4_PCE 0x100          /* Performance counter enable */
#define CR4_MCE 0x40           /* Machine Check Enable */
#define CR4_PSE 0x10           /* Page Size Extensions */
#define CR4_DE  0x08           /* Debugging Extensions */
#define CR4_TSD 0x04           /* Time Stamp Disable */
#define CR4_PVI 0x02           /* Protected-Mode Virtual Interrupts */
#define CR4_VME 0x01           /* V86 Mode Extensions */

/* EFLAGS Register. */
#define FLAG_CF   0x00000001    /* Carry Flag. */
#define FLAG_PF   0x00000004    /* Parity Flag. */
#define FLAG_AF   0x00000010    /* Auxiliary Carry Flag. */
#define FLAG_ZF   0x00000040    /* Zero Flag. */
#define FLAG_SF   0x00000080    /* Sign Flag. */
#define FLAG_TF   0x00000100    /* Trap Flag. */
#define FLAG_IF   0x00000200    /* Interrupt Flag. */
#define FLAG_DF   0x00000400    /* Direction Flag. */
#define FLAG_OF   0x00000800    /* Overflow Flag. */
#define FLAG_IOPL 0x00003000    /* I/O Privilege Level (2 bits). */
#define FLAG_IOPL_SHIFT 12
#define FLAG_NT   0x00004000    /* Nested Task. */
#define FLAG_RF   0x00010000    /* Resume Flag. */
#define FLAG_VM   0x00020000    /* Virtual 8086 Mode. */
#define FLAG_AC   0x00040000    /* Alignment Check. */
#define FLAG_VIF  0x00080000    /* Virtual Interrupt Flag. */
#define FLAG_VIP  0x00100000    /* Virtual Interrupt Pending. */
#define FLAG_ID   0x00200000    /* ID Flag. */

/* Page fault error codes */
#define FEC_PR 0x1             /* Page fault caused by protection violation */
#define FEC_WR 0x2             /* Page fault caused by a write */
#define FEC_U  0x4             /* Page fault occured while in user mode */


/* Application segment type bits */
#define STA_X 0x8              /* Executable segment */
#define STA_A 0x1              /* Accessed */

#define STA_C 0x4              /* Conforming code segment (executable only) */
#define STA_R 0x2              /* Readable (executable segments) */

#define STA_E 0x4              /* Expand down (non-executable segments) */
#define STA_W 0x2              /* Writeable (non-executable segments) */


/* Segment selectors. */
#define SEL_NULL        0x00    /* Null selector. */
#define SEL_KCSEG       0x08    /* Kernel code selector. */
#define SEL_KDSEG       0x10    /* Kernel data selector. */
#define SEL_UCSEG       0x1B    /* User code selector. */
#define SEL_UDSEG       0x23    /* User data selector. */
#define SEL_TSS         0x28    /* Task-state segment. */
#define SEL_CNT         6       /* Number of segments. */

#ifndef __ASSEMBLER__
struct tss
  {
    uint16_t back_link, :16;
    uint32_t esp0;
    uint16_t ss0, :16;
    uint32_t esp1;
    uint16_t ss1, :16;
    uint32_t esp2;
    uint16_t ss2, :16;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint16_t es, :16;
    uint16_t cs, :16;
    uint16_t ss, :16;
    uint16_t ds, :16;
    uint16_t fs, :16;
    uint16_t gs, :16;
    uint16_t ldt, :16;
    uint16_t trace, bitmap;
  };

enum seg_system
  {
    SYS_SYSTEM = 0,             /* System segment. */
    SYS_CODE_DATA = 1           /* Code or data segment. */
  };

enum seg_granularity
  {
    GRAN_BYTE = 0,              /* Limit has 1-byte granularity. */
    GRAN_PAGE = 1               /* Limit has 4 kB granularity. */
  };

enum seg_type
  {
    /* System segment types. */
    TYPE_TSS_16_A = 1,          /* 16-bit TSS (available). */
    TYPE_LDT = 2,               /* LDT. */
    TYPE_TSS_16_B = 3,          /* 16-bit TSS (busy). */
    TYPE_CALL_16 = 4,           /* 16-bit call gate. */
    TYPE_TASK = 5,              /* Task gate. */
    TYPE_INT_16 = 6,            /* 16-bit interrupt gate. */
    TYPE_TRAP_16 = 7,           /* 16-bit trap gate. */
    TYPE_TSS_32_A = 9,          /* 32-bit TSS (available). */
    TYPE_TSS_32_B = 11,         /* 32-bit TSS (busy). */
    TYPE_CALL_32 = 12,          /* 32-bit call gate. */
    TYPE_INT_32 = 14,           /* 32-bit interrupt gate. */
    TYPE_TRAP_32 = 15,          /* 32-bit trap gate. */

    /* Code/data segment types. */
    TYPE_CODE = 8,              /* 1=Code segment, 0=data segment. */
    TYPE_ACCESSED = 1,          /* Set if accessed. */

    /* Data segment types. */
    TYPE_EXPAND_DOWN = 4,       /* 1=Expands up, 0=expands down. */
    TYPE_WRITABLE = 2,          /* 1=Read/write, 0=read-only. */

    /* Code segment types. */
    TYPE_CONFORMING = 4,        /* 1=Conforming, 0=nonconforming. */
    TYPE_READABLE = 2           /* 1=Exec/read, 0=exec-only. */
  };

static inline uint64_t
make_dtr_operand (uint16_t limit, void *base)
{
  return limit | ((uint64_t) (uint32_t) base << 16);
}
#endif

#endif /* !_MMU_H_ */
