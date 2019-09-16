#include "userprog/pagedir.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "threads/init.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include "vm/pageinfo.h"
#include "vm/frametable.h"

static uint32_t *active_pd (void);
static void invalidate_pagedir (uint32_t *);

/* Creates a new page directory that has mappings for kernel
   virtual addresses, but none for user virtual addresses.
   Returns the new page directory, or a null pointer if memory
   allocation fails. */
uint32_t *
pagedir_create (void) 
{
  uint32_t *pd = palloc_get_page (0);
  if (pd != NULL)
    memcpy (pd, init_page_dir, PGSIZE);
  return pd;
}

/* Destroys page directory PD, unloading all the pages it
   references. */
void
pagedir_destroy (uint32_t *pd) 
{
  uint32_t *pde;
  uint32_t *pt;
  uint32_t *pte;
  void *ubase;
  void *upage;

  if (pd == NULL)
    return;

  ASSERT (pd != init_page_dir);
  for (ubase = 0, pde = pd; pde < pd + pd_no (PHYS_BASE);
       ubase += PTSPAN, pde++)
    {
      if (*pde & PTE_P)
        {
          pt = pde_get_pt (*pde);
          for (upage = ubase, pte = pt; pte < pt + PGSIZE / sizeof *pte;
               upage += PGSIZE, pte++)
              pagedir_unload_page (pd, upage);
          palloc_free_multiple (pt, 2);
        }
    }
  palloc_free_page (pd);
}

/* Returns the address of the page table entry for virtual
   address VADDR in page directory PD.
   If PD does not have a page table for VADDR, behavior depends
   on CREATE.  If CREATE is true, then a new page table is created
   along with additional space for page info and a pointer to the
   page table is returned.  Otherwise, a null pointer is returned.
*/
static uint32_t *
lookup_page (uint32_t *pd, const void *vaddr, bool create)
{
  uint32_t *pt, *pde;

  ASSERT (pd != NULL);

  /* Shouldn't create new kernel virtual mappings. */
  ASSERT (!create || is_user_vaddr (vaddr));

  /* Check for a page table for VADDR.
     If one is missing, create one if requested. */
  pde = pd + pd_no (vaddr);
  if (*pde == 0) 
    {
      if (create)
        {
          pt = palloc_get_multiple (PAL_ZERO, 2);
          if (pt == NULL) 
            return NULL; 
      
          *pde = pde_create (pt);
        }
      else
        return NULL;
    }

  /* Return the page table entry. */
  pt = pde_get_pt (*pde);
  return &pt[pt_no (vaddr)];
}

/* Adds a mapping in page directory PD from user virtual page
   UPAGE to the physical frame identified by kernel virtual
   address KPAGE.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   If WRITABLE is true, the new page is read/write;
   otherwise it is read-only.
   Returns true if successful, false if memory allocation
   failed. */
bool
pagedir_set_page (uint32_t *pd, const void *upage, void *kpage, bool writable)
{
  uint32_t *pte;

  ASSERT (pg_ofs (upage) == 0);
  ASSERT (pg_ofs (kpage) == 0);
  ASSERT (is_user_vaddr (upage));
  ASSERT (vtop (kpage) >> PTSHIFT < init_ram_pages);
  ASSERT (pd != init_page_dir);

  pte = lookup_page (pd, upage, true);
  if (pte != NULL) 
    {
      ASSERT ((*pte & PTE_P) == 0);
      *pte = pte_create_user (kpage, writable);
      return true;
    }
  else
    return false;
}

/* Looks up the physical address that corresponds to user virtual
   address UADDR in PD.  Returns the kernel virtual address
   corresponding to thxat physical address, or a null pointer if
   UADDR is unmapped. */
void *
pagedir_get_page (uint32_t *pd, const void *uaddr) 
{
  uint32_t *pte;

  ASSERT (is_user_vaddr (uaddr));
  
  pte = lookup_page (pd, uaddr, false);
  if (pte != NULL && (*pte & PTE_P) != 0)
    return pte_get_page (*pte) + pg_ofs (uaddr);
  else
    return NULL;
}

/* Marks user virtual page UPAGE "not present" in page
   directory PD.  Later accesses to the page will fault.  Other
   bits in the page table entry are preserved.
   UPAGE need not be mapped. */
void
pagedir_clear_page (uint32_t *pd, const void *upage) 
{
  uint32_t *pte;

  ASSERT (pg_ofs (upage) == 0);
  ASSERT (is_user_vaddr (upage));

  pte = lookup_page (pd, upage, false);
  if (pte != NULL && (*pte & PTE_P) != 0)
    {
      *pte &= ~PTE_P;
      invalidate_pagedir (pd);
    }
}

/* Returns true if the PTE for virtual page VPAGE in PD is dirty,
   that is, if the page has been modified since the PTE was
   installed.
   Returns false if PD contains no PTE for VPAGE. */
bool
pagedir_is_dirty (uint32_t *pd, const void *vpage) 
{
  uint32_t *pte = lookup_page (pd, vpage, false);
  return pte != NULL && (*pte & PTE_D) != 0;
}

