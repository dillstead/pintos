#ifndef HEADER_FILESYS_H
#define HEADER_FILESYS_H 1

#include <stdbool.h>

void filesys_init (bool reformat);
bool filesys_create (const char *name);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);

#endif /* filesys.h */
