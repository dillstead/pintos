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

/* Number of sector indices stored directly in the inode. */
#define NDIRECT_SECTORS   124
/* Number of indirect (or doubly indirect) indicies stored in a sector. */
#define NINDIRECT_SECTORS 128
#define NDIRECT_BYTES     (NDIRECT_SECTORS * BLOCK_SECTOR_SIZE)
/* The total number of bytes that can be stored in a single doubly indirect
   sector. */
#define NDINDIRECT_BYTES  (NINDIRECT_SECTORS * BLOCK_SECTOR_SIZE);

/* Identifies an inode. */
#define INODE_MAGIC 0x494e

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
{
  block_sector_t start;               /* First data sector. */
  off_t length;                       /* File size in bytes. */
  unsigned short magic;               /* Magic number. */
  unsigned short is_dir;              /* File or directory. */
  /* The first NDIRECT_SECTORS data sector numbers are stored directly in
     the inode. */
  block_sector_t sectors[NDIRECT_SECTORS + 1];
};

static off_t update_length (struct inode *inode, off_t offset);

/* Returns the direct sector index of the byte offset. */
static inline size_t
direct_sector_idx (off_t pos)
{
  return pos / BLOCK_SECTOR_SIZE;
}

/* Returns the indirect sector index of the byte offset. */
static inline size_t
indirect_sector_idx (off_t pos)
{
  return (pos - NDIRECT_BYTES) / NDINDIRECT_BYTES;
}

/* Returns the doubly indirect sector index of the byte offset. */
static inline size_t
dindirect_sector_idx (off_t pos)
{
  return (pos - NDIRECT_BYTES) / BLOCK_SECTOR_SIZE % NINDIRECT_SECTORS;
}

/* In-memory inode. */
struct inode 
{
  struct list_elem elem;              /* Element in inode list. */
  block_sector_t sector;              /* Sector number of disk location. */
  int open_cnt;                       /* Number of openers. */
  bool removed;                       /* True if deleted, false otherwise. */
  int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
  struct lock lock;                   /* Used to protect directory inodes. */
  struct inode_disk *data;            /* Inode content. */
};

/* Returns the block device sector that contains byte offset POS
   within INODE. If there's no block device sector at offset POS
   allocates one. NOTE: This function is written in such a way 
   as to never acquire more than one buffer at the same time 
   (calls to free_map_allocate acquire a buffer) to avoid a 
   potential dealock situation where a cache size number of processes 
   all acquire a buffer at the same time and attempt to acquire a second.*/
