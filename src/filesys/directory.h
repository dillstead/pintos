#ifndef FILESYS_DIRECTORY_H
#define FILESYS_DIRECTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "devices/disk.h"

/* Maximum length of a file name component.
   This is the traditional UNIX maximum length.
   After directories are implemented, this maximum length may be
   retained, but much longer full path names must be allowed. */
#define NAME_MAX 14

struct inode;
struct dir;
bool dir_create (disk_sector_t sector, size_t entry_cnt);
bool dir_open (struct inode *, struct dir **);
bool dir_open_root (struct dir **);
void dir_close (struct dir *);
bool dir_lookup (const struct dir *, const char *name, struct inode **);
bool dir_add (struct dir *, const char *name, disk_sector_t);
bool dir_remove (struct dir *, const char *name);
void dir_list (const struct dir *);

#endif /* filesys/directory.h */
