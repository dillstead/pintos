#include "filesys/filehdr.h"
#include <bitmap.h>
#include <debug.h>
#include <stdio.h>
#include "filesys/filesys.h"
#include "threads/malloc.h"

/* Allocates sectors from bitmap B for the content of a file
   whose size is LENGTH bytes, and returns a new `struct filehdr'
   properly initialized for the file.
   It is the caller's responsible to allocate a sector for the
   file header itself, and to write the file header and bitmap
   to disk.
   If memory or disk allocation fails, returns a null pointer,
   leaving bitmap B is unchanged. */
struct filehdr *
filehdr_allocate (struct bitmap *b, off_t length) 
{
  struct filehdr *h;
  size_t sector_cnt;

  ASSERT (b != NULL);
  ASSERT (length >= 0);

  sector_cnt = (length / DISK_SECTOR_SIZE) + (length % DISK_SECTOR_SIZE > 0);
  if (sector_cnt > DIRECT_CNT)
    return false;

  h = calloc (1, sizeof *h);
  if (h == NULL)
    return NULL;

  h->length = length;
  while (h->sector_cnt < sector_cnt)
    {
      size_t sector = bitmap_find_and_set (b);
      if (sector == BITMAP_ERROR)
        {
          filehdr_deallocate (h, b);
          free (h);
          return NULL;
        }
      h->sectors[h->sector_cnt++] = sector;
    }

  return h;
}

/* Marks the sectors for H's content as free in bitmap B.
   Neither H's own disk sector nor its memory are freed. */
void
filehdr_deallocate (struct filehdr *h, struct bitmap *b) 
{
  size_t i;
  
  ASSERT (h != NULL);
  ASSERT (b != NULL);

  for (i = 0; i < h->sector_cnt; i++)
    bitmap_reset (b, h->sectors[i]);
}

/* Reads a file header from FILEHDR_SECTOR
   and returns a new `struct filehdr' that contains it.
   Returns a null pointer fi memory allocation fails. */
struct filehdr *
filehdr_read (disk_sector_t filehdr_sector) 
{
  struct filehdr *h = calloc (1, sizeof *h);
  if (h == NULL)
    return NULL;

  ASSERT (sizeof *h == DISK_SECTOR_SIZE);
  disk_read (filesys_disk, filehdr_sector, h);

  return h;
}

/* Writes H to disk in sector FILEHDR_SECTOR. */
void
filehdr_write (const struct filehdr *h, disk_sector_t filehdr_sector) 
{
  ASSERT (h != NULL);
  ASSERT (sizeof *h == DISK_SECTOR_SIZE);
  disk_write (filesys_disk, filehdr_sector, h);
}

/* Frees the memory (but not the on-disk sector) associated with
   H. */
void
filehdr_destroy (struct filehdr *h) 
{
  free (h);
}

/* Returns the disk sector that contains byte offset POS within
   the file with header H.
   Returns -1 if H does not contain data for a byte at offset
   POS. */
disk_sector_t
filehdr_byte_to_sector (const struct filehdr *h, off_t pos) 
{
  size_t idx;

  ASSERT (h != NULL);

  idx = pos / DISK_SECTOR_SIZE;
  return idx < h->sector_cnt ? h->sectors[idx] : (disk_sector_t) -1;
}

/* Returns the length, in bytes, of the file with header H, */
off_t
filehdr_length (const struct filehdr *h)
{
  ASSERT (h != NULL);
  return h->length;
}

/* Prints a representation of H to the system console. */
void
filehdr_print (const struct filehdr *h) 
{
  size_t i;
  
  printf ("File header: %jd bytes, %zd sectors (",
          (intmax_t) h->length, h->sector_cnt);

  /* This loop could be unsafe for large h->sector_cnt, can you
     see why? */
  for (i = 0; i < h->sector_cnt; i++) 
    {
      if (i != 0)
        printf (", ");
      printf ("%jd", (intmax_t) h->sectors[i]); 
    }
  printf (")\n");
}
