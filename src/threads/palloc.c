#include "threads/palloc.h"
#include <bitmap.h>
#include <debug.h>
#include <inttypes.h>
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
    size_t first_page;                  /* Page number of first page. */
    size_t page_cnt;                    /* Number of pages. */
  };

/* Tracks pages in use, with a lock protecting it. */
static struct bitmap *used_map;
static struct lock used_map_lock;

/* Two pools: one for kernel data, one for user pages. */
static struct pool kernel_pool, user_pool;

/* Maximum number of pages to put in user pool. */
size_t max_user_pages = SIZE_MAX;

static void init_pool (struct pool *pool, size_t first_page, size_t page_cnt,
                       const char *name);
static bool page_from_pool (const struct pool *, void *page);

/* Initializes the page allocator. */
void
palloc_init (void) 
{
  /* used_map from 1 MB as long as necessary. */
  size_t bitmap_start = 1024;
  size_t bitmap_pages = DIV_ROUND_UP (bitmap_needed_bytes (ram_pages), PGSIZE);

  /* Free space from the bitmap to the end of RAM. */
  size_t free_start = bitmap_start + bitmap_pages;
  size_t free_pages = ram_pages - free_start;

  /* Kernel and user get half of free space each.
     User space can be limited by max_user_pages. */
  size_t half_free = free_pages / 2;
  size_t kernel_pages = half_free;
  size_t user_pages = half_free < max_user_pages ? half_free : max_user_pages;

  used_map = bitmap_create_preallocated (ram_pages,
                                         ptov (bitmap_start * PGSIZE),
                                         bitmap_pages * PGSIZE);
  init_pool (&kernel_pool, free_start, kernel_pages, "kernel pool");
  init_pool (&user_pool, free_start + kernel_pages, user_pages, "use pool");
  lock_init (&used_map_lock, "used_map");
}

/* Initializes POOL to start (named NAME) at physical page number
   FIRST_PAGE and continue for PAGE_CNT pages. */
static void
init_pool (struct pool *pool, size_t first_page, size_t page_cnt,
           const char *name) 
{
  printf ("%zu pages available in %s.\n", page_cnt, name);
  pool->first_page = first_page;
  pool->page_cnt = page_cnt;
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
  struct pool *pool;
  void *pages;
  size_t page_idx;

  if (page_cnt == 0)
    return NULL;

  pool = flags & PAL_USER ? &user_pool : &kernel_pool;

  lock_acquire (&used_map_lock);
  page_idx = bitmap_scan_and_flip (used_map,
                                   pool->first_page, pool->page_cnt,
                                   false);
  lock_release (&used_map_lock);

  if (page_idx != BITMAP_ERROR)
    pages = ptov (PGSIZE * page_idx);
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

  page_idx = vtop (pages) / PGSIZE;

#ifndef NDEBUG
  memset (pages, 0xcc, PGSIZE * page_cnt);
#endif

  ASSERT (bitmap_all (used_map, page_idx, page_cnt));
  bitmap_set_multiple (used_map, page_idx, page_cnt, false);
}

/* Frees the page at PAGE. */
void
palloc_free_page (void *page) 
{
  palloc_free_multiple (page, 1);
}

/* Returns true if PAGE was allocated from POOL,
   false otherwise. */
static bool
page_from_pool (const struct pool *pool, void *page) 
{
  size_t phys_page_no = vtop (page) / PGSIZE;

  return (phys_page_no >= pool->first_page
          && phys_page_no < pool->first_page + pool->page_cnt);
}
