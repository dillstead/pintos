#ifndef THREADS_MMU_H
#define THREADS_MMU_H

#include <debug.h>
#include <stdint.h>
#include <stdbool.h>

#include "threads/loader.h"

/* Virtual to physical translation works like this on an x86:

   - The top 10 bits of the virtual address (bits 22:31) are used
     to index into the page directory.  If the PDE is marked
     "present," the physical address of a page table is read from
     the PDE thus obtained.  If the PDE is marked "not present"
     then a page fault occurs.

   - The next 10 bits of the virtual address (bits 12:21) are
     used to index into the page table.  If the PTE is marked
     "present," the physical address of a data page is read from
     the PTE thus obtained.  If the PTE is marked "not present"
     then a page fault occurs.

   - The bottom 12 bits of the virtual address (bits 0:11) are
     added to the data page's physical base address, producing
     the final physical address.


   32                    22                     12                      0
   +--------------------------------------------------------------------+
   | Page Directory Index |   Page Table Index   |    Page Offset       |
   +--------------------------------------------------------------------+
                |                    |                     |
        _______/             _______/                _____/
       /                    /                       /
      /    Page Directory  /      Page Table       /    Data Page
     /     .____________. /     .____________.    /   .____________.
     |1,023|____________| |1,023|____________|    |   |____________|
     |1,022|____________| |1,022|____________|    |   |____________|
     |1,021|____________| |1,021|____________|    \__\|____________|
     |1,020|____________| |1,020|____________|       /|____________|
     |     |            | |     |            |        |            |
     |     |            | \____\|            |_       |            |
     |     |      .     |      /|      .     | \      |      .     |
     \____\|      .     |_      |      .     |  |     |      .     |
          /|      .     | \     |      .     |  |     |      .     |
           |      .     |  |    |      .     |  |     |      .     |
           |            |  |    |            |  |     |            |
           |____________|  |    |____________|  |     |____________|
          4|____________|  |   4|____________|  |     |____________|
          3|____________|  |   3|____________|  |     |____________|
          2|____________|  |   2|____________|  |     |____________|
          1|____________|  |   1|____________|  |     |____________|
          0|____________|  \__\0|____________|  \____\|____________|
                              /                      /
*/

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

/* Offset within a page. */
static inline unsigned pg_ofs (const void *va) {
  return (uintptr_t) va & PGMASK;
}

/* Virtual page number. */
static inline uintptr_t pg_no (const void *va) {
  return (uintptr_t) va >> PTSHIFT;
}

/* Round up to nearest page boundary. */
static inline void *pg_round_up (const void *va) {
  return (void *) (((uintptr_t) va + PGSIZE - 1) & ~PGMASK);
}

/* Round down to nearest page boundary. */
static inline void *pg_round_down (const void *va) {
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
vtop (const void *vaddr)
{
  ASSERT (vaddr >= PHYS_BASE);

  return (uintptr_t) vaddr - (uintptr_t) PHYS_BASE;
}

/* Page directories and page tables.

   For more information see [IA32-v3] pages 3-23 to 3-28.

   PDEs and PTEs share a common format:

   32                                   12                       0
   +------------------------------------+------------------------+
   |         Physical Address           |         Flags          |
   +------------------------------------+------------------------+

   In a PDE, the physical address points to a page table.
   In a PTE, the physical address points to a data or code page.
   The important flags are listed below.
   When a PDE or PTE is not "present", the other flags are
   ignored.
   A PDE or PTE that is initialized to 0 will be interpreted as
   "not present", which is just fine. */
#define PG_P 0x1               /* 1=present, 0=not present. */
#define PG_W 0x2               /* 1=read/write, 0=read-only. */
#define PG_U 0x4               /* 1=user/kernel, 0=kernel only. */
#define PG_A 0x20              /* 1=accessed, 0=not acccessed. */
#define PG_D 0x40              /* 1=dirty, 0=not dirty (PTEs only). */

/* Obtains page directory index from a virtual address. */
static inline uintptr_t pd_no (const void *va) {
  return (uintptr_t) va >> PDSHIFT;
}

/* Returns a PDE that points to page table PT. */
static inline uint32_t pde_create (uint32_t *pt) {
  ASSERT (pg_ofs (pt) == 0);
  return vtop (pt) | PG_U | PG_P | PG_W;
}

/* Returns a pointer to the page table that page directory entry
   PDE, which must "present", points to. */
static inline uint32_t *pde_get_pt (uint32_t pde) {
  ASSERT (pde & PG_P);
  return ptov (pde & ~PGMASK);
}

/* Obtains page table index from a virtual address. */
static inline unsigned pt_no (void *va) {
  return ((uintptr_t) va & PTMASK) >> PTSHIFT;
}

/* Returns a PTE that points to PAGE.
   The PTE's page is readable.
   If WRITABLE is true then it will be writable as well.
   The page will be usable only by ring 0 code (the kernel). */
static inline uint32_t pte_create_kernel (uint32_t *page, bool writable) {
  ASSERT (pg_ofs (page) == 0);
  return vtop (page) | PG_P | (writable ? PG_W : 0);
}

/* Returns a PTE that points to PAGE.
   The PTE's page is readable.
   If WRITABLE is true then it will be writable as well.
   The page will be usable by both user and kernel code. */
static inline uint32_t pte_create_user (uint32_t *page, bool writable) {
  return pte_create_kernel (page, writable) | PG_U;
}

/* Returns a pointer to the page that page table entry PTE, which
   must "present", points to. */
static inline void *pte_get_page (uint32_t pte) {
  ASSERT (pte & PG_P);
  return ptov (pte & ~PGMASK);
}

#endif /* threads/mmu.h */