/* Set the dirty bit to DIRTY in the PTE for virtual page VPAGE
   in PD. */
void
pagedir_set_dirty (uint32_t *pd, const void *vpage, bool dirty) 
{
  uint32_t *pte = lookup_page (pd, vpage, false);
  if (pte != NULL) 
    {
      if (dirty)
        *pte |= PTE_D;
      else 
        {
          *pte &= ~(uint32_t) PTE_D;
          invalidate_pagedir (pd);
        }
    }
}

/* Returns true if the PTE for virtual page VPAGE in PD has been
   accessed recently, that is, between the time the PTE was
   installed and the last time it was cleared.  Returns false if
   PD contains no PTE for VPAGE. */
bool
pagedir_is_accessed (uint32_t *pd, const void *vpage) 
{
  uint32_t *pte = lookup_page (pd, vpage, false);
  return pte != NULL && (*pte & PTE_A) != 0;
}

/* Sets the accessed bit to ACCESSED in the PTE for virtual page
   VPAGE in PD. */
void
pagedir_set_accessed (uint32_t *pd, const void *vpage, bool accessed) 
{
  uint32_t *pte = lookup_page (pd, vpage, false);
  if (pte != NULL) 
    {
      if (accessed)
        *pte |= PTE_A;
      else 
        {
          *pte &= ~(uint32_t) PTE_A; 
          invalidate_pagedir (pd);
        }
    }
}

/* Loads page directory PD into the CPU's page directory base
   register. */
void
pagedir_activate (uint32_t *pd) 
{
  if (pd == NULL)
    pd = init_page_dir;

  /* Store the physical address of the page directory into CR3
     aka PDBR (page directory base register).  This activates our
     new page tables immediately.  See [IA32-v2a] "MOV--Move
     to/from Control Registers" and [IA32-v3a] 3.7.5 "Base
     Address of the Page Directory". */
  asm volatile ("movl %0, %%cr3" : : "r" (vtop (pd)) : "memory");
}

/* Returns the currently active page directory. */
static uint32_t *
active_pd (void) 
{
  /* Copy CR3, the page directory base register (PDBR), into
     `pd'.
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 3.7.5 "Base Address of the Page Directory". */
  uintptr_t pd;
  asm volatile ("movl %%cr3, %0" : "=r" (pd));
  return ptov (pd);
}

/* Some page table changes can cause the CPU's translation
   lookaside buffer (TLB) to become out-of-sync with the page
   table.  When this happens, we have to "invalidate" the TLB by
   re-activating it.

   This function invalidates the TLB if PD is the active page
   directory.  (If PD is not active then its entries are not in
   the TLB, so there is no need to invalidate anything.) */
static void
invalidate_pagedir (uint32_t *pd) 
{
  if (active_pd () == pd) 
    {
      /* Re-activating PD clears the TLB.  See [IA32-v3a] 3.12
         "Translation Lookaside Buffers (TLBs)". */
      pagedir_activate (pd);
    } 
}

/* Clears the page table entry, frees the associated frame, 
   unloads the associated frame, and frees the page info. */
void
pagedir_unload_page (uint32_t *pd, const void *upage)
{
  struct page_info *info;

  ASSERT (pg_ofs (upage) == 0);
  ASSERT (is_user_vaddr (upage));
  ASSERT (pd != init_page_dir);

  info = pagedir_get_info (pd, upage);
  if (info != NULL)
    {
      /* The frame table will free the frame and page info and clear
         the entry. */
      frametable_unload_frame (pd, upage);
    }
}

/* Associates a pointer to additional information with a
   page.  UPAGE may or may not already be mapped.  Returns 
   true if successful, false if memory allocation failed. */
bool
pagedir_set_info (uint32_t *pd, const void *upage, struct page_info *info)
{
  uint32_t *pte;
  struct page_info **pie;

  ASSERT (pg_ofs (upage) == 0);
  ASSERT (is_user_vaddr (upage));
  ASSERT (pd != init_page_dir);

  pte = lookup_page (pd, upage, true);
  if (pte != NULL)
    {
      pie = ((struct page_info **) pg_next_page (pte)) + pt_no (upage);
      *pie = info;
      return true;
    }
  else
    return false;
}

/* Looks up the pointer to additional information associated
   with the page that corresponds to user virtual address 
   UPAGE in PD.  Returns a pointer to the page information
   or null if there is none. */
struct page_info *
pagedir_get_info (uint32_t *pd, const void *upage)
{
  uint32_t *pte;
  struct page_info **pie;

  ASSERT (pg_ofs (upage) == 0);
  ASSERT (is_user_vaddr (upage));
  ASSERT (pd != init_page_dir);

  pte = lookup_page (pd, upage, false);
  if (pte != NULL)
    {
      pie = ((struct page_info **) pg_next_page (pte)) + pt_no (upage);
      return *pie;
    }
  else
    return NULL;
}
