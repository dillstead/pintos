#ifndef HEADER_MALLOC_H
#define HEADER_MALLOC_H

#include "lib/debug.h"
#include <stddef.h>

void malloc_init (void);
void *malloc (size_t) __attribute__ ((malloc));
void *calloc (size_t, size_t) __attribute__ ((malloc));
void free (void *);

#endif /* malloc.h */
