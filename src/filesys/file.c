#include "filesys/file.h"
#include <debug.h>
#include <string.h>
#include "filesys/directory.h"
#include "filesys/filehdr.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"

bool
file_open (struct file *file, disk_sector_t hdr_sector) 
{
  file->hdr = filehdr_read (hdr_sector);
  file->bounce = malloc (DISK_SECTOR_SIZE);
  file->pos = 0;
  if (file->hdr != NULL && file->bounce != NULL)
    return true;
  else
    {
      filehdr_destroy (file->hdr);
      free (file->bounce);
      return false;
    }
}

void
file_close (struct file *file) 
{
  filehdr_destroy (file->hdr);
}

off_t
file_read (struct file *file, void *buffer, off_t size) 
{
  off_t bytes_read = file_read_at (file, buffer, size, file->pos);
  file->pos += bytes_read;
  return bytes_read;
}

off_t
file_read_at (struct file *file, void *buffer_, off_t size,
              off_t file_ofs) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      off_t sector_idx = filehdr_byte_to_sector (file->hdr, file_ofs);
      int sector_ofs = file_ofs % DISK_SECTOR_SIZE;

      /* Bytes left in file, bytes left in sector, lesser of the two. */
      off_t file_left = filehdr_length (file->hdr) - file_ofs;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = file_left < sector_left ? file_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size == 0)
        break;

      /* Read sector into bounce buffer, then copy into caller's
         buffer. */
      disk_read (filesys_disk, sector_idx, file->bounce);
      memcpy (buffer + bytes_read, file->bounce + sector_ofs, chunk_size);

      /* Advance. */
      size -= chunk_size;
      file_ofs += chunk_size;
      bytes_read += chunk_size;
    }

  return bytes_read;
}

off_t
file_write (struct file *file, const void *buffer, off_t size) 
{
  off_t bytes_written = file_write_at (file, buffer, size, file->pos);
  file->pos += bytes_written;
  return bytes_written;
}

off_t
file_write_at (struct file *file, const void *buffer_, off_t size,
               off_t file_ofs) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  while (size > 0) 
    {
      /* Starting byte offset within sector to read. */
      off_t sector_idx = filehdr_byte_to_sector (file->hdr, file_ofs);
      int sector_ofs = file_ofs % DISK_SECTOR_SIZE;

      /* Bytes left in file, bytes left in sector, lesser of the two. */
      off_t file_left = filehdr_length (file->hdr) - file_ofs;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = file_left < sector_left ? file_left : sector_left;

      /* Number of bytes to actually writen into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size == 0)
        break;

      /* If the sector contains data before or after the chunk
         we're writing, then we need to read in the sector
         first.  Otherwise we start with a sector of all zeros. */
      if (sector_ofs > 0 || chunk_size < sector_ofs)
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

off_t
file_length (struct file *file) 
{
  ASSERT (file != NULL);
  return filehdr_length (file->hdr);
}

void
file_seek (struct file *file, off_t file_ofs) 
{
  ASSERT (file != NULL);
  ASSERT (file_ofs >= 0);
  file->pos = file_ofs;
}

off_t
file_tell (struct file *file) 
{
  ASSERT (file != NULL);
  return file->pos;
}
