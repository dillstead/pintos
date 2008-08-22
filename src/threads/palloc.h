#ifndef THREADS_PALLOC_H
#define THREADS_PALLOC_H

#include <stddef.h>

/* How to allocate pages. */
enum palloc_flags
  {
    PAL_ASSERT = 0x1,           /* Panic on failure. */
    PAL_ZERO = 0x2,             /* Zero page contents. */
    PAL_USER = 0x4,             /* User page. */
    PAL_NOCACHE = 0x8           /* Disable memory caching for page. */
  };

/* Maximum number of pages to put in user pool. */
extern size_t user_page_limit;

extern void* zero_page;

void palloc_init (void);
void *palloc_get_page (enum palloc_flags);
void *palloc_get_multiple (enum palloc_flags, size_t page_cnt);
void palloc_free_page (void *);
void palloc_free_multiple (void *, size_t page_cnt);

#endif /* threads/palloc.h */
