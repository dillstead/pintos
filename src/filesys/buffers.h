#ifndef FILESYS_BUFFER_H
#define FILESYS_BUFFER_H

#include <stdint.h>
#include <list.h>
#include "threads/synch.h"
#include "devices/block.h"

struct buffer
{
  /* Status flags.  See buffer.c. */
  uint8_t flags;
  /* The disk sector for the buffer. */
  block_sector_t sector;
  /* The sector that's currently being evicted if this is not UINT_MAX. */
  block_sector_t evicting_sector;
  /* The number of processes waiting on the buffer to become available. */
  unsigned waiting;
  /* If the buffer is in use, wait on available. */
  struct condition available;
  /* If the buffer is being evicted, wait on evicting. */
  struct condition evicted;
  uint8_t data[BLOCK_SECTOR_SIZE];
  struct list_elem elem;
};

void buffers_init (void);
void buffers_done (void);
struct buffer *buffer_acquire (block_sector_t sector, bool is_meta);
void buffer_release (struct buffer *buffer, bool dirty);
void buffer_read_ahead (block_sector_t sector, bool is_meta);

#endif /* filesys/buffer.h */