static bool
byte_to_sector (struct inode *inode, bool is_dir, off_t pos,
                block_sector_t *psector)
{
#ifdef DEBUG_INODE_MAP
  printf ("byte_to_sector enter %s%d, inode: %p, pos: %u, dir: %d\n",
          thread_name (), thread_current ()->tid, inode, pos, is_dir);
  size_t asector_idx;
#endif
  ASSERT (inode != NULL);
  ASSERT (pos < MAX_FILE_SIZE);
  size_t sector_idx;
  block_sector_t sector;
  block_sector_t next_sector = 0;
  bool allocated = false;
  struct buffer *buffer;
  /* An indirect, doubly indirect or data sector's data. */
  block_sector_t *data;
  bool success = false;

  /* Directories are already locked.  Files must be locked to prevent races
     when two or more processes are allocating and adding blocks at the same
     time. */
  if (!is_dir)
    lock_acquire (&inode->lock);
  sector_idx = direct_sector_idx (pos);
  if (sector_idx >= NDIRECT_SECTORS)
    sector_idx = NDIRECT_SECTORS;
  buffer = buffer_acquire (inode->sector, true);
  inode->data = (struct inode_disk *) buffer->data;
  next_sector = inode->data->sectors[sector_idx];
#ifdef DEBUG_INODE_MAP
  printf ("indirect or data idx: %u, sec: %u\n", sector_idx, next_sector);
#endif  
  if (next_sector == 0)
    {
      buffer_release (buffer, false);
      /* Allocate and add a new indirect or data block. */
      if (!free_map_allocate (1, &next_sector))
        goto done;
      allocated = true;
#ifdef DEBUG_INODE_MAP
      printf ("%s%d allocating indirect or data, idx: %u, sec: %u\n",
              thread_name (), thread_current ()->tid, sector_idx, next_sector);
#endif
      buffer = buffer_acquire (inode->sector, true);
      inode->data = (struct inode_disk *) buffer->data;        
      inode->data->sectors[sector_idx] = next_sector;
      buffer_release (buffer, true);
      buffer = buffer_acquire (next_sector, sector_idx >= NDIRECT_SECTORS);
      data = (block_sector_t *) buffer->data;
      memset (data, 0, BLOCK_SECTOR_SIZE);
    }
  else
    buffer_release (buffer, false);
  if (sector_idx < NDIRECT_SECTORS)
    {
      /* Data block. */
      if (allocated)
        buffer_release (buffer, true);
      *psector = next_sector;
      success = true;
      goto done;
    }
  else
    {
      sector = next_sector;
      /* Indirect block. */
      if (!allocated)
        {
          buffer = buffer_acquire (sector, true);
          data = (block_sector_t *) buffer->data;
        }
      sector_idx = indirect_sector_idx (pos);
#ifdef DEBUG_INODE_MAP
      asector_idx = NDIRECT_SECTORS + 128 * sector_idx;
#endif
      next_sector = data[sector_idx];
#ifdef DEBUG_INODE_MAP
      printf ("doubly indirect idx: %u, sec: %u\n", sector_idx, next_sector);
#endif        
      if (next_sector == 0)
        {
          buffer_release (buffer, false);
          /* Allocate and add a new doubly indirect block. */
          if (!free_map_allocate (1, &next_sector))
            goto done;
#ifdef DEBUG_INODE_MAP
          printf ("%s%d allocating doubly indirect, idx: %u, sec: %u\n",
                  thread_name (), thread_current ()->tid, sector_idx,
                  next_sector);
#endif
          buffer = buffer_acquire (sector, true);
          data = (block_sector_t *) buffer->data;
          data[sector_idx] = next_sector;
          buffer_release (buffer, true);
          buffer = buffer_acquire (next_sector, true);
          data = (block_sector_t *) buffer->data;
          memset (data, 0, BLOCK_SECTOR_SIZE);
        }
      else
        {
          buffer_release (buffer, false);
          buffer = buffer_acquire (next_sector, true);
          data = (block_sector_t *) buffer->data;
        }
      sector = next_sector;
      sector_idx = dindirect_sector_idx (pos);
#ifdef DEBUG_INODE_MAP
      asector_idx += sector_idx;
#endif      
      next_sector = data[sector_idx];
#ifdef DEBUG_INODE_MAP
      printf ("data idx: %u, sec: %u\n", sector_idx, next_sector);
#endif        
      if (next_sector == 0)
        {
          buffer_release (buffer, false);
          if (!free_map_allocate (1, &next_sector))
            goto done;
#ifdef DEBUG_INODE_MAP
          printf ("%s%d allocating data, idx: %u sec: %u\n",
                  thread_name (), thread_current ()->tid, sector_idx,
                  next_sector);
#endif
          buffer = buffer_acquire (sector, true);
          data = (block_sector_t *) buffer->data;          
          data[sector_idx] = next_sector;
          buffer_release (buffer, true);
          buffer = buffer_acquire (next_sector, false);
          data = (block_sector_t *) buffer->data;
          memset (data, 0, BLOCK_SECTOR_SIZE);
          buffer_release (buffer, true);
        }
      else
        buffer_release (buffer, false);
    }
  *psector = next_sector;
  success = true;

 done:
  if (!is_dir)
    lock_release (&inode->lock);
#ifdef DEBUG_INODE_MAP
  printf ("byte_to_sector exit %s%d, suc: %d, eidx: %u, aidx: %u, sec: %u\n",
          thread_name (), thread_current ()->tid, success,
          pos / BLOCK_SECTOR_SIZE, asector_idx, next_sector);
#endif
  return success;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;
static struct lock inodes_lock;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
  lock_init (&inodes_lock);
}

/* Initializes an inode for a file or directory with LENGTH length 
   and writes the new inode to sector SECTOR on the file system device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_dir)
{
#ifdef DEBUG_INODE
  printf ("inode_create enter %s%d, sec: %u, len: %u, dir: %u\n",
          thread_name (), thread_current ()->tid, sector, length,
          is_dir);
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
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->is_dir = is_dir;
      buffer = buffer_acquire (sector, true);
      memcpy (buffer->data, disk_inode, sizeof *disk_inode);
      buffer_release (buffer, true);
      success = true; 
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
  struct inode *inode = NULL;

#ifdef DEBUG_INODE
  printf ("inode_open enter %s%d, sec: %u\n", thread_name (),
          thread_current ()->tid, sector);
#endif
  lock_acquire (&inodes_lock);
  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode->open_cnt++;
#ifdef DEBUG_INODE
          printf ("inode_open exit %s%d, inode: %p\n", thread_name (),
                  thread_current ()->tid, inode);
#endif
          goto done;
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
      goto done;
    }

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  lock_init (&inode->lock);
#ifdef DEBUG_INODE
  printf ("inode_open exit %s%d, inode: %p\n", thread_name (),
          thread_current ()->tid, inode);
#endif
done:
  lock_release (&inodes_lock);
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
  lock_acquire (&inodes_lock);
  if (inode != NULL)
    inode->open_cnt++;
  lock_release (&inodes_lock);
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
   If INODE was also a removed inode, frees its blocks. NOTE: This 
   function is written in such a way a as to never acquire more than 
   one buffer at the same time (calls to free_map_release acquire a 
   buffer) to avoid a potential dealock situation where a cache size 
   number of processes all acquire a buffer at the same time and attempt 
   to acquire a second.*/
