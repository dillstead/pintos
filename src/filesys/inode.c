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
    disk_sector_t first_sector;         /* Starting sector. */
    uint32_t unused[126];               /* Unused padding. */
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

  /* Allocate inode. */
  idx = alloc_inode (sector);
  if (idx == NULL)
    return NULL;

  /* Allocate disk sectors. */
  sector_cnt = DIV_ROUND_UP (length, DISK_SECTOR_SIZE);
  idx->data.length = length;
  idx->data.first_sector = bitmap_scan_and_flip (b, 0, sector_cnt, false);
  if (idx->data.first_sector == BITMAP_ERROR)
    return NULL;

  /* Zero out the file contents. */
  if (sector_cnt > 0) 
    {
      static const char zero_sector[DISK_SECTOR_SIZE];
      disk_sector_t i;
      
      for (i = 0; i < sector_cnt; i++)
        disk_write (filesys_disk, idx->data.first_sector + i, zero_sector);
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

          free_map = bitmap_create (disk_size (filesys_disk));
          if (free_map != NULL)
            {
              disk_sector_t start, end;
              
              bitmap_read (free_map, free_map_file);

              /* Reset inode sector bit. */
              bitmap_reset (free_map, idx->sector);

              /* Reset inode data sector bits. */
              start = idx->data.first_sector;
              end = start + DIV_ROUND_UP (idx->data.length, DISK_SECTOR_SIZE);
              bitmap_set_multiple (free_map, start, end, false);

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
  ASSERT (idx != NULL);

  if (pos < idx->data.length)
    return idx->data.first_sector + pos / DISK_SECTOR_SIZE;
  else
    return (disk_sector_t) -1;
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
  printf ("Inode %"PRDSNu": %"PRDSNu" bytes, "
          "%zu sectors starting at %"PRDSNu"\n",
          idx->sector, idx->data.length,
          (size_t) DIV_ROUND_UP (idx->data.length, DISK_SECTOR_SIZE),
          idx->data.first_sector);
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
