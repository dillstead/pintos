#include "filesys/file.h"
#include <debug.h>
#include <string.h>
#include "filesys/directory.h"
#include "filesys/inode.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"

/* An open file. */
struct file 
  {
    struct inode *inode;        /* File's inode. */
    uint8_t *bounce;            /* Bounce buffer for reads and writes. */
    off_t pos;                  /* Current position. */
  };

/* Opens and returns the file whose inode is in sector
   INODE_SECTOR.  Returns a null pointer if unsuccessful. */
struct file *
file_open (disk_sector_t inode_sector) 
{
  struct file *file = calloc (1, sizeof *file);
  if (file == NULL)
    return NULL;
  
  file->inode = inode_open (inode_sector);
  file->bounce = malloc (DISK_SECTOR_SIZE);
  file->pos = 0;
  if (file->inode == NULL || file->bounce == NULL)
    {
      inode_close (file->inode);
      free (file->bounce);
      return NULL;
    }

  return file;
}

/* Closes FILE. */
void
file_close (struct file *file) 
{
  if (file == NULL)
    return;
  
  inode_close (file->inode);
  free (file->bounce);
  free (file);
}

/* Reads SIZE bytes from FILE into BUFFER,
   starting at the file's current position,
   and advances the current position.
   Returns the number of bytes actually read,
   which may be less than SIZE if end of file is reached. */
off_t
file_read (struct file *file, void *buffer, off_t size) 
{
  off_t bytes_read = file_read_at (file, buffer, size, file->pos);
  file->pos += bytes_read;
  return bytes_read;
}

/* Reads SIZE bytes from FILE into BUFFER,
   starting at offset FILE_OFS in the file.
   The file's current position is unaffected
   Returns the number of bytes actually read,
   which may be less than SIZE if end of file is reached. */
off_t
file_read_at (struct file *file, void *buffer_, off_t size,
              off_t file_ofs) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      disk_sector_t sector_idx;
      int sector_ofs = file_ofs % DISK_SECTOR_SIZE;

      /* Bytes left in file, bytes left in sector, lesser of the two. */
      off_t file_left = inode_length (file->inode) - file_ofs;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = file_left < sector_left ? file_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      /* Read sector into bounce buffer, then copy into caller's
         buffer. */
      sector_idx = inode_byte_to_sector (file->inode, file_ofs);
      disk_read (filesys_disk, sector_idx, file->bounce);
      memcpy (buffer + bytes_read, file->bounce + sector_ofs, chunk_size);

      /* Advance. */
      size -= chunk_size;
      file_ofs += chunk_size;
      bytes_read += chunk_size;
    }

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into FILE,
   starting at the file's current position,
   and advances the current position.
   Returns the number of bytes actually written,
   which may be less than SIZE if end of file is reached.
   (Normally we'd grow the file in that case, but file growth is
   not yet implemented.) */
off_t
file_write (struct file *file, const void *buffer, off_t size) 
{
  off_t bytes_written = file_write_at (file, buffer, size, file->pos);
  file->pos += bytes_written;
  return bytes_written;
}

/* Writes SIZE bytes from BUFFER into FILE,
   starting at offset FILE_OFS in the file.
   The file's current position is unaffected
   Returns the number of bytes actually written,
   which may be less than SIZE if end of file is reached.
   (Normally we'd grow the file in that case, but file growth is
   not yet implemented.) */
off_t
file_write_at (struct file *file, const void *buffer_, off_t size,
               off_t file_ofs) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      off_t sector_idx;
      int sector_ofs = file_ofs % DISK_SECTOR_SIZE;

      /* Bytes left in file, bytes left in sector, lesser of the two. */
      off_t file_left = inode_length (file->inode) - file_ofs;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = file_left < sector_left ? file_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      /* If the sector contains data before or after the chunk
         we're writing, then we need to read in the sector
         first.  Otherwise we start with a sector of all zeros. */
      sector_idx = inode_byte_to_sector (file->inode, file_ofs);
      if (sector_ofs > 0 || chunk_size < sector_left)
        disk_read (filesys_disk, sector_idx, file->bounce);
      else
        memset (file->bounce, 0, DISK_SECTOR_SIZE);
      memcpy (file->bounce + sector_ofs, buffer + bytes_written, chunk_size);
      disk_write (filesys_disk, sector_idx, file->bounce);

      /* Advance. */
      size -= chunk_size;
      file_ofs += chunk_size;
      bytes_written += chunk_size;
    }

  return bytes_written;
}

/* Returns the size of FILE in bytes. */
off_t
file_length (struct file *file) 
{
  ASSERT (file != NULL);
  return inode_length (file->inode);
}

/* Sets the current position in FILE to an offset of FILE_OFS
   bytes from the start of the file. */
void
file_seek (struct file *file, off_t file_ofs) 
{
  ASSERT (file != NULL);
  ASSERT (file_ofs >= 0);
  file->pos = file_ofs;
}

/* Returns the current position in FILE as a byte offset from the
   start of the file. */
off_t
file_tell (struct file *file) 
{
  ASSERT (file != NULL);
  return file->pos;
}
