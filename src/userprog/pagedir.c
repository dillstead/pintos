#include "userprog/pagedir.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "threads/init.h"
#include "threads/mmu.h"
#include "threads/palloc.h"

/* Creates a new page directory that has mappings for kernel
   virtual addresses, but none for user virtual addresses.
   Returns the new page directory, or a null pointer if memory
   allocation fails. */
uint32_t *
pagedir_create (void) 
{
  uint32_t *pd = palloc_get (0);
  memcpy (pd, base_page_dir, PGSIZE);
  return pd;
}

/* Destroys page directory PD, freeing all the pages it
   references. */
void
pagedir_destroy (uint32_t *pd) 
{
  uint32_t *pde;

  if (pd == NULL)
    return;

  for (pde = pd; pde < pd + pd_no (PHYS_BASE); pde++)
    if (*pde & PG_P) 
      {
        uint32_t *pt = pde_get_pt (*pde);
        uint32_t *pte;
        
        for (pte = pt; pte < pt + PGSIZE / sizeof *pte; pte++)
          if (*pte & PG_P) 
            palloc_free (pte_get_page (*pte));
        palloc_free (pt);
      }
  palloc_free (pd);
}

/* Returns the mapping of user virtual address UADDR in page
   directory PD into a kernel virtual address.
   If UADDR is unmapped, behavior varies based on CREATE:
   if CREATE is true, then a new, zeroed page is created and a
   pointer into it is returned,
   otherwise a null pointer is returned.
   Also returns a null pointer if UADDR is a kernel address. */
static uint32_t *
lookup_page (uint32_t *pd, void *uaddr, bool create)
{
  uint32_t *pt, *pde;

  ASSERT (pd != NULL);

  /* Make sure it's a user address. */
  if (uaddr >= PHYS_BASE)
    return NULL;

  /* Check for a page table for UADDR.
     If one is missing, create one if requested. */
  pde = pd + pd_no (uaddr);
  if (*pde == 0) 
    {
      if (create)
        {
          pt = palloc_get (PAL_ZERO);
          if (pt == NULL) 
            return NULL; 
      
          *pde = pde_create (pt);
        }
      else
        return NULL;
    }

  /* Return the page table entry. */
  pt = pde_get_pt (*pde);
  return &pt[pt_no (uaddr)];
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE in page directory PD.
   UPAGE must not already be mapped.
   If WRITABLE is true, the new page is read/write;
   otherwise it is read-only.
   Returns true if successful, false if memory allocation
   failed. */
bool
pagedir_set_page (uint32_t *pd, void *upage, void *kpage,
                  bool writable) 
{
  uint32_t *pte;

  ASSERT (pg_ofs (upage) == 0);
  ASSERT (pg_ofs (kpage) == 0);
  ASSERT (upage < PHYS_BASE);
  ASSERT (lookup_page (pd, upage, false) == NULL);

  pte = lookup_page (pd, upage, true);
  if (pte != NULL) 
    {
      *pte = pte_create_user (kpage, writable);
      return true;
    }
  else
    return false;
}

/* Returns the kernel virtual address that user virtual address
   UADDR is mapped to in PD, or a null pointer if there is no
   mapping. */
void *
pagedir_get_page (uint32_t *pd, const void *uaddr) 
{
  uint32_t *pte = lookup_page (pd, (void *) uaddr, false);
  return pte != NULL && *pte != 0 ? pte_get_page (*pte) : NULL;
}

/* Loads page directory PD into the CPU's page directory base
   register. */
void
pagedir_activate (uint32_t *pd) 
{
  if (pd == NULL)
    pd = base_page_dir;
  asm volatile ("movl %0,%%cr3" :: "r" (vtop (pd)));
}
