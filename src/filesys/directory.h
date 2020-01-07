#ifndef FILESYS_DIRECTORY_H
#define FILESYS_DIRECTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "devices/block.h"

/* Maximum length of a file name component.
   This is the traditional UNIX maximum length.
   After directories are implemented, this maximum length may be
   retained, but much longer full path names must be allowed. */
#define NAME_MAX 14

struct inode;
struct file;

bool dir_create (block_sector_t sector, size_t entry_cnt);

/* Reading and writing. */
bool dir_lookup (struct inode *dinode, const char *name, struct inode **inode);
bool dir_add (struct inode *dinode, const char *name,
              block_sector_t inode_sector);
bool dir_remove (struct inode *dinode, const char *name);
bool dir_readdir (struct file *file, char name[NAME_MAX + 1]);
bool dir_is_empty (struct inode *dinode);

#endif /* filesys/directory.h */