void
inode_close (struct inode *inode) 
{
#ifdef DEBUG_INODE
  printf ("inode_close enter %s%d, inode: %p\n", thread_name (),
          thread_current ()->tid, inode);
#endif
  /* A data sector. */
  block_sector_t sector;
  /* An indirect sector. */
  block_sector_t isector;
  /* A doubly indirect sector. */
  block_sector_t disector;
  struct buffer *buffer;
  /* An indirect or doubly indirect sector's data. */
  block_sector_t *data;
  size_t i;
  size_t j;
  
  /* Ignore null pointer. */
  if (inode == NULL)
    {
#ifdef DEBUG_INODE
      printf ("inode_close exit %s%d\n", thread_name (),
              thread_current ()->tid);
#endif      
      return;
    }
  lock_acquire (&inodes_lock);
  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
#ifdef DEBUG_INODE
          printf ("%s%d release resources\n", thread_name (),
                  thread_current ()->tid);
#endif      
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
      lock_release (&inodes_lock);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
#ifdef DEBUG_INODE
          printf ("%s%d free blocks\n", thread_name (),
                  thread_current ()->tid);
#endif      
          /* Free direct data blocks and disk inode. */
          buffer = buffer_acquire (inode->sector, true);
          inode->data = (struct inode_disk *) buffer->data;
          for (i = 0; i < NDIRECT_SECTORS; i++)
            {
              sector = inode->data->sectors[i];
              if (sector != 0)
                {
#ifdef DEBUG_INODE
                  printf ("%s%d free data %u\n", thread_name (),
                          thread_current ()->tid, sector);
#endif      
                  free_map_release (sector, 1);
                }
            }
          isector = inode->data->sectors[NDIRECT_SECTORS];
          buffer_release (buffer, false);
#ifdef DEBUG_INODE
          printf ("%s%d free dinode %u\n", thread_name (),
                  thread_current ()->tid, inode->sector);
#endif      
          free_map_release (inode->sector, 1);
          /* Free indirect, doubly indirect, and data blocks. */
          if (isector != 0)
            {
              for (i = 0; i < NINDIRECT_SECTORS; i++)
                {
                  buffer = buffer_acquire (isector, true);
                  data = (block_sector_t *) buffer->data;
                  disector = data[i];
                  buffer_release (buffer, false);
                  if (disector != 0)
                    {
                      for (j = 0; j < NINDIRECT_SECTORS; j++)
                        {
                          buffer = buffer_acquire (disector, true);
                          data = (block_sector_t *) buffer->data;
                          sector = data[j];
                          buffer_release (buffer, false);
                          if (sector != 0)
                            {
#ifdef DEBUG_INODE
                              printf ("%s%d free data %u\n", thread_name (),
                                      thread_current ()->tid, sector);
#endif      
                              free_map_release (sector, 1);
                            }         
                        }
#ifdef DEBUG_INODE
                      printf ("%s%d free dindirect %u\n", thread_name (),
                              thread_current ()->tid, disector);
#endif                            
                      free_map_release (disector, 1);
                    }
                }
#ifdef DEBUG_INODE
              printf ("%s%d free indirect %u\n", thread_name (),
                      thread_current ()->tid, isector);
#endif                            
              free_map_release (isector, 1);
            }
        }
      free (inode); 
    }
  else
    lock_release (&inodes_lock);
#ifdef DEBUG_INODE
  printf ("inode_close exit %s%d\n", thread_name (),
          thread_current ()->tid);
#endif      
}

void
inode_lock (struct inode *inode )
{
  lock_acquire (&inode->lock);
}

