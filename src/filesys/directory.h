#ifndef HEADER_DIRECTORY_H
#define HEADER_DIRECTORY_H 1

#include <stdbool.h>
#include <stddef.h>
#include "disk.h"

/* Maximum length of a filename.
   This is the traditional UNIX maximum. */
#define FILENAME_LEN_MAX 14

struct dir 
  {
    size_t entry_cnt;
    struct dir_entry *entries;
  };

struct dir_entry 
  {
    bool in_use;
    char name[FILENAME_LEN_MAX + 1];
    disk_sector_no filehdr_sector;
  };

struct file;
bool dir_init (struct dir *, size_t entry_cnt);
void dir_destroy (struct dir *);
void dir_read (struct dir *, struct file *);
void dir_write (struct dir *, struct file *);
bool dir_lookup (const struct dir *, const char *name, disk_sector_no *);
bool dir_add (struct dir *, const char *name, disk_sector_no);
bool dir_remove (struct dir *, const char *name);
void dir_list (const struct dir *);
void dir_dump (const struct dir *);

#endif /* directory.h */
