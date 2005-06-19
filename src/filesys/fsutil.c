#include "filesys/fsutil.h"
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/disk.h"
#include "threads/mmu.h"
#include "threads/malloc.h"
#include "threads/palloc.h"

/* List files in the root directory. */
void
fsutil_ls (char **argv UNUSED) 
{
  printf ("Files in the root directory:\n");
  filesys_list ();
  printf ("End of listing.\n");
}

/* Prints the contents of file ARGV[1] to the system console as
   hex and ASCII. */
void
fsutil_cat (char **argv)
{
  const char *filename = argv[1];
  
  struct file *file;
  char *buffer;

  printf ("Printing '%s' to the console...\n", filename);
  file = filesys_open (filename);
  if (file == NULL)
    PANIC ("%s: open failed", filename);
  buffer = palloc_get_page (PAL_ASSERT);
  for (;;) 
    {
      off_t pos = file_tell (file);
      off_t n = file_read (file, buffer, PGSIZE);
      if (n == 0)
        break;

      hex_dump (pos, buffer, n, true); 
    }
  palloc_free_page (buffer);
  file_close (file);
}

/* Deletes file ARGV[1]. */
void
fsutil_rm (char **argv) 
{
  const char *filename = argv[1];
  
  printf ("Deleting '%s'...\n", filename);
  if (!filesys_remove (filename))
    PANIC ("%s: delete failed\n", filename);
}

/* Copies from the "scratch" disk, hdc or hd1:0 to file ARGV[1]
   in the filesystem.

   The current sector on the scratch disk must begin with the
   string "PUT\0" followed by a 32-bit little-endian integer
   indicating the file size in bytes.  Subsequent sectors hold
   the file content.

   The first call to this function will read starting at the
   beginning of the scratch disk.  Later calls advance across the
   disk.  This disk position is independent of that used for
   fsutil_get(), so all `put's should precede all `get's. */
void
fsutil_put (char **argv) 
{
  static disk_sector_t sector = 0;

  const char *filename = argv[1];
  struct disk *src;
  struct file *dst;
  off_t size;
  void *buffer;

  printf ("Putting '%s' into the file system...\n", filename);

  /* Allocate buffer. */
  buffer = malloc (DISK_SECTOR_SIZE);
  if (buffer == NULL)
    PANIC ("couldn't allocate buffer");

  /* Open source disk and read file size. */
  src = disk_get (1, 0);
  if (src == NULL)
    PANIC ("couldn't open source disk (hdc or hd1:0)");

  /* Read file size. */
  disk_read (src, sector++, buffer);
  if (memcmp (buffer, "PUT", 4))
    PANIC ("%s: missing PUT signature on scratch disk", filename);
  size = ((int32_t *) buffer)[1];
  if (size < 0)
    PANIC ("%s: invalid file size %d", filename, size);
  
  /* Create destination file. */
  if (!filesys_create (filename, size))
    PANIC ("%s: create failed", filename);
  dst = filesys_open (filename);
  if (dst == NULL)
    PANIC ("%s: open failed", filename);

  /* Do copy. */
  while (size > 0)
    {
      int chunk_size = size > DISK_SECTOR_SIZE ? DISK_SECTOR_SIZE : size;
      disk_read (src, sector++, buffer);
      if (file_write (dst, buffer, chunk_size) != chunk_size)
        PANIC ("%s: write failed with %"PROTd" bytes unwritten",
               filename, size);
      size -= chunk_size;
    }

  /* Finish up. */
  file_close (dst);
  free (buffer);
}

/* Copies file FILENAME from the file system to the scratch disk.

   The current sector on the scratch disk will receive "GET\0"
   followed by the file's size in bytes as a 32-bit,
   little-endian integer.  Subsequent sectors receive the file's
   data.

   The first call to this function will write starting at the
   beginning of the scratch disk.  Later calls advance across the
   disk.  This disk position is independent of that used for
   fsutil_put(), so all `put's should precede all `get's. */
void
fsutil_get (char **argv)
{
  static disk_sector_t sector = 0;

  const char *filename = argv[1];
  void *buffer;
  struct file *src;
  struct disk *dst;
  off_t size;

  printf ("Getting '%s' from the file system...\n", filename);

  /* Allocate buffer. */
  buffer = malloc (DISK_SECTOR_SIZE);
  if (buffer == NULL)
    PANIC ("couldn't allocate buffer");

  /* Open source file. */
  src = filesys_open (filename);
  if (src == NULL)
    PANIC ("%s: open failed", filename);
  size = file_length (src);

  /* Open target disk. */
  dst = disk_get (1, 0);
  if (dst == NULL)
    PANIC ("couldn't open target disk (hdc or hd1:0)");
  
  /* Write size to sector 0. */
  memset (buffer, 0, DISK_SECTOR_SIZE);
  memcpy (buffer, "GET", 4);
  ((int32_t *) buffer)[1] = size;
  disk_write (dst, sector++, buffer);
  
  /* Do copy. */
  while (size > 0) 
    {
      int chunk_size = size > DISK_SECTOR_SIZE ? DISK_SECTOR_SIZE : size;
      if (sector >= disk_size (dst))
        PANIC ("%s: out of space on scratch disk", filename);
      if (file_read (src, buffer, chunk_size) != chunk_size)
        PANIC ("%s: read failed with %"PROTd" bytes unread", filename, size);
      memset (buffer + chunk_size, 0, DISK_SECTOR_SIZE - chunk_size);
      disk_write (dst, sector++, buffer);
      size -= chunk_size;
    }

  /* Finish up. */
  file_close (src);
  free (buffer);
}
