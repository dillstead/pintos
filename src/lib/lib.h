#ifndef HEADER_LIB_H
#define HEADER_LIB_H 1

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include "debug.h"

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
int strcmp (const char *, const char *);
char *strtok_r (char *, const char *, char **);

int atoi (const char *);

void vprintk (const char *, va_list) PRINTF_FORMAT (1, 0);
void printk (const char *, ...) PRINTF_FORMAT (1, 2);
int vsnprintf (char *, size_t, const char *, va_list) PRINTF_FORMAT (3, 0);
int snprintf (char *, size_t, const char *, ...) PRINTF_FORMAT (3, 4);

void hex_dump (const void *, size_t size, bool ascii);

static inline int isdigit (int c) { return c >= '0' && c <= '9'; }
static inline int isprint (int c) { return c >= 32 && c < 127; }
static inline int isgraph (int c) { return c >= 33 && c < 127; }
static inline int isspace (int c) { return strchr (" \t\n\r\v", c) != NULL; }
static inline int islower (int c) { return c >= 'a' && c <= 'z'; }
static inline int isupper (int c) { return c >= 'A' && c <= 'Z'; }
static inline int isalpha (int c) { return islower (c) || isupper (c); }

#endif /* lib.h */
