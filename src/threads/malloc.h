#ifndef THREADS_MALLOC_H
#define THREADS_MALLOC_H

#include <debug.h>
#include <stddef.h>

void malloc_init (void);
void *malloc (size_t) __attribute__ ((malloc));
void *xmalloc (size_t) __attribute__ ((malloc));
void *calloc (size_t, size_t) __attribute__ ((malloc));
void *xcalloc (size_t, size_t) __attribute__ ((malloc));
void free (void *);

#endif /* threads/malloc.h */
