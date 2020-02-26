#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/buffers.h"
#include "devices/timer.h"

/* Implements a buffer cache.  Buffers are read in from the file system and 
   cached for subsequent use.  Dirty buffers are written back to the file 
   system periodically by a background thread or when they are evicted to make
   space for a new buffer.  Asynchronous read ahead is supported. */

/* Buffer flags. */
/* If set, the buffer is currently in use by a process. */
#define BUF_IN_USE             0x01
/* If set, the buffer contents do not match the disk contents. */
#define BUF_DIRTY              0x02
/* Marks a buffer as meta data (inode) as opposed to just plain data.
   When searching for a buffer to use, data buffers are chosen over 
   meta data buffers.  Inodes should remain in the cache
   for as long as possible for performance reasons. */
#define BUF_META               0x08

#define CACHE_SIZE             64
/* How often the background write back thread wakes up to write dirty
   buffers to disk. */
#define WRITE_BACK_INTERVAL_MS 100

/* Read ahead sectors placed on the read ahead queue. */
struct read_ahead_sector
{
  /* The disk sector to read. */
  block_sector_t sector;
  bool is_meta;
};

static struct buffer buffers[CACHE_SIZE];
static struct lock cache_lock;
static struct list cache;
/* When signaled indicates that a buffer is available. */
static struct condition buffer_available;
/* Read ahead. */
static struct lock read_ahead_lock;
/* Read ahead queue. */
static struct read_ahead_sector read_ahead_sectors[CACHE_SIZE];
static int sectors_head;
static int sectors_tail;
static size_t sectors_size;
/* When signaled indicates a buffer is available for reading ahead. */
static struct condition read_ahead_available;
/* Stops the read ahead thread. */
static bool stop_read_ahead;
/* Used to wait for the read ahead thread to exit. */
static struct semaphore read_ahead_done;

static int cache_accesses;
static int cache_hits;

static struct buffer *get_buffer_to_acquire (block_sector_t sector);
static struct buffer *get_buffer_to_write_back (void);
static void load_buffer (block_sector_t sector, bool is_meta,
                         struct buffer *buffer);
static void flush_all (void);
static void read_ahead (void *aux UNUSED);
static void write_back (void *aux UNUSED);

void
buffers_init (void)
{
  int i;

  list_init (&cache);
  lock_init (&cache_lock);
  cond_init (&buffer_available);
  lock_init (&read_ahead_lock);
  cond_init (&read_ahead_available);
  sema_init (&read_ahead_done, 0);
  stop_read_ahead = false;
  for (i = 0; i < CACHE_SIZE; i++)
    {
      buffers[i].sector = UINT_MAX;
      buffers[i].evicting_sector = UINT_MAX;
      cond_init (&buffers[i].available);
      cond_init (&buffers[i].evicted);
      list_push_back (&cache, &buffers[i].elem);
    }
  thread_create ("read_ahead", PRI_DEFAULT, read_ahead, NULL);
  thread_create ("write_back", PRI_DEFAULT, write_back, NULL);
}

void
buffers_done (void)
{
  lock_acquire (&cache_lock);
  stop_read_ahead = true;
  cond_signal (&read_ahead_available, &cache_lock);
  lock_release (&cache_lock);
  sema_down (&read_ahead_done);
  flush_all ();
  printf ("cache accesses: %d, hits: %d\n", cache_accesses,
          cache_hits);
}

/* Acquires a buffer for reading and writing.  If the buffer is already
   cached, it's returned without any I/O.  If not, the contents of the 
   buffer are read in before returning.  If the buffer is currently in 
   use or being evicted, waits until it's available.  The returned buffer 
   is locked so no other process can acquire it until buffer_release is 
   called.  Acquired buffers should be released as quickly as possible. */
struct buffer *
buffer_acquire (block_sector_t sector, bool is_meta)
{
  struct buffer *buffer;
  bool acquire = false;

  lock_acquire (&cache_lock);
  cache_accesses++;

  while (!acquire)
    {
      buffer = get_buffer_to_acquire (sector);
      if (buffer == NULL)
        cond_wait (&buffer_available, &cache_lock);
      else if (sector == buffer->sector)
        {
          cache_hits++;
          buffer->waiting++;
          while (buffer->flags & BUF_IN_USE)
            cond_wait (&buffer->available, &cache_lock);
          buffer->waiting--;
          buffer->flags |= BUF_IN_USE;
          lock_release (&cache_lock);
          acquire = true;
        }
      else if (sector == buffer->evicting_sector)
        {
          ASSERT (buffer->flags & BUF_IN_USE);
          
          while (sector == buffer->evicting_sector)
            cond_wait (&buffer->evicted, &cache_lock);
        }
      else
        {
          load_buffer (sector, is_meta, buffer);
          acquire = true;
        }
    }
  return buffer;
}

/* Releases a buffer and marks it as dirty if it's been written to.  The
 * buffer is available for reuse if there are no processes waiting on it. */
void
buffer_release (struct buffer *buffer, bool dirty)
{
  ASSERT (buffer->flags & BUF_IN_USE);
  
  lock_acquire (&cache_lock);
  buffer->flags &= ~BUF_IN_USE;
  if (dirty)
    buffer->flags |= BUF_DIRTY;
  if (buffer->waiting > 0)
      cond_signal (&buffer->available, &cache_lock);
  else
    {
      list_remove (&buffer->elem);
      list_push_back (&cache, &buffer->elem);
      cond_signal (&buffer_available, &cache_lock);      
    }
  lock_release (&cache_lock);
}

