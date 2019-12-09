#include "filesys/inode.h"
#ifdef DEBUG_INODE
#include <stdio.h>
#endif
#include <list.h>
#ifdef DEBUG_INODE
#include <string.h>
#endif
#include <debug.h>
#include <round.h>
#include <limits.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "filesys/buffers.h"
#include "threads/malloc.h"
#ifdef DEBUG_INODE
#include "threads/thread.h"
#endif

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    uint32_t unused[125];               /* Not used. */
  };

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk *data;            /* Inode content. */
  };

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns UINT_MAX if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (struct inode *inode, off_t pos) 
{
  ASSERT (inode != NULL);
  block_sector_t sector;
  struct buffer *buffer;

  buffer = buffer_acquire (inode->sector, true);
  inode->data = (struct inode_disk *) buffer->data;
  if (pos < inode->data->length)
    sector = inode->data->start + pos / BLOCK_SECTOR_SIZE;
  else
    sector = UINT_MAX;
  buffer_release (buffer, false);
  return sector;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
#ifdef DEBUG_INODE
  printf ("inode_create enter %s%d, sec: %u, len: %u\n", thread_name (),
          thread_current ()->tid, sector, length);
#endif
  struct inode_disk *disk_inode = NULL;
  struct buffer *buffer;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      if (free_map_allocate (sectors, &disk_inode->start)) 
        {
          buffer = buffer_acquire (sector, true);          
          memcpy (buffer->data, disk_inode, sizeof *disk_inode);
          buffer_release (buffer, true);
          if (sectors > 0) 
            {
              static char zeros[BLOCK_SECTOR_SIZE];
              size_t i;
              
              for (i = 0; i < sectors; i++)
                {
                  buffer = buffer_acquire (disk_inode->start + i, false);
                  memcpy (buffer->data, &zeros, sizeof zeros);
                  buffer_release (buffer, true);
                }
            }
          success = true; 
        } 
      free (disk_inode);
    }

#ifdef DEBUG_INODE
  printf ("inode_create exit %s%d, suc: %d\n", thread_name (),
          thread_current ()->tid, success);
#endif
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

#ifdef DEBUG_INODE
  printf ("inode_open enter %s%d, sec: %u\n", thread_name (),
          thread_current ()->tid, sector);
#endif
  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
#ifdef DEBUG_INODE
  printf ("inode_open exit %s%d, inode: %p\n", thread_name (),
          thread_current ()->tid, inode);
#endif
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    {
#ifdef DEBUG_INODE
      printf ("inode_open exit %s%d, inode: NULL\n", thread_name (),
              thread_current ()->tid);
#endif
      return NULL;
    }

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
#ifdef DEBUG_INODE
  printf ("inode_open exit %s%d, inode: %p\n", thread_name (),
          thread_current ()->tid, inode);
#endif
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
#ifdef DEBUG_INODE
  printf ("inode_reopen %s%d, inode: %p\n", thread_name (),
          thread_current ()->tid, inode);
#endif
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
#ifdef DEBUG_INODE
  printf ("inode_get_inumber %s%d, inode: %u\n", thread_name (),
          thread_current ()->tid, inode->sector);
#endif
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
#ifdef DEBUG_INODE
  printf ("inode_close enter %s%d, inode: %p\n", thread_name (),
          thread_current ()->tid, inode);
#endif
  struct buffer *buffer;
  
  /* Ignore null pointer. */
  if (inode == NULL)
    {
#ifdef DEBUG_INODE
      printf ("inode_close exit %s%d\n", thread_name (),
              thread_current ()->tid);
#endif      
      return;
    }

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          buffer = buffer_acquire (inode->sector, true);
          inode->data = (struct inode_disk *) buffer->data;
          free_map_release (inode->data->start,
                            bytes_to_sectors (inode->data->length)); 
          buffer_release (buffer, false);
          free_map_release (inode->sector, 1);
        }

      free (inode); 
    }
#ifdef DEBUG_INODE
  printf ("inode_close exit %s%d\n", thread_name (),
          thread_current ()->tid);
#endif      
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
#ifdef DEBUG_INODE
  printf ("inode_remove %s%d, inode: %p\n", thread_name (),
          thread_current ()->tid, inode);
#endif
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
#ifdef DEBUG_INODE
  printf ("inode_read_at %s%d enter, inode: %p, sz: %u, off: %u, len: %u\n",
          thread_name (), thread_current ()->tid,
          inode, size, offset, inode_length (inode));

#endif
  struct buffer *cached_buffer;
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  block_sector_t sector_idx;

  if (size <= 0)
    {
#ifdef DEBUG_INODE
      printf ("inode_read_at %s%d exits, bytes: 0\n", thread_name (),
              thread_current ()->tid);
#endif
      return 0;
    }
  
  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      cached_buffer = buffer_acquire (sector_idx, false);
      memcpy (buffer + bytes_read, cached_buffer->data + sector_ofs,
              chunk_size);          
      buffer_release (cached_buffer, false);
          
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  sector_idx = byte_to_sector (inode, offset + BLOCK_SECTOR_SIZE - 1);
  if (sector_idx != UINT_MAX)
    buffer_read_ahead (sector_idx, false);
#ifdef DEBUG_INODE
  printf ("inode_read_at %s%d exits, bytes: %u\n", thread_name (),
          thread_current ()->tid, bytes_read);
#endif
  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
#ifdef DEBUG_INODE
  printf ("inode_write_at %s%d enter, inode: %p, sz: %u, off: %u, len: %u\n",
          thread_name (), thread_current ()->tid,
          inode, size, offset, inode_length (inode));

#endif
  struct buffer *cached_buffer;
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  block_sector_t sector_idx;

  if (inode->deny_write_cnt || size <= 0)
    {
#ifdef DEBUG_INODE
      printf ("inode_write_at %s%d exit, bytes: 0\n", thread_name (),
              thread_current ()->tid);
#endif
    return 0;
    }

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      cached_buffer = buffer_acquire (sector_idx, false);
      memcpy (cached_buffer->data + sector_ofs, buffer + bytes_written,
              chunk_size);
      buffer_release (cached_buffer, true);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  sector_idx = byte_to_sector (inode, offset + BLOCK_SECTOR_SIZE - 1);
  if (sector_idx != UINT_MAX)
    buffer_read_ahead (sector_idx, false);
#ifdef DEBUG_INODE
  printf ("inode_write_at %s%d exit, bytes: %u\n", thread_name (),
          thread_current ()->tid, bytes_written);
#endif
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (struct inode *inode)
{
  struct buffer *buffer;
  off_t length;
  
  buffer = buffer_acquire (inode->sector, true);
  inode->data = (struct inode_disk *) buffer->data;
  length = inode->data->length;
  buffer_release (buffer, false);
  return length;
}

