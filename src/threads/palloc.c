#include "threads/palloc.h"
#include <debug.h>
#include <list.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "threads/init.h"
#include "threads/loader.h"
#include "threads/mmu.h"
#include "threads/synch.h"

/* Page allocator.  Hands out memory in page-size chunks.
   See malloc.h for an allocator that hands out smaller
   chunks.

   System memory is divided into two "pools" called the kernel
   and user pools.  The user pool is for user (virtual) memory
   pages, the kernel pool for everything else.  The idea here is
   that the kernel needs to have memory for its own operations
   even if user processes are swapping like mad.

   By default, half of system RAM is given to the kernel pool and
   half to the user pool.  That should be huge overkill for the
   kernel pool, but that's just fine for demonstration purposes.

   Within each pool, we simply use a linked list of free pages.
   It would be straightforward to add all available memory to
   this free list at initialization time.  In practice, though,
   that's really slow because it causes the emulator we're
   running under to have to fault in every page of memory.  So
   instead we only add pages to the free list as needed. */

/* A free page owned by the page allocator. */
struct page
  {
    list_elem elem;             /* Free list element. */
  };

/* A memory pool. */
struct pool
  {
    struct lock lock;                   /* Mutual exclusion. */
    uint8_t *start, *end;               /* Start and end of pool. */
    uint8_t *uninit;                    /* First page not yet in free_list. */
    struct list free_list;              /* Free pages. */
  };

/* Two pools: one for kernel data, one for user pages. */
struct pool kernel_pool, user_pool;

static void init_pool (struct pool *, void *start, void *end,
                       const char *name);
static bool page_from_pool (const struct pool *, void *page);

/* Initializes the page allocator. */
void
palloc_init (void) 
{
  /* End of the kernel as recorded by the linker.
     See kernel.lds.S. */
  extern char _end;

  /* Free memory. */
  uint8_t *free_start = pg_round_up (&_end);
  uint8_t *free_end = ptov (ram_pages * PGSIZE);
  size_t free_pages = (free_end - free_start) / PGSIZE;
  uint8_t *free_middle = free_start + free_pages / 2 * PGSIZE;

  /* Give half of memory to kernel, half to user. */
  init_pool (&kernel_pool, free_start, free_middle, "kernel pool");
  init_pool (&user_pool, free_middle, free_end, "user pool");
}

/* Obtains and returns a free page.  If PAL_USER is set, the page
   is obtained from the user pool, otherwise from the kernel
   pool.  If PAL_ZERO is set in FLAGS, then the page is filled
   with zeros.  If no pages are available, returns a null
   pointer, unless PAL_ASSERT is set in FLAGS, in which case the
   kernel panics. */
void *
palloc_get (enum palloc_flags flags)
{
  struct pool *pool = flags & PAL_USER ? &user_pool : &kernel_pool;
  struct page *page;

  lock_acquire (&pool->lock);

  /* If there's a page in the free list, take it.
     Otherwise, if there's a page not yet added to the free list,
     use it.
     Otherwise, we're out of memory. */
  if (!list_empty (&pool->free_list))
    page = list_entry (list_pop_front (&pool->free_list), struct page, elem);
  else if (pool->uninit < pool->end)
    {
      page = (struct page *) pool->uninit;
      pool->uninit += PGSIZE;
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

  lock_release (&pool->lock);
  
  return page;
}

/* Frees PAGE. */
void
palloc_free (void *page_) 
{
  struct pool *pool;
  struct page *page = page_;

  ASSERT (page == pg_round_down (page));
  if (page_from_pool (&kernel_pool, page))
    pool = &kernel_pool;
  else if (page_from_pool (&user_pool, page))
    pool = &user_pool;
  else
    PANIC ("freeing invalid pointer");

#ifndef NDEBUG
  memset (page, 0xcc, PGSIZE);
#endif

  lock_acquire (&pool->lock);
  list_push_front (&pool->free_list, &page->elem);
  lock_release (&pool->lock);
}

/* Initializes pool P as starting at START and ending at END,
   naming it NAME for debugging purposes. */
static void
init_pool (struct pool *p, void *start, void *end, const char *name) 
{
  ASSERT (pg_ofs (start) == 0);
  ASSERT (pg_ofs (end) == 0);

  printf ("%d kB allocated for %s.\n",
          (PGSIZE / 1024) * (pg_no (end) - pg_no (start)), name);

  lock_init (&p->lock, name);
  p->start = p->uninit = start;
  p->end = end;
  list_init (&p->free_list);
}

/* Returns true if PAGE was allocated from POOL,
   false otherwise. */
static bool
page_from_pool (const struct pool *pool, void *page_) 
{
  uint8_t *page = page_;

  return page >= pool->start && page < pool->uninit;
}
