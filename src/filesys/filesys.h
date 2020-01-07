#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"

/* Per process maximum number of open files. */
#define MAX_OPEN_FILES 128

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

/* Block device that contains the file system. */
struct block *fs_device;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *path, off_t initial_size);
struct file *filesys_open (const char *path);
bool filesys_remove (const char *path);
bool filesys_chdir (const char *path);
bool filesys_mkdir (const char *path, off_t initial_size);
int fd_open (const char *path, bool deny_write);
off_t fd_size (int fd);
off_t fd_read (int fd, void *buffer_, off_t size);
off_t fd_write (int fd, const void *buffer, off_t size);
void fd_seek (int fd, off_t new_pos);
off_t fd_tell (int fd);
void fd_close (int fd);
bool fd_readdir (int fd, char *name);
bool fd_is_dir (int fd);
block_sector_t fd_inumber (int fd);
struct file *fd_get_file (int fd);

#endif /* filesys/filesys.h */
