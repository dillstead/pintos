#ifndef HEADER_LIB_H
#define HEADER_LIB_H 1

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include "debug.h"

/* <string.h> */
void *memset (void *, int, size_t);
void *memcpy (void *, const void *, size_t);
void *memmove (void *, const void *, size_t);
void *memchr (const void *, int, size_t);
int memcmp (const void *, const void *, size_t);

char *strchr (const char *, int);
size_t strlcpy (char *, const char *, size_t);
size_t strlen (const char *);
size_t strnlen (const char *, size_t);
int strcmp (const char *, const char *);
char *strtok_r (char *, const char *, char **);

/* <stdlib.h> */
int atoi (const char *);

/* <stdio.h> */
int vsnprintf (char *, size_t, const char *, va_list) PRINTF_FORMAT (3, 0);
int snprintf (char *, size_t, const char *, ...) PRINTF_FORMAT (3, 4);

/* <ctype.h> */
static inline int islower (int c) { return c >= 'a' && c <= 'z'; }
static inline int isupper (int c) { return c >= 'A' && c <= 'Z'; }
static inline int isalpha (int c) { return islower (c) || isupper (c); }
static inline int isdigit (int c) { return c >= '0' && c <= '9'; }
static inline int isalnum (int c) { return isalpha (c) || isdigit (c); }
static inline int isxdigit (int c) {
  return isdigit (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static inline int isspace (int c) { return strchr (" \f\n\r\t\v", c) != NULL; }
static inline int isgraph (int c) { return c >= 33 && c < 127; }
static inline int isprint (int c) { return c >= 32 && c < 127; }
static inline int iscntrl (int c) { return c >= 0 && c < 32; }
static inline int isascii (int c) { return c >= 0 && c < 128; }
static inline int ispunct (int c) {
  return isprint (c) && !isalnum (c) && !isspace (c);
}

/* Nonstandard. */

/* Yields X rounded up to the nearest multiple of STEP.
   For X >= 0, STEP >= 1 only. */
#define ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP) * (STEP))

/* Yields X divided by STEP, rounded up.
   For X >= 0, STEP >= 1 only. */
#define DIV_ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))

/* Yields X rounded down to the nearest multiple of STEP.
   For X >= 0, STEP >= 1 only. */
#define ROUND_DOWN(X, STEP) ((X) / (STEP) * (STEP))

/* There is no DIV_ROUND_DOWN.   It would be simply X / STEP. */

void vprintk (const char *, va_list) PRINTF_FORMAT (1, 0);
void printk (const char *, ...) PRINTF_FORMAT (1, 2);
void hex_dump (const void *, size_t size, bool ascii);

#endif /* lib.h */
