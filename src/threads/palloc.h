#ifndef THREADS_PALLOC_H
#define THREADS_PALLOC_H

#include <stddef.h>

/* How to allocate pages. */
enum palloc_flags
  {
    PAL_ASSERT = 1 << 0,   /* Panic on failure. */
    PAL_ZERO = 1 << 1,     /* Zero page contents. */
    PAL_USER = 1 << 2,     /* User page. */
    PAL_NOCACHE = 1 << 3   /* Disable memory caching for page. */
  };

extern void *zero_page;

void palloc_init (size_t user_page_limit);
void *palloc_get_page (enum palloc_flags);
void *palloc_get_multiple (enum palloc_flags, size_t page_cnt);
void palloc_free_page (void *);
void palloc_free_multiple (void *, size_t page_cnt);

#endif /* threads/palloc.h */
