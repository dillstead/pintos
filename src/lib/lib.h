#ifndef HEADER_LIB_H
#define HEADER_LIB_H 1

#include <stdarg.h>
#include <stddef.h>

#define ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP) * (STEP))
#define DIV_ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))
#define ROUND_DOWN(X, STEP) ((X) / (STEP) * (STEP))

void *memset (void *, int, size_t);
void *memcpy (void *, const void *, size_t);
void *memmove (void *, const void *, size_t);
void *memchr (const void *, int, size_t);
int memcmp (const void *, const void *, size_t);

char *strchr (const char *, int);
size_t strlcpy (char *, const char *, size_t);
size_t strlen (const char *);

void vprintk (const char *, va_list);
void printk (const char *, ...)
     __attribute__ ((format (printf, 1, 2)));

static inline int
isdigit (int c) 
{
  return c >= '0' && c <= '9';
}

#endif /* lib.h */
