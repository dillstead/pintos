#include "file.h"

#ifdef FILESYS_STUB
#include "debug.h"
#include "filesys-stub.h"
#include "lib.h"
#include "malloc.h"

bool
file_open (struct file *file, const char *name) 
{
  struct dir dir;
  disk_sector_no hdr_sector;
  bool success = false;

  dir_init (&dir, NUM_DIR_ENTRIES);
  dir_read (&dir, &root_dir_file);
  if (dir_lookup (&dir, name, &hdr_sector))
    success = file_open_sector (file, hdr_sector);

  dir_destroy (&dir);
  return success;
}

bool
file_open_sector (struct file *file, disk_sector_no hdr_sector) 
{
  file->pos = 0;
  return filehdr_read (&file->hdr, hdr_sector);
}

void
file_close (struct file *file) 
{
  filehdr_destroy (file);
}

off_t
file_read (struct file *file, void *buffer, off_t size) 
{
  off_t retval = file_read_at (file, buffer, size, file->pos);
  file->pos += retval;
  return retval;
}

off_t
file_read_at (struct file *file, void *buffer_, off_t size, off_t start) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce;

  bounce = malloc (DISK_SECTOR_SIZE);
  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      off_t sector_idx = filehdr_byte_to_sector (file->hdr);
      int sector_ofs = start % DISK_SECTOR_SIZE;

      /* Bytes left in file, bytes left in sector. */
      off_t file_left = filehdr_size (file->hdr) - start;
      off_t sector_left = DISK_SECTOR_SIZE - sector_ofs;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = file_left < sector_left ? file_left : sector_left;
      if (chunk_size == 0)
        break;

      /* Read sector into bounce buffer, then copy into caller's
         buffer. */
      disk_read (disk, sector_idx, bounce);
      memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);

      /* Advance. */
      size -= chunk_size;
      start += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

off_t
file_write (struct file *file, const void *buffer, off_t size) 
{
  off_t retval = file_write_at (file, buffer, size, file->pos);
  file->pos += retval;
  return retval;
}

off_t
file_write_at (struct file *file, const void *buffer_, off_t size,
               off_t start) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce;

  bounce = malloc (DISK_SECTOR_SIZE);
  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      off_t sector_idx = filehdr_byte_to_sector (file->hdr);
      int sector_ofs = start % DISK_SECTOR_SIZE;

      /* Bytes left in file, bytes left in sector. */
      off_t file_left = filehdr_size (file->hdr) - start;
      off_t sector_left = DISK_SECTOR_SIZE - sector_ofs;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = file_left < sector_left ? file_left : sector_left;
      if (chunk_size == 0)
        break;

      /* Read sector into bounce buffer, then copy into caller's
         buffer. */
      disk_read (disk, sector_idx, bounce);
      memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);

      /* Advance. */
      size -= chunk_size;
      start += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

off_t
file_length (struct file *file) 
{
  int32_t length;

  filesys_stub_lock ();
  filesys_stub_put_string ("length");
  filesys_stub_put_file (file);
  filesys_stub_match_string ("length");
  length = filesys_stub_get_int32 ();
  filesys_stub_unlock ();

  return length;
}

void
file_seek (struct file *file, off_t pos) 
{
  filesys_stub_lock ();
  filesys_stub_put_string ("seek");
  filesys_stub_put_file (file);
  filesys_stub_put_uint32 (pos);
  filesys_stub_match_string ("seek");
  filesys_stub_unlock ();
}

off_t
file_tell (struct file *file) 
{
  int32_t pos;

  filesys_stub_lock ();
  filesys_stub_put_string ("tell");
  filesys_stub_put_file (file);
  filesys_stub_match_string ("tell");
  pos = filesys_stub_get_int32 ();
  filesys_stub_unlock ();

  return pos;
}
#endif /* FILESYS_STUB */
