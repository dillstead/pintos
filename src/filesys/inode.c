#include "filesys/inode.h"
#include <bitmap.h>
#include <list.h>
#include <debug.h>
#include <round.h>
#include <stdio.h>
#include "filesys/filesys.h"
#include "threads/malloc.h"

/* On-disk inode.
   Must be exactly DISK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    off_t length;                       /* File size in bytes. */
    disk_sector_t start;                /* Starting sector. */
    uint32_t unused[126];               /* Unused padding. */
  };

/* Returns the number of sectors to allocate for a file SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, DISK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    disk_sector_t sector;               /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    struct inode_disk data;             /* On-disk data. */
  };

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

static void deallocate_inode (const struct inode *);

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode for a file LENGTH bytes in size and
   writes the new inode to sector SECTOR on the file system
   disk.  Allocates sectors for the file from FREE_MAP.
   Returns true if successful.
   Returns false if memory or disk allocation fails.  FREE_MAP
   may be modified regardless. */
bool
inode_create (struct bitmap *free_map, disk_sector_t sector, off_t length) 
{
  static const char zero_sector[DISK_SECTOR_SIZE];
  struct inode_disk *idx;
  size_t start;
  size_t i;

  ASSERT (free_map != NULL);
  ASSERT (length >= 0);

  /* Allocate sectors. */
  start = bitmap_scan_and_flip (free_map, 0, bytes_to_sectors (length), false);
  if (start == BITMAP_ERROR)
    return false;

  /* Create inode. */
  idx = calloc (sizeof *idx, 1);
  if (idx == NULL)
    return false;
  idx->length = length;
  idx->start = start;

  /* Commit to disk. */
  disk_write (filesys_disk, sector, idx);
  for (i = 0; i < bytes_to_sectors (length); i++)
    disk_write (filesys_disk, idx->start + i, zero_sector);

  free (idx);
  return true;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (disk_sector_t sector) 
{
  struct list_elem *e;
  struct inode *idx;

  /* Check whether this inode is already open.
     (A hash table would be better, but the Pintos base code
     avoids using the hash table so that users are free to modify
     it at will.) */
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

  /* Allocate memory. */
  idx = calloc (1, sizeof *idx);
  if (idx == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &idx->elem);
  idx->sector = sector;
  idx->open_cnt = 1;
  idx->removed = false;

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
  /* Ignore null pointer. */
  if (idx == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--idx->open_cnt == 0)
    {
      /* Deallocate blocks if removed. */
      if (idx->removed)
        deallocate_inode (idx);

      /* Remove from inode list and free memory. */
      list_remove (&idx->elem);
      free (idx); 
    }
}

/* Deallocates the blocks allocated for IDX. */
static void
deallocate_inode (const struct inode *idx)
{
  struct bitmap *free_map = bitmap_create (disk_size (filesys_disk));
  if (free_map != NULL) 
    {
      bitmap_read (free_map, free_map_file);
      bitmap_reset (free_map, idx->sector);
      bitmap_set_multiple (free_map, idx->data.start,
                           bytes_to_sectors (idx->data.length), false);
      bitmap_write (free_map, free_map_file);
      bitmap_destroy (free_map);
    }
  else
    printf ("inode_close(): can't free blocks");
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
  ASSERT (idx != NULL);
  ASSERT (pos < idx->data.length);
  return idx->data.start + pos / DISK_SECTOR_SIZE;
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
  ASSERT (idx != NULL);
  printf ("Inode %"PRDSNu": %"PRDSNu" bytes, "
          "%zu sectors starting at %"PRDSNu"\n",
          idx->sector, idx->data.length,
          (size_t) DIV_ROUND_UP (idx->data.length, DISK_SECTOR_SIZE),
          idx->data.start);
}
