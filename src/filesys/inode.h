#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/disk.h"

struct bitmap;

void inode_init (void);
bool inode_create (struct bitmap *, disk_sector_t, off_t);
struct inode *inode_open (disk_sector_t);
void inode_close (struct inode *);
void inode_remove (struct inode *);
disk_sector_t inode_byte_to_sector (const struct inode *, off_t);
off_t inode_length (const struct inode *);
void inode_print (const struct inode *);

#endif /* filesys/inode.h */
