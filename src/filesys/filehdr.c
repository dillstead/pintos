#include "filehdr.h"
#include "bitmap.h"
#include "debug.h"
#include "malloc.h"
#include "filesys.h"
#include "lib.h"

struct filehdr *
filehdr_allocate (struct bitmap *b, off_t length) 
{
  struct filehdr *h;
  size_t sector_cnt;

  ASSERT (b != NULL);
  ASSERT (length >= 0);

  h = calloc (1, sizeof *h);
  if (h == NULL)
    return NULL;

  h->length = length;
  sector_cnt = (length / DISK_SECTOR_SIZE) + (length % DISK_SECTOR_SIZE > 0);
  while (h->sector_cnt < sector_cnt)
    {
      size_t sector = bitmap_find_and_set (b);
      if (sector == BITMAP_ERROR)
        {
          filehdr_deallocate (h, b);
          return NULL;
        }
      h->sectors[h->sector_cnt++] = sector;
    }

  return h;
}

void
filehdr_deallocate (struct filehdr *h, struct bitmap *b) 
{
  size_t i;
  
  ASSERT (h != NULL);
  ASSERT (b != NULL);

  for (i = 0; i < h->sector_cnt; i++)
    bitmap_reset (b, h->sectors[i]);
}

struct filehdr *
filehdr_read (disk_sector_no filehdr_sector) 
{
  struct filehdr *h = calloc (1, sizeof *h);
  if (h == NULL)
    return NULL;

  ASSERT (sizeof *h == DISK_SECTOR_SIZE);
  disk_read (filesys_disk, filehdr_sector, h);

  return h;
}

void
filehdr_write (const struct filehdr *h, disk_sector_no filehdr_sector) 
{
  ASSERT (h != NULL);
  ASSERT (sizeof *h == DISK_SECTOR_SIZE);
  disk_write (filesys_disk, filehdr_sector, h);
}

void
filehdr_destroy (struct filehdr *h) 
{
  free (h);
}

disk_sector_no
filehdr_byte_to_sector (const struct filehdr *h, off_t pos) 
{
  size_t idx;

  ASSERT (h != NULL);

  idx = pos / DISK_SECTOR_SIZE;
  return idx < h->sector_cnt ? h->sectors[idx] : (disk_sector_no) -1;
}

off_t
filehdr_length (const struct filehdr *h)
{
  ASSERT (h != NULL);
  return h->length;
}

void filehdr_print (const struct filehdr *);
