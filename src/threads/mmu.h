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

#include "loader.h"

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

#define	PHYS_BASE ((void *) LOADER_PHYS_BASE)

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
#define PG_A 0x20              /* Accessed */
#define PG_D 0x40              /* Dirty */

/* EFLAGS Register. */
#define FLAG_MBS  0x00000002    /* Must be set. */
#define FLAG_IF   0x00000200    /* Interrupt Flag. */

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



#endif /* !_MMU_H_ */
