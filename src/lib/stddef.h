#ifndef __LIB_STDDEF_H
#define __LIB_STDDEF_H

#define NULL ((void *) 0)
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *) 0)->MEMBER)
typedef int ptrdiff_t;
typedef unsigned int size_t;

#endif /* lib/stddef.h */
