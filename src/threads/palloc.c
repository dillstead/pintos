#include "threads/palloc.h"
#include <bitmap.h>
#include <debug.h>
#include <round.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "threads/init.h"
#include "threads/loader.h"
#include "threads/mmu.h"
#include "threads/synch.h"

/* Page allocator.  Hands out memory in page-size (or
   page-multiple) chunks.  See malloc.h for an allocator that
   hands out smaller chunks.

   System memory is divided into two "pools" called the kernel
   and user pools.  The user pool is for user (virtual) memory
   pages, the kernel pool for everything else.  The idea here is
   that the kernel needs to have memory for its own operations
   even if user processes are swapping like mad.

   By default, half of system RAM is given to the kernel pool and
   half to the user pool.  That should be huge overkill for the
   kernel pool, but that's just fine for demonstration purposes. */

/* A memory pool. */
struct pool
  {
    struct lock lock;                   /* Mutual exclusion. */
    struct bitmap *used_map;            /* Bitmap of free pages. */
    uint8_t *start, *end;               /* Start and end of pool. */
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

/* Obtains and returns a group of PAGE_CNT contiguous free pages.
   If PAL_USER is set, the pages are obtained from the user pool,
   otherwise from the kernel pool.  If PAL_ZERO is set in FLAGS,
   then the pages are filled with zeros.  If too few pages are
   available, returns a null pointer, unless PAL_ASSERT is set in
   FLAGS, in which case the kernel panics. */
void *
palloc_get_multiple (enum palloc_flags flags, size_t page_cnt)
{
  struct pool *pool = flags & PAL_USER ? &user_pool : &kernel_pool;
  void *pages;
  size_t page_idx;

  if (page_cnt == 0)
    return NULL;

  lock_acquire (&pool->lock);

  page_idx = bitmap_scan_and_flip (pool->used_map, 0, page_cnt, false);
  if (page_idx != BITMAP_ERROR)
    pages = pool->start + PGSIZE * page_idx;
  else
    pages = NULL;

  if (pages != NULL) 
    {
      if (flags & PAL_ZERO)
        memset (pages, 0, PGSIZE * page_cnt);
    }
  else 
    {
      if (flags & PAL_ASSERT)
        PANIC ("palloc_get: out of pages");
    }

  lock_release (&pool->lock);
  
  return pages;
}

/* Obtains and returns a single free page.
   If PAL_USER is set, the page is obtained from the user pool,
   otherwise from the kernel pool.  If PAL_ZERO is set in FLAGS,
   then the page is filled with zeros.  If no pages are
   available, returns a null pointer, unless PAL_ASSERT is set in
   FLAGS, in which case the kernel panics. */
void *
palloc_get_page (enum palloc_flags flags) 
{
  return palloc_get_multiple (flags, 1);
}

/* Frees the PAGE_CNT pages starting at PAGES. */
void
palloc_free_multiple (void *pages, size_t page_cnt) 
{
  struct pool *pool;
  size_t page_idx;

  ASSERT (pg_ofs (pages) == 0);
  if (pages == NULL || page_cnt == 0)
    return;

  if (page_from_pool (&kernel_pool, pages))
    pool = &kernel_pool;
  else if (page_from_pool (&user_pool, pages))
    pool = &user_pool;
  else
    NOT_REACHED ();

  page_idx = pg_no (pages) - pg_no (pool->start);
  ASSERT (pg_no (pages) + page_cnt <= pg_no (pool->end));

#ifndef NDEBUG
  memset (pages, 0xcc, PGSIZE * page_cnt);
#endif

  lock_acquire (&pool->lock);
  ASSERT (bitmap_all (pool->used_map, page_idx, page_idx + page_cnt));
  bitmap_set_multiple (pool->used_map, page_idx, page_idx + page_cnt, false);
  lock_release (&pool->lock);
}

/* Frees the page at PAGE. */
void
palloc_free_page (void *page) 
{
  palloc_free_multiple (page, 1);
}

/* Initializes pool P as starting at START and ending at END,
   naming it NAME for debugging purposes. */
static void
init_pool (struct pool *p, void *start, void *end, const char *name) 
{
  size_t bitmap_size;
  size_t page_cnt;

  ASSERT (pg_ofs (start) == 0);
  ASSERT (pg_ofs (end) == 0);
  ASSERT (end > start);

  page_cnt = pg_no (end) - pg_no (start);
  printf ("%d kB allocated for %s.\n", (PGSIZE / 1024) * page_cnt, name);

  lock_init (&p->lock, name);
  bitmap_size = ROUND_UP (bitmap_needed_bytes (page_cnt), PGSIZE);
  p->used_map = bitmap_create_preallocated (page_cnt, start, bitmap_size);
  p->start = start + bitmap_size;
  p->end = end;
  ASSERT (p->end > p->start);
}

/* Returns true if PAGE was allocated from POOL,
   false otherwise. */
static bool
page_from_pool (const struct pool *pool, void *page_) 
{
  uint8_t *page = page_;

  return page >= pool->start && page < pool->end;
}
