#ifndef HEADER_RANDOM_H
#define HEADER_RANDOM_H 1

#include <stddef.h>

void random_init (unsigned seed);
void random_bytes (void *, size_t);
unsigned long random_ulong (void);

#endif /* random.h */
