#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H

#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

/* Predefined file handles. */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

/* Standard functions. */
int printf (const char *, ...) PRINTF_FORMAT (1, 2);
int snprintf (char *, size_t, const char *, ...) PRINTF_FORMAT (3, 4);
int vprintf (const char *, va_list) PRINTF_FORMAT (1, 0);
int vsnprintf (char *, size_t, const char *, va_list) PRINTF_FORMAT (3, 0);
int putchar (int);
int puts (const char *);

/* Nonstandard functions. */
#ifndef KERNEL
int hprintf (int, const char *, ...) PRINTF_FORMAT (2, 3);
int vhprintf (int, const char *, va_list) PRINTF_FORMAT (2, 0);
#endif
void hex_dump (const void *, size_t size, bool ascii);

/* Internal functions. */
void __vprintf (const char *format, va_list args,
                void (*output) (char, void *), void *aux);
void __printf (const char *format,
               void (*output) (char, void *), void *aux, ...);

/* Try to be helpful. */
#define sprintf dont_use_sprintf_use_snprintf
#define vsprintf dont_use_vsprintf_use_vsnprintf

#endif /* lib/stdio.h */
