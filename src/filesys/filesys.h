#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/off_t.h"

/* Per process maximum number of open files. */
#define MAX_OPEN_FILES 128

/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

/* Block device that contains the file system. */
struct block *fs_device;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size);
struct file *filesys_open (const char *name);
bool filesys_remove (const char *name);
bool process_file_create (const char *name, off_t initial_size);
bool process_file_remove (const char *name);
int process_file_open (const char *name, bool deny_write);
off_t process_file_size (int fd);
off_t process_file_read (int fd, void *buffer_, off_t size);
off_t process_file_write (int fd, const void *buffer, off_t size);
void process_file_seek (int fd, off_t new_pos);
off_t process_file_tell (int fd);
void process_file_close (int fd);
struct file *process_file_get_file (int fd);
#endif /* filesys/filesys.h */
