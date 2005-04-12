#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"

/* Disk used for filesystem. */
extern struct disk *filesys_disk;

/* The free map file, opened by filesys_init() and never
   closed. */
extern struct file *free_map_file;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);
bool filesys_list (void);

void filesys_self_test (void);

#endif /* filesys/filesys.h */
