#include "threads/paging.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "threads/init.h"
#include "threads/mmu.h"
#include "threads/palloc.h"

static uint32_t *base_page_dir;

static uint32_t make_pde (uint32_t *pt) {
  ASSERT (pg_ofs (pt) == 0);
  return vtop (pt) | PG_U | PG_P | PG_W;
}

static uint32_t make_kernel_pte (uint32_t *page, bool writable) {
  ASSERT (pg_ofs (page) == 0);
  return vtop (page) | PG_P | (writable ? PG_W : 0);
}

static uint32_t make_user_pte (uint32_t *page, bool writable) {
  return make_kernel_pte (page, writable) | PG_U;
}

static uint32_t *
pde_get_pt (uint32_t pde) 
{
  ASSERT (pde & PG_P);

  return ptov (pde & ~PGMASK);
}

static void *
pte_get_page (uint32_t pte) 
{
  ASSERT (pte & PG_P);
  
  return ptov (pte & ~PGMASK);
}

/* Populates the base page directory and page table with the
   kernel virtual mapping, and then sets up the CPU to use the
   new page directory.

   At the time this function is called, the active page table
   (set up by loader.S) only maps the first 4 MB of RAM, so we
   should not try to use extravagant amounts of memory.
   Fortunately, there is no need to do so. */
void
paging_init (void)
{
  uint32_t *pd, *pt;
  size_t page;

  pd = base_page_dir = palloc_get (PAL_ASSERT | PAL_ZERO);
  pt = NULL;
  for (page = 0; page < ram_pages; page++) 
    {
      uintptr_t paddr = page * PGSIZE;
      void *vaddr = ptov (paddr);
      size_t pde_idx = pd_no (vaddr);
      size_t pte_idx = pt_no (vaddr);

      if (pd[pde_idx] == 0)
        {
          pt = palloc_get (PAL_ASSERT | PAL_ZERO);
          pd[pde_idx] = make_pde (pt);
        }

      pt[pte_idx] = make_kernel_pte (vaddr, true);
    }

  pagedir_activate (pd);
}

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
  void *kpage, *upage;
  unsigned pde_idx;

  /* Destroy user pages. */
  for (kpage = pagedir_first (pd, &upage); kpage != NULL;
       kpage = pagedir_next (pd, &upage)) 
    palloc_free (kpage);

  /* Destroy page table pages. */
  for (pde_idx = 0; pde_idx < pd_no (PHYS_BASE); pde_idx++) 
    {
      uint32_t pde = pd[pde_idx];

      if (pde != 0) 
        {
          uint32_t *pt = pde_get_pt (pde);
          palloc_free (pt);
        }
    }

  /* Destroy page directory. */
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
      
          *pde = make_pde (pt);
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
      *pte = make_user_pte (kpage, writable);
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

static uint32_t *
scan_pt (uint32_t *pt, unsigned pde_idx, unsigned pte_idx, void **upage) 
{
  for (; pte_idx < PGSIZE / sizeof *pt; pte_idx++) 
    {
      uint32_t pte = pt[pte_idx];

      if (pte != 0) 
        {
          void *kpage = pte_get_page (pte);
          if (kpage != NULL) 
            {
              *upage = (void *) ((pde_idx << PDSHIFT) | (pte_idx << PTSHIFT));
              return kpage;
            }
        }
    }
  
  return NULL;
}

static void *
scan_pd (uint32_t *pd, unsigned pde_idx, void **upage) 
{
  for (; pde_idx < pd_no (PHYS_BASE); pde_idx++) 
    {
      uint32_t pde = pd[pde_idx];

      if (pde != 0) 
        {
          void *kpage = scan_pt (pde_get_pt (pde), pde_idx, 0, upage);
          if (kpage != NULL)
            return kpage;
        }
    }
  
  return NULL;
}

void *
pagedir_first (uint32_t *pd, void **upage) 
{
  return scan_pd (pd, 0, upage);
}

void *
pagedir_next (uint32_t *pd, void **upage) 
{
  unsigned pde_idx, pte_idx;
  void *kpage;

  pde_idx = pd_no (*upage);
  pte_idx = pt_no (*upage);
  kpage = scan_pt (pde_get_pt (pd[pde_idx]),
                   pde_idx, pte_idx + 1, upage);
  if (kpage == NULL)
    kpage = scan_pd (pd, pde_idx + 1, upage);
  return kpage;
}

void
pagedir_activate (uint32_t *pd) 
{
  if (pd == NULL)
    pd = base_page_dir;
  asm volatile ("movl %0,%%cr3" :: "r" (vtop (pd)));
}
