#ifndef HEADER_PALLOC_H
#define HEADER_PALLOC_H 1

#include <stdint.h>

enum palloc_flags
  {
    PAL_ASSERT = 001,           /* Panic on failure. */
    PAL_ZERO = 002              /* Zero page contents. */
  };

void palloc_init (void);
void *palloc_get (enum palloc_flags);
void palloc_free (void *);

#endif /* palloc.h */
