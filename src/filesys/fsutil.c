#include "filesys/fsutil.h"
#include <debug.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/mmu.h"
#include "threads/palloc.h"

/* Destination filename and size for copy-in operations. */
char *fsutil_copyin_file;
int fsutil_copyin_size;

/* Source filename for copy-out operations. */
char *fsutil_copyout_file;

/* Name of a file print to print to console. */
char *fsutil_print_file;

/* Name of a file to delete. */
char *fsutil_remove_file;

/* List all files in the filesystem to the system console? */
bool fsutil_list_files;

/* Dump full contents of filesystem to the system console? */
bool fsutil_dump_filesys;

/* Copies from the "scratch" disk, hdc or hd1:0,
   to a file named FILENAME in the filesystem.
   The file will be SIZE bytes in length. */
static void
copy_in (const char *filename, off_t size) 
{
  struct disk *src;
  struct file *dst;
  disk_sector_t sector;
  void *buffer;

  /* Open source disk. */
  src = disk_get (1, 0);
  if (src == NULL)
    PANIC ("couldn't open source disk (hdc or hd1:0)");
  if (size > (off_t) disk_size (src) * DISK_SECTOR_SIZE)
    PANIC ("source disk is too small for %lld-byte file",
           (unsigned long long) size);
  
  /* Create destination file. */
  if (!filesys_create (filename, size))
    PANIC ("%s: create failed", filename);
  dst = filesys_open (filename);
  if (dst == NULL)
    PANIC ("%s: open failed", filename);

  /* Do copy. */
  buffer = palloc_get (PAL_ASSERT);
  sector = 0;
  while (size > 0)
    {
      int chunk_size = size > DISK_SECTOR_SIZE ? DISK_SECTOR_SIZE : size;
      disk_read (src, sector++, buffer);
      if (file_write (dst, buffer, chunk_size) != chunk_size)
        PANIC ("%s: write failed with %lld bytes unwritten",
               filename, (unsigned long long) size);
      size -= chunk_size;
    }
  palloc_free (buffer);

  file_close (dst);
}

/* Copies FILENAME from the file system to the scratch disk.
   The first four bytes of the first sector in the disk
   receive the file's size in bytes as a little-endian integer.
   The second and subsequent sectors receive the file's data. */
static void
copy_out (const char *filename) 
{
  void *buffer;
  struct file *src;
  struct disk *dst;
  off_t size;
  disk_sector_t sector;

  buffer = palloc_get (PAL_ASSERT | PAL_ZERO);

  /* Open source file. */
  src = filesys_open (filename);
  if (src == NULL)
    PANIC ("%s: open failed", filename);
  size = file_length (src);

  /* Open target disk. */
  dst = disk_get (1, 0);
  if (dst == NULL)
    PANIC ("couldn't open target disk (hdc or hd1:0)");
  if (size + DISK_SECTOR_SIZE > (off_t) disk_size (dst) * DISK_SECTOR_SIZE)
    PANIC ("target disk is too small for %lld-byte file",
           (unsigned long long) size);
  
  /* Write size to sector 0. */
  *(uint32_t *) buffer = size;
  disk_write (dst, 0, buffer);
  
  /* Do copy. */
  sector = 1;
  while (size > 0) 
    {
      int chunk_size = size > DISK_SECTOR_SIZE ? DISK_SECTOR_SIZE : size;
      if (file_read (src, buffer, chunk_size) != chunk_size)
        PANIC ("%s: read failed with %lld bytes unread",
               filename, (unsigned long long) size);
      disk_write (dst, sector++, buffer);
      size -= chunk_size;
    }
  palloc_free (buffer);

  file_close (src);
}

/* Executes the filesystem operations described by the variables
   declared in fsutil.h. */
void
fsutil_run (void) 
{
  if (fsutil_copyin_file != NULL) 
    copy_in (fsutil_copyin_file, fsutil_copyin_size);

  if (fsutil_copyout_file != NULL)
    copy_out (fsutil_copyout_file);

  if (fsutil_print_file != NULL)
    fsutil_print (fsutil_print_file);

  if (fsutil_remove_file != NULL) 
    {
      if (filesys_remove (fsutil_remove_file))
        printf ("%s: removed\n", fsutil_remove_file);
      else
        PANIC ("%s: remove failed\n", fsutil_remove_file);
    }

  if (fsutil_list_files)
    filesys_list ();

  if (fsutil_dump_filesys)
    filesys_dump ();
}

/* Prints the contents of file FILENAME to the system console as
   hex and ASCII. */
void
fsutil_print (const char *filename) 
{
  struct file *file;
  char *buffer;

  file = filesys_open (filename);
  if (file == NULL)
    PANIC ("%s: open failed", filename);
  buffer = palloc_get (PAL_ASSERT);
  for (;;) 
    {
      off_t n = file_read (file, buffer, PGSIZE);
      if (n == 0)
        break;

      hex_dump (0, buffer, n, true);
    }
  palloc_free (buffer);
  file_close (file);
}
