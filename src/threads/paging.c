#include "paging.h"
#include <stdbool.h>
#include <stddef.h>
#include "init.h"
#include "lib.h"
#include "mmu.h"
#include "palloc.h"

static uint32_t *base_page_dir;

static uint32_t
make_pde (uint32_t *pagetab) 
{
  ASSERT (PGOFS ((uintptr_t) pagetab) == 0);
  
  return vtop (pagetab) | PG_U | PG_P | PG_W;
}

static uint32_t
make_pte (uint32_t *page, bool writable)
{
  uint32_t entry;

  ASSERT (PGOFS ((uintptr_t) page) == 0);
  
  entry = vtop (page) | PG_U | PG_P;
  if (writable)
    entry |= PG_W;
  return entry;
}

static uint32_t *
pde_get_pagetab (uint32_t pde) 
{
  ASSERT (pde & PG_P);

  return ptov (PGROUNDDOWN (pde));
}

static void *
pte_get_page (uint32_t pte) 
{
  ASSERT (pte & PG_P);
  
  return ptov (PGROUNDDOWN (pte));
}

/* Populates the base page directory and page table with the
   kernel virtual mapping, and then sets up the CPU to use the
   new page directory.

   At the time this function is called, the active page table
   only maps the first 4 MB of RAM, so it should not try to use
   extravagant amounts of memory.  Fortunately, there is no need
   to do so. */
void
paging_init (void)
{
  uint32_t *pd, *pt;
  size_t page;

  pd = base_page_dir = palloc_get (PAL_ASSERT | PAL_ZERO);
  pt = NULL;
  for (page = 0; page < ram_pages; page++) 
    {
      uintptr_t paddr = page * NBPG;
      void *vaddr = ptov (paddr);
      size_t pde_idx = PDENO ((uintptr_t) vaddr);
      size_t pte_idx = PTENO ((uintptr_t) vaddr);

      if (pd[pde_idx] == 0)
        {
          pt = palloc_get (PAL_ASSERT | PAL_ZERO);
          pd[pde_idx] = make_pde (pt);
        }

      pt[pte_idx] = make_pte (vaddr, true);
    }

  /* Set the page table. */
  asm volatile ("movl %0,%%cr3" :: "r" (vtop (pd)));
}

uint32_t *
pagedir_create (void) 
{
  uint32_t *pd = palloc_get (0);
  memcpy (pd, base_page_dir, NBPG);
  return pd;
}

void
pagedir_destroy (uint32_t *pd) 
{
  void *kpage, *upage;

  for (kpage = pagedir_first (pd, &upage); kpage != NULL;
       kpage = pagedir_next (pd, &upage)) 
    palloc_free (kpage);
  palloc_free (pd);
}

static uint32_t *
lookup_page (uint32_t *pagedir, void *upage, bool create)
{
  uint32_t *pagetab;
  uint32_t *pde;

  ASSERT (pagedir != NULL);
  ASSERT (PGOFS ((uintptr_t) upage) == 0);
  ASSERT ((uintptr_t) upage < PHYS_BASE);

  /* Check for a page table for UPAGE.
     If one is missing, create one if requested. */
  pde = pagedir + PDENO ((uint32_t) upage);
  if (*pde == 0) 
    {
      if (create)
        {
          pagetab = palloc_get (PAL_ZERO);
          if (pagetab == NULL) 
            return NULL; 
      
          *pde = make_pde (pagetab);
        }
      else
        return NULL;
    }

  /* Return the page table entry. */
  pagetab = pde_get_pagetab (*pde);
  return &pagetab[PTENO ((uintptr_t) upage)];
}

bool
pagedir_set_page (uint32_t *pagedir, void *upage, void *kpage,
                  bool writable) 
{
  uint32_t *pte;

  ASSERT (PGOFS ((uintptr_t) kpage) == 0);

  pte = lookup_page (pagedir, upage, true);
  if (pte != NULL) 
    {
      *pte = make_pte (kpage, writable);
      return true;
    }
  else
    return false;
}

void *
pagedir_get_page (uint32_t *pagedir, void *upage) 
{
  uint32_t *pte = lookup_page (pagedir, upage, false);
  return pte != NULL && *pte != 0 ? pte_get_page (*pte) : NULL;
}

void
pagedir_clear_page (uint32_t *pagedir, void *upage)
{
  uint32_t *pte = lookup_page (pagedir, upage, false);
  if (pte != NULL)
    *pte = 0;
}

static uint32_t *
scan_pt (uint32_t *pt, unsigned pde_idx, unsigned pte_idx, void **upage) 
{
  for (; pte_idx < NBPG / sizeof *pt; pte_idx++) 
    {
      uint32_t pte = pt[pte_idx];

      if (pte != 0) 
        {
          void *kpage = pte_get_page (pte);
          if (kpage != NULL) 
            {
              *upage = (void *) ((pde_idx << PDSHIFT)
                                 | (pte_idx << PGSHIFT));
              return kpage;
            }
        }
    }
  
  return NULL;
}

static void *
scan_pd (uint32_t *pd, unsigned pde_idx, void **upage) 
{
  for (; pde_idx < PDENO (PHYS_BASE); pde_idx++) 
    {
      uint32_t pde = pd[pde_idx];

      if (pde != 0) 
        {
          void *kpage = scan_pt (pde_get_pagetab (pde), pde_idx, 0, upage);
          if (kpage != NULL)
            return kpage;
        }
    }
  
  return NULL;
}

void *
pagedir_first (uint32_t *pagedir, void **upage) 
{
  return scan_pd (pagedir, 0, upage);
}

void *
pagedir_next (uint32_t *pd, void **upage) 
{
  unsigned pde_idx, pte_idx;
  void *kpage;

  pde_idx = PDENO (*upage);
  pte_idx = PTENO (*upage);
  kpage = scan_pt (pde_get_pagetab (pd[pde_idx]),
                   pde_idx, pte_idx + 1, upage);
  if (kpage == NULL)
    kpage = scan_pd (pd, pde_idx + 1, upage);
  return kpage;
}

void pagedir_activate (uint32_t *pagedir);
