#include "palloc.h"
#include <stddef.h>
#include <stdint.h>
#include "debug.h"
#include "init.h"
#include "loader.h"
#include "lib.h"
#include "list.h"
#include "mmu.h"
#include "synch.h"

/* Page allocator.  Hands out memory in page-size chunks.
   See malloc.h for an allocator that hands out smaller
   chunks.

   We simply use a linked list of free pages.  It would be
   straightforward to add all available memory to this free list
   at initialization time.  In practice, though, that's really
   slow because it causes the emulator we're running under to
   have to fault in every page of memory.  So instead we only add
   pages to the free list as needed. */

/* A free page owned by the page allocator. */
struct page
  {
    list_elem free_elem;        /* Free list element. */
  };

/* Keeps multiple threads away from free_pages and
   uninit_start. */
static struct lock lock;

/* List of free pages. */
static struct list free_pages;

/* Range of pages (expressed as byte pointers to the beginnings
   of pages) that we haven't added to the free list yet. */
static uint8_t *uninit_start, *uninit_end;

/* Initializes the page allocator. */
void
palloc_init (void) 
{
  extern char _start, _end;

  /* Kernel static code and data, in 4 kB pages.
     We can figure this out because the linker records the start
     and end of the kernel as _start and _end.  See
     kernel.lds.S. */
  size_t kernel_pages = (&_end - &_start + 4095) / 4096;

  /* Then we know how much is available to allocate. */
  uninit_start = ptov (LOADER_KERN_BASE + kernel_pages * PGSIZE);
  uninit_end = ptov (ram_pages * PGSIZE);

  /* Initialize other variables. */
  lock_init (&lock, "palloc");
  list_init (&free_pages);
}

void *
palloc_get (enum palloc_flags flags)
{
  struct page *page;

  lock_acquire (&lock);

  if (!list_empty (&free_pages))
    page = list_entry (list_pop_front (&free_pages), struct page, free_elem);
  else if (uninit_start < uninit_end) 
    {
      page = (struct page *) uninit_start;
      uninit_start += PGSIZE;
    }
  else
    page = NULL;

  if (page != NULL) 
    {
      if (flags & PAL_ZERO)
        memset (page, 0, PGSIZE);
    }
  else 
    {
      if (flags & PAL_ASSERT)
        PANIC ("palloc_get: out of pages");
    }

  lock_release (&lock);
  
  return page;
}

void
palloc_free (void *page_) 
{
  struct page *page = page_;

  ASSERT (page == pg_round_down (page));
#ifndef NDEBUG
  memset (page, 0xcc, PGSIZE);
#endif

  lock_acquire (&lock);
  list_push_front (&free_pages, &page->free_elem);
  lock_release (&lock);
}
