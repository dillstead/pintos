#ifndef FILESYS_DIRECTORY_H
#define FILESYS_DIRECTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "devices/disk.h"

/* Maximum length of a filename.
   This is the traditional UNIX maximum.
   (This macro name comes from POSIX.1.) */
#define NAME_MAX 14

/* A directory. */
struct dir 
  {
    size_t entry_cnt;           /* Number of entries. */
    struct dir_entry *entries;  /* Array of entries. */
  };

/* A single directory entry. */
struct dir_entry 
  {
    bool in_use;                        /* In use or free? */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    disk_sector_t filehdr_sector;       /* Sector number of header. */
  };

struct file;
bool dir_init (struct dir *, size_t entry_cnt);
void dir_destroy (struct dir *);
void dir_read (struct dir *, struct file *);
void dir_write (struct dir *, struct file *);
bool dir_lookup (const struct dir *, const char *name, disk_sector_t *);
bool dir_add (struct dir *, const char *name, disk_sector_t);
bool dir_remove (struct dir *, const char *name);
void dir_list (const struct dir *);
void dir_dump (const struct dir *);

#endif /* filesys/directory.h */
