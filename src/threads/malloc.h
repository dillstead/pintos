#ifndef HEADER_MALLOC_H
#define HEADER_MALLOC_H

#include "debug.h"
#include <stddef.h>

void malloc_init (void);
void *malloc (size_t) ATTRIBUTE ((malloc));
void *calloc (size_t, size_t) ATTRIBUTE ((malloc));
void free (void *);

#endif /* malloc.h */