void
inode_unlock (struct inode *inode)
{
    lock_release (&inode->lock);
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
  off_t length;
  bool is_dir;
  off_t new_offset;
  block_sector_t sector;

  if (size <= 0)
    {
#ifdef DEBUG_INODE
      printf ("inode_read_at %s%d exits, bytes: 0\n", thread_name (),
              thread_current ()->tid);
#endif
      return 0;
    }
  length = inode_length (inode);
  is_dir = inode_is_dir (inode);
  if (offset >= length)
    {
#ifdef DEBUG_INODE
      printf ("inode_read_at %s%d exits, offset beyond length, bytes: 0\n",
              thread_name (), thread_current ()->tid);
#endif
      return 0;
    }    
  while (size > 0) 
    {
      if (!byte_to_sector (inode, is_dir, offset, &sector))
        break;
      
      /* Disk sector to read, starting byte offset within sector. */
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = length - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      cached_buffer = buffer_acquire (sector, false);
      memcpy (buffer + bytes_read, cached_buffer->data + sector_ofs,
              chunk_size);          
      buffer_release (cached_buffer, false);
          
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  /* If possible, read ahead the next sector so it's cached for a sequential
     read. */
#ifdef DEBUG_INODE
  //printf ("%s%d: reading ahead\n", thread_name (), thread_current ()->tid);
#endif  
  new_offset = offset + BLOCK_SECTOR_SIZE - 1;
  if (size == 0 && new_offset > offset && new_offset < length
      && byte_to_sector (inode, is_dir, new_offset, &sector))
    {
#ifdef DEBUG_INODE
      printf ("read ahead %s%d, off: %u, new: %u\n", thread_name (),
              thread_current ()->tid, offset / BLOCK_SECTOR_SIZE,
              new_offset / BLOCK_SECTOR_SIZE);
#endif           
      buffer_read_ahead (sector, false);
    }
#ifdef DEBUG_INODE
  printf ("inode_read_at %s%d exits, bytes: %u\n", thread_name (),
          thread_current ()->tid, bytes_read);
#endif
  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs. */
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
  off_t length;
  bool is_dir;
  off_t new_offset;
  block_sector_t sector;

  if (inode->deny_write_cnt || size <= 0)
    {
#ifdef DEBUG_INODE
      printf ("inode_write_at %s%d exit, bytes: 0\n", thread_name (),
              thread_current ()->tid);
#endif
      return 0;
    }
  is_dir = inode_is_dir (inode);
  while (size > 0) 
    {
      if (!byte_to_sector (inode, is_dir, offset, &sector))
        break;

      /* Disk sector to read, starting byte offset within sector. */
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = MAX_FILE_SIZE - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;
      
      cached_buffer = buffer_acquire (sector, false);
#ifdef DEBUG_INODE
      printf ("%s%d copying from %u to %u, sz: %u\n", thread_name (),
              thread_current ()->tid, sector_ofs, bytes_written,
              chunk_size);
#endif      
      memcpy (cached_buffer->data + sector_ofs, buffer + bytes_written,
              chunk_size);
      buffer_release (cached_buffer, true);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  /* If the file was extended, update the length. */
  length = update_length (inode, offset);
  /* If possible, read ahead the next sector so it's cached for a sequential
     write. */
#ifdef DEBUG_INODE
  //printf ("%s%d: reading ahead\n", thread_name (), thread_current ()->tid);
#endif
  new_offset = offset + BLOCK_SECTOR_SIZE - 1;
  if (size == 0 && new_offset > offset && new_offset < length
      && byte_to_sector (inode, is_dir, new_offset, &sector))
    {
#ifdef DEBUG_INODE
      printf ("read ahead %s%d, off: %u, new: %u\n", thread_name (),
              thread_current ()->tid, offset / BLOCK_SECTOR_SIZE,
              new_offset / BLOCK_SECTOR_SIZE);
#endif           
      buffer_read_ahead (sector, false);
    }
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

/* Returns true if the inode is a directory, false if it's a file. */
bool
inode_is_dir (struct inode *inode)
{
  struct buffer *buffer;
  bool is_dir;
  
  buffer = buffer_acquire (inode->sector, true);
  inode->data = (struct inode_disk *) buffer->data;
  is_dir = inode->data->is_dir;
  buffer_release (buffer, false);
  return is_dir;
}

/* Returns true if the inode has been removed, false if not. */
bool
inode_is_removed (struct inode *inode)
{
  return inode->removed;
}

static off_t
update_length (struct inode *inode, off_t offset)
{
  struct buffer *buffer;
  off_t length;
  
  buffer = buffer_acquire (inode->sector, true);
  inode->data = (struct inode_disk *) buffer->data;
  length = inode->data->length;
  if (offset > length)
    {
      length = offset;
      inode->data->length = length;
      buffer_release (buffer, true);
    }
  else
    buffer_release (buffer, false);
  return length;
}


