#include "filesys/inode.h"
#include <bitmap.h>
#include <list.h>
#include <debug.h>
#include <stdio.h>
#include "filesys/filesys.h"
#include "threads/malloc.h"

/* Number of direct sector pointers in an inode. */
#define DIRECT_CNT ((DISK_SECTOR_SIZE - sizeof (off_t) - sizeof (size_t)) \
                    / sizeof (disk_sector_t))

/* On-disk inode.
   It is exactly DISK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    off_t length;                       /* File size in bytes. */
    size_t sector_cnt;                  /* File size in sectors. */
    disk_sector_t sectors[DIRECT_CNT];  /* Sectors allocated for file. */
  };

/* In-memory inode. */
struct inode 
  {
    list_elem elem;                     /* Element in inode list. */
    disk_sector_t sector;               /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    struct inode_disk data;             /* On-disk data. */
  };

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
struct list open_inodes;

static struct inode *alloc_inode (disk_sector_t);

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Allocates sectors from bitmap B for the content of a file
   whose size is LENGTH bytes, and returns a new `struct inode'
   properly initialized for the file.
   It is the caller's responsible to write the inode to disk with
   inode_commit(), as well as the bitmap.
   If memory or disk allocation fails, returns a null pointer,
   leaving bitmap B is unchanged. */
struct inode *
inode_create (struct bitmap *b, disk_sector_t sector, off_t length) 
{
  struct inode *idx;
  size_t sector_cnt;

  ASSERT (b != NULL);
  ASSERT (length >= 0);

  /* Calculate number of sectors to allocate for file data. */
  sector_cnt = (length / DISK_SECTOR_SIZE) + (length % DISK_SECTOR_SIZE > 0);
  if (sector_cnt > DIRECT_CNT)
    return NULL;

  /* Allocate inode. */
  idx = alloc_inode (sector);
  if (idx == NULL)
    return NULL;

  /* Allocate disk sectors. */
  idx->data.length = length;
  while (idx->data.sector_cnt < sector_cnt)
    {
      size_t sector = bitmap_scan_and_flip (b, 0, 1, false);
      if (sector == BITMAP_ERROR)
        goto error;

      idx->data.sectors[idx->data.sector_cnt++] = sector;
    }

  /* Zero out the file contents. */
  if (sector_cnt > 0) 
    {
      void *zero_sector;
      size_t i;
      
      zero_sector = calloc (1, DISK_SECTOR_SIZE);
      if (zero_sector == NULL)
        goto error;
      for (i = 0; i < sector_cnt; i++)
        disk_write (filesys_disk, idx->data.sectors[i], zero_sector);
      free (zero_sector);
    }

  return idx;

 error:
  inode_remove (idx);
  inode_close (idx);
  return NULL;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (disk_sector_t sector) 
{
  list_elem *e;
  struct inode *idx;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      idx = list_entry (e, struct inode, elem);
      if (idx->sector == sector) 
        {
          idx->open_cnt++;
          return idx;
        }
    }

  /* Allocate inode. */
  idx = alloc_inode (sector);
  if (idx == NULL)
    return NULL;

  /* Read from disk. */
  ASSERT (sizeof idx->data == DISK_SECTOR_SIZE);
  disk_read (filesys_disk, sector, &idx->data);

  return idx;
}

/* Closes inode IDX and writes it to disk.
   If this was the last reference to IDX, frees its memory.
   If IDX was also a removed inode, frees its blocks. */
void
inode_close (struct inode *idx) 
{
  if (idx == NULL)
    return;

  if (--idx->open_cnt == 0)
    {
      if (idx->removed) 
        {
          struct bitmap *free_map;
          size_t i;

          free_map = bitmap_create (disk_size (filesys_disk));
          if (free_map != NULL)
            {
              bitmap_read (free_map, free_map_file);
              
              bitmap_reset (free_map, idx->sector);
              for (i = 0; i < idx->data.sector_cnt; i++)
                bitmap_reset (free_map, idx->data.sectors[i]);

              bitmap_write (free_map, free_map_file);
              bitmap_destroy (free_map);
            }
          else
            printf ("inode_close(): can't free blocks");
        }
      list_remove (&idx->elem);
      free (idx); 
    }
}

/* Writes IDX to disk. */
void
inode_commit (const struct inode *idx) 
{
  ASSERT (idx != NULL);
  ASSERT (sizeof idx->data == DISK_SECTOR_SIZE);
  disk_write (filesys_disk, idx->sector, &idx->data);
}

/* Marks IDX to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *idx) 
{
  ASSERT (idx != NULL);
  idx->removed = true;
}

/* Returns the disk sector that contains byte offset POS within
   the file with inode IDX.
   Returns -1 if IDX does not contain data for a byte at offset
   POS. */
disk_sector_t
inode_byte_to_sector (const struct inode *idx, off_t pos) 
{
  size_t i;

  ASSERT (idx != NULL);

  i = pos / DISK_SECTOR_SIZE;
  return i < idx->data.sector_cnt ? idx->data.sectors[i] : (disk_sector_t) -1;
}

/* Returns the length, in bytes, of the file with inode IDX. */
off_t
inode_length (const struct inode *idx)
{
  ASSERT (idx != NULL);
  return idx->data.length;
}

/* Prints a representation of IDX to the system console. */
void
inode_print (const struct inode *idx) 
{
  size_t i;
  
  printf ("Inode %"PRDSNu": %"PRDSNu" bytes, %zu sectors (",
          idx->sector, idx->data.length, idx->data.sector_cnt);

  /* This loop could be unsafe for large idx->data.sector_cnt, can
     you see why? */
  for (i = 0; i < idx->data.sector_cnt; i++) 
    {
      if (i != 0)
        printf (", ");
      printf ("%"PRDSNu, idx->data.sectors[i]); 
    }
  printf (")\n");
}

/* Returns a newly allocated and initialized inode. */
static struct inode *
alloc_inode (disk_sector_t sector) 
{
  /* Allocate memory. */
  struct inode *idx = calloc (1, sizeof *idx);
  if (idx == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &idx->elem);
  idx->sector = sector;
  idx->open_cnt = 1;
  idx->removed = false;

  return idx;
}
