#ifndef HEADER_PALLOC_H
#define HEADER_PALLOC_H 1

/* Page allocator.  Hands out memory in page-size chunks.
   See malloc.h for an allocator that hands out smaller
   chunks. */

#include <stdint.h>

enum palloc_flags
  {
    PAL_ASSERT = 001,           /* Panic on failure. */
    PAL_ZERO = 002              /* Zero page contents. */
  };

void palloc_init (uint8_t *start, uint8_t *end);
void *palloc_get (enum palloc_flags);
void palloc_free (void *);

#endif /* palloc.h */
