#ifndef HEADER_INIT_H
#define HEADER_INIT_H 1

#include <stddef.h>

struct tss *tss;

extern size_t kernel_pages;
extern size_t ram_pages;

extern int argc;
extern char *argv[];

#endif /* init.h */
