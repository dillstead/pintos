#ifndef THREADS_PALLOC_H
#define THREADS_PALLOC_H

#include <stdint.h>

enum palloc_flags
  {
    PAL_ASSERT = 001,           /* Panic on failure. */
    PAL_ZERO = 002,             /* Zero page contents. */
    PAL_USER = 004              /* User page. */
  };

void palloc_init (void);
void *palloc_get (enum palloc_flags);
void palloc_free (void *);

#endif /* threads/palloc.h */
