#ifndef HEADER_FILESYS_H
#define HEADER_FILESYS_H 1

#include <stdbool.h>
#include <stdint.h>
#include "filesys/off_t.h"

/* Disk used for filesystem. */
extern struct disk *filesys_disk;

struct file;
void filesys_init (bool format);
bool filesys_create (const char *name, off_t initial_size);
bool filesys_open (const char *name, struct file *);
bool filesys_remove (const char *name);
bool filesys_list (void);
bool filesys_dump (void);

void filesys_self_test (void);

#endif /* filesys.h */
