#ifndef HEADER_MMU_H
#define HEADER_MMU_H 1

#ifndef __ASSEMBLER__
#include <debug.h>
#include <stdint.h>
#endif

#include "threads/loader.h"

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

/* Page Directory Entry (PDE) and Page Table Entry (PTE) flags. */
#define PG_P 0x1               /* 1=present, 0=not present. */
#define PG_W 0x2               /* 1=read/write, 0=read-only. */
#define PG_U 0x4               /* 1=user/kernel, 0=kernel only. */
#define PG_A 0x20              /* 1=accessed, 0=not acccessed. */
#define PG_D 0x40              /* 1=dirty, 0=not dirty. */

/* EFLAGS Register. */
#define FLAG_MBS  0x00000002    /* Must be set. */
#define FLAG_IF   0x00000200    /* Interrupt Flag. */

#endif /* mmu.h */