/* Adds a sector to the read ahead queue.  If the queue is full, does
   nothing. */
void
buffer_read_ahead (block_sector_t sector, bool is_meta)
{
  struct read_ahead_sector ra_sector;
  
  lock_acquire (&read_ahead_lock);
  if (sectors_size < CACHE_SIZE)
    {
      ra_sector.sector = sector;
      ra_sector.is_meta = is_meta;
      read_ahead_sectors[sectors_head++ % CACHE_SIZE] = ra_sector;
      sectors_size++;
      cond_signal (&read_ahead_available, &read_ahead_lock);
    }
  lock_release (&read_ahead_lock);
}

/* Loads a sector into a buffer.  If a sector is already present it's 
   evicted. */
static void
load_buffer (block_sector_t sector, bool is_meta, struct buffer *buffer)
{
  ASSERT (!(buffer->flags & BUF_IN_USE));
  
  buffer->flags |= BUF_IN_USE;
  if (is_meta)
    buffer->flags |= BUF_META;
  else
    buffer->flags &= ~BUF_META;
  if (buffer->flags & BUF_DIRTY)
    {
      buffer->evicting_sector = buffer->sector;
      buffer->sector = sector;
      lock_release (&cache_lock);
      block_write (fs_device, buffer->evicting_sector, buffer->data);
      lock_acquire (&cache_lock);
      buffer->evicting_sector = UINT_MAX;
      buffer->flags &= ~BUF_DIRTY;
      cond_signal (&buffer->evicted, &cache_lock);
    }
  else
    buffer->sector = sector;
  lock_release (&cache_lock);
  ASSERT (buffer->evicting_sector == UINT_MAX);
  block_read (fs_device, buffer->sector, buffer->data);
}

/* Writes dirty buffers until no more are available. Depending on the number
   of dirty buffers, this call can take some time to finish.  For that reason
   it should be done on a background thread or called when shutting down. */
static
void flush_all (void)
{
  struct buffer *buffer;

  lock_acquire (&cache_lock);
  buffer = get_buffer_to_write_back ();
  while (buffer != NULL)
    {
      buffer->waiting++;
      while (buffer->flags & BUF_IN_USE)
        cond_wait (&buffer->available, &cache_lock);
      buffer->waiting--;
      buffer->flags |= BUF_IN_USE;
      lock_release (&cache_lock);
      block_write (fs_device, buffer->sector, buffer->data);
      buffer->flags &= ~BUF_DIRTY;
      buffer_release (buffer, false);
      lock_acquire (&cache_lock);
      buffer = get_buffer_to_write_back ();
    }
  lock_release (&cache_lock);
}

/* Reads in read ahead buffes until the read ahead queue is empty or it's 
   told to stop. */
static void
read_ahead (void *aux UNUSED)
{
  struct read_ahead_sector ra_sector;
  struct buffer *buffer;
  
  while (true)
    {
      lock_acquire (&read_ahead_lock);
      while (sectors_size == 0 && !stop_read_ahead)
        cond_wait (&read_ahead_available, &read_ahead_lock);      
      if (stop_read_ahead)
        break;
      ra_sector = read_ahead_sectors[sectors_tail++ % CACHE_SIZE];
      sectors_size--;
      lock_release (&read_ahead_lock);
      lock_acquire (&cache_lock);
      buffer = get_buffer_to_acquire (ra_sector.sector);
      if (buffer != NULL && ra_sector.sector != buffer->sector
          && ra_sector.sector != buffer->evicting_sector)
        {
          load_buffer (ra_sector.sector, ra_sector.is_meta, buffer);
          buffer_release (buffer, false);
        }
      else
        lock_release (&cache_lock);
    }
    sema_up (&read_ahead_done);
    thread_exit ();
}

static void
write_back (void *aux UNUSED)
{
  while (true)
    {
      flush_all ();
      timer_msleep (WRITE_BACK_INTERVAL_MS);
    }
}

/* Looks for a buffer in the cache.  If a buffer is already in the cache returns
   it immediately.  If not, the least recently used unused buffer is returned 
   with data buffers taking precedence over meta data buffers.  If no suitable 
   buffer can be found, returns NULL. */
static struct buffer *
get_buffer_to_acquire (block_sector_t sector)
{
  struct buffer *buffer;
  struct buffer *meta_buffer = NULL;
  struct buffer *data_buffer = NULL;
  struct list_elem *e;

  for (e = list_begin (&cache); e != list_end (&cache); e = list_next (e))
    {
      buffer = list_entry (e, struct buffer, elem);
      if (sector == buffer->sector || sector == buffer->evicting_sector)
        return buffer;
      else if (buffer->waiting == 0 && !(buffer->flags & BUF_IN_USE))
        {
          if (buffer->flags & BUF_META)
            {
              if (meta_buffer == NULL)
                meta_buffer = buffer;
            }
          else if (data_buffer == NULL)
            data_buffer = buffer;
        }
    }
  return data_buffer != NULL ? data_buffer : meta_buffer;
}

/* Looks for a dirty buffer in the cache.  If no suitable buffer can be found
   returns NULL. */
static struct buffer *
get_buffer_to_write_back (void)
{
  struct buffer *buffer;
  struct list_elem *e;

  for (e = list_begin (&cache); e != list_end (&cache); e = list_next (e))
    {
      buffer = list_entry (e, struct buffer, elem);
      if ((buffer->flags & BUF_DIRTY) && buffer->evicting_sector == UINT_MAX)
        return buffer;
    }
  return NULL;
}
