#include "userprog/pagedir.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "threads/init.h"
#include "threads/mmu.h"
#include "threads/palloc.h"

uint32_t *
pagedir_create (void) 
{
  uint32_t *pd = palloc_get (0);
  memcpy (pd, base_page_dir, PGSIZE);
  return pd;
}

void
pagedir_destroy (uint32_t *pd) 
{
  uint32_t *pde;

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

static uint32_t *
lookup_page (uint32_t *pd, void *upage, bool create)
{
  uint32_t *pt;
  uint32_t *pde;

  ASSERT (pd != NULL);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (upage < PHYS_BASE);

  /* Check for a page table for UPAGE.
     If one is missing, create one if requested. */
  pde = pd + pd_no (upage);
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
  return &pt[pt_no (upage)];
}

bool
pagedir_set_page (uint32_t *pd, void *upage, void *kpage,
                  bool writable) 
{
  uint32_t *pte;

  ASSERT (pg_ofs (kpage) == 0);

  pte = lookup_page (pd, upage, true);
  if (pte != NULL) 
    {
      *pte = pte_create_user (kpage, writable);
      return true;
    }
  else
    return false;
}

void *
pagedir_get_page (uint32_t *pd, void *upage) 
{
  uint32_t *pte = lookup_page (pd, upage, false);
  return pte != NULL && *pte != 0 ? pte_get_page (*pte) : NULL;
}

void
pagedir_clear_page (uint32_t *pd, void *upage)
{
  uint32_t *pte = lookup_page (pd, upage, false);
  if (pte != NULL)
    *pte = 0;
}

void
pagedir_activate (uint32_t *pd) 
{
  if (pd == NULL)
    pd = base_page_dir;
  asm volatile ("movl %0,%%cr3" :: "r" (vtop (pd)));
}
