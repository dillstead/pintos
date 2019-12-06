#include <stdio.h>
#include <stdbool.h>
#include <list.h>
#include <hash.h>
#include <debug.h>
#include <string.h>
#include "userprog/pagedir.h"
#include "vm/pageinfo.h"
#include "vm/frametable.h"
#include "vm/swap.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "filesys/filesys.h"
/* Additional file information, only relevant if the page is backed by a 
   file. */
struct file_info
{
  struct file *file;
  /* The file offset of the end of the mapped region.  It's used to
     calculate the start offset and the size of the region.
  */
  off_t end_offset;
};

static inline off_t offset (off_t end_offset)
{
  return end_offset > 0  ? (end_offset - 1) & ~PGMASK : 0;
}

static inline off_t size (off_t end_offset)
{
  return end_offset - offset (end_offset);
}

/* Additional information associated with user pages. */
struct page_info
{
  uint8_t type;
  uint8_t writable;
  /* The page directory that is mapping the page. */
  uint32_t *pd;
  /* The user virtual page address corresponding to the page. */
  const void *upage;
  /* If true the page is swapped and its contents can be read back
     from swap_sector. */
  bool swapped;
  /* Information about the frame backing the page. */
  struct frame *frame;
  /* Depending on the type this can be information about the backing file, the 
     swap block if the page is swapped out, or the kernel virtual page address
     of content to to initialize the page with. */
  union
  {
    struct file_info file_info;
    block_sector_t swap_sector;
    const void *kpage;
  } data;
  /* List element for the associated frame's page_info_list. */
  struct list_elem elem;
};

/* Information associated with each frame. */
struct frame
{
  /* The kernal virtual page address corresponding to the physical frame. */
  void *kpage;
  /* List of information about each page that is mapped to this frame. */
  struct list page_info_list;
  /* If greater than zero the frame is locked and will not be evicted. */
  unsigned short lock;
  /* If true, data is being read to or written from this frame. */
  bool io;
  struct condition io_done;
  /* Hash element for read_only_frames. */
  struct hash_elem hash_elem;
  struct list_elem list_elem;
};

/* Lock used for manipullating internal data structures. */
static struct lock frame_lock;
/* Cache of read-only file frames indexed by inode and offset. */
static struct hash read_only_frames;
/* List of frames that are potentially available for for eviction.
   This is treated as a circular list with clock hand pointing 
   to the beginning of the list.  Frames are always added to the
   end of the list. */
static struct list frame_list;
/* The hand of the clock for the clock page replacement algorithm
   which is used for choosing a frame to evict. The clock hand 
   points to the next frame to examine. */
static struct list_elem *clock_hand;

static void frame_init (struct frame *frame);
static struct frame *allocate_frame (void);
static bool load_frame (uint32_t *pd, const void *upage, bool write,
                        bool keep_locked);
static void map_page (struct page_info *page_info, struct frame *frame,
                      const void *upage);
static void wait_for_io_done (struct frame **frame);
static struct frame *lookup_read_only_frame (struct page_info *page_info);
static void *evict_frame (void);
static void *get_frame_to_evict (void);
static unsigned frame_hash (const struct hash_elem *e, void *aux UNUSED);
static bool frame_less (const struct hash_elem *a, const struct hash_elem *b,
                        void *aux UNUSED);

struct page_info *
pageinfo_create (void)
{
  struct page_info *page_info;
  
  page_info = calloc (1, sizeof *page_info);
  return page_info;
}

void
pageinfo_destroy (struct page_info *page_info)
{
  free (page_info);
}

void
pageinfo_set_upage (struct page_info *page_info, const void *upage)
{
  page_info->upage = upage;
}

void
pageinfo_set_type (struct page_info *page_info, int type)
{
  page_info->type = type;
}

void
pageinfo_set_writable (struct page_info *page_info, int writable)
{
  page_info->writable = writable;
}

void
pageinfo_set_pagedir (struct page_info *page_info, uint32_t *pd)
{
  page_info->pd = pd;
}

void
pageinfo_set_fileinfo (struct page_info *page_info, struct file *file,
                       off_t end_offset)
{
  page_info->data.file_info.file = file;
  page_info->data.file_info.end_offset = end_offset;
}

void
pageinfo_set_kpage (struct page_info *page_info, const void *kpage)
{
  page_info->data.kpage = kpage;
}

void
frametable_init (void)
{
  lock_init (&frame_lock);
  list_init (&frame_list);
  clock_hand = list_end (&frame_list);
  hash_init (&read_only_frames, frame_hash, frame_less, NULL);
}

/* Reads data into a frame from the appropriate place and maps the
   user virtual page UPAGE to it.  If WRITE is true, the page will be
   mapped as read/write. */
bool
frametable_load_frame(uint32_t *pd, const void *upage, bool write)
{
  return load_frame (pd, upage, write, false);
}

/* Unmaps the frame mapped by UPAGE, writes out any modified data to the 
   appropriate place, and frees all resources associated with the 
   the frame and the page info. */
void
frametable_unload_frame (uint32_t *pd, const void *upage)
{
  struct page_info *page_info, *p;
  struct file_info *file_info;
  void *kpage;
  struct frame *frame;
  off_t bytes_written;
  struct list_elem *e;

  ASSERT (is_user_vaddr (upage));
  page_info = pagedir_get_info (pd, upage);
  if (page_info == NULL)
    return;
  lock_acquire (&frame_lock);
  /* It's possible the frame could be in the process of being evicted.
     If so, wait for eviction to finish before continuing. When 
     wait_for_io_done returns, frame_lock will be held. */
  wait_for_io_done (&page_info->frame);
  if (page_info->frame != NULL)
    {
      frame = page_info->frame;
      page_info->frame = NULL;
      if (list_size (&frame->page_info_list) > 1)
        {
          for (e = list_begin (&frame->page_info_list);
               e != list_end (&frame->page_info_list); e = list_next (e))
            {
              p = list_entry (e, struct page_info, elem);
              if (page_info == p)
                {
                  list_remove (e);
                  break;
                }
            }
        }
      else
        {
          ASSERT (list_entry (list_begin (&frame->page_info_list),
                              struct page_info, elem) == page_info);
          /* Don't remove the page info from the frame's list until
             the frame is removed from the cache because in order to
             lookup the frame for removal it needs to have a page info
             in its list. */
          if (page_info->type & PAGE_TYPE_FILE && page_info->writable == 0)
            hash_delete (&read_only_frames, &frame->hash_elem);
          if (clock_hand == &frame->list_elem)
            clock_hand = list_next (clock_hand);
          list_remove (&page_info->elem);
          list_remove (&frame->list_elem);
        }
      pagedir_clear_page (page_info->pd, upage);
      /* At this point the frame has been removed from the shared data
         structures and it's safe to release the lock and, if necessary,
         free the resources associated with the frame. */
      lock_release (&frame_lock);
      if (list_empty (&frame->page_info_list))
        {
          if (page_info->writable & WRITABLE_TO_FILE
              && pagedir_is_dirty (page_info->pd, upage))
            {
              ASSERT (page_info->writable != 0);
              file_info = &page_info->data.file_info;
              bytes_written = process_file_write_at (file_info->file,
                                                     frame->kpage,
                                                     size (file_info->end_offset),
                                                     offset (file_info->end_offset));
              ASSERT (bytes_written == size (file_info->end_offset));
            }
          palloc_free_page (frame->kpage);
          ASSERT (frame->lock == 0);
          free (frame);
        }
    }
  else 
    lock_release (&frame_lock);
  /* Free resources associated with page info. */
  if (page_info->swapped)
    {
      swap_release (page_info->data.swap_sector);
      page_info->swapped = false;
    }
  else if (page_info->type & PAGE_TYPE_KERNEL)
    {
      /* If it's a kernel page, it must not have been loaded in
         because if it was it would be a zero page. */
      kpage = (void *) page_info->data.kpage;
      ASSERT (kpage != NULL);
      palloc_free_page (kpage);
      page_info->data.kpage = NULL;
    }
  pagedir_set_info (page_info->pd, upage, NULL);
  free (page_info);
}

/* Identical to frametale_load_frame with the exception that,
   upon return, the frame is locked to prevent it from being evicted. */
bool
frametable_lock_frame(uint32_t *pd, const void *upage, bool write)
{
  return load_frame (pd, upage, write, true);
}

/* Unlocks a frame that was locked with frametable_lock_frame.  NOTE:
   the frame is not unloaded, only unlocked. */
void
frametable_unlock_frame(uint32_t *pd, const void *upage)
{
  struct page_info *page_info;
  
  ASSERT (is_user_vaddr (upage));
  page_info = pagedir_get_info (pd, upage);
  if (page_info == NULL)
    return;
  ASSERT (page_info->frame != NULL);
  lock_acquire (&frame_lock);
  page_info->frame->lock--;
  lock_release (&frame_lock);
}

static bool
load_frame (uint32_t *pd, const void *upage, bool write, bool keep_locked)
{
  struct page_info *page_info;
  struct file_info *file_info;
  struct frame *frame = NULL;
  void *kpage;
  off_t bytes_read;
  bool success = false;

  /* Only holds the frame lock when modifying shared data structures.  Releases
     the lock when doing a I/O operations so other processes can load frames
     that don't require I/O without having to wait. */
  ASSERT (is_user_vaddr (upage));
  page_info = pagedir_get_info (pd, upage);
  if (page_info == NULL || (write && page_info->writable == 0))
    return false;
  lock_acquire (&frame_lock);
  /* It's possible the frame could be in the process of being evicted.
     If so, wait for eviction to finish before continuing. When 
     wait_for_io_done returns, frame_lock will be held and frame will be
     NULL. */
  wait_for_io_done (&page_info->frame);
  ASSERT (page_info->frame == NULL || keep_locked);
  if (page_info->frame != NULL)
    {
      if (keep_locked)
        page_info->frame->lock++;
      lock_release (&frame_lock);
      return true;
    }
  /* Attempt to satisfy a read only page by looking it up in the 
     cache. */
  if (page_info->type & PAGE_TYPE_FILE
      && page_info->writable == 0)
    {
      frame = lookup_read_only_frame (page_info);
      if (frame != NULL)
        {
          /* Make sure to map the page before releasing the lock.  If not,
             it's possible that frame could be freed if the final process 
             that maps the frame exits. */
          map_page (page_info, frame, upage);
          /* If another process is loading the frame in, wait for it to
             finish.  Lock the frame so it won't get evicted right
             after it's loaded in and before the page is mapped. */
          frame->lock++;
          wait_for_io_done (&frame);
          frame->lock--;
          success = true;
        }
    }
  /* Fill a new frame. */
  if (frame == NULL)
    {
      frame = allocate_frame ();
      if (frame != NULL)
        {
          /* Map page to frame and read the data in. */
          map_page (page_info, frame, upage);
          if (page_info->swapped || page_info->type & PAGE_TYPE_FILE)
            {
              frame->io = true;
              frame->lock++;
              if (page_info->swapped)
                {
                  lock_release (&frame_lock);
                  swap_read (page_info->data.swap_sector, frame->kpage);
                  page_info->swapped = false;
                }
              else
                {
                  if (page_info->writable == 0)
                    {
                      /* Add the read only frame to the cache before the data is read
                         from the file to ensure that the next process that tries to
                         read it in will wait for the read to complete instead of 
                         reading the same data into a new frame. */
                      hash_insert (&read_only_frames, &frame->hash_elem);
                    }
                  file_info = &page_info->data.file_info;
                  lock_release (&frame_lock);
                  bytes_read = process_file_read_at (file_info->file,
                                                     frame->kpage,
                                                     size (file_info->end_offset),
                                                     offset (file_info->end_offset));
                  ASSERT (bytes_read == size (file_info->end_offset));
                }
              lock_acquire (&frame_lock);
              frame->lock--;
              frame->io = false;
              cond_broadcast (&frame->io_done, &frame_lock);
            }
          else if (page_info->type & PAGE_TYPE_KERNEL)
            {
              kpage = (void *) page_info->data.kpage;
              ASSERT (kpage != NULL);
              memcpy (frame->kpage, kpage, PGSIZE);
              palloc_free_page (kpage);
              page_info->data.kpage = NULL;
              /* Change to a zero page now that the data has been copied in. */
              page_info->type = PAGE_TYPE_ZERO;
            }
          /* else zero page */
          success = true;
        }
    }
  if (success && keep_locked)
    frame->lock++;
  lock_release (&frame_lock);
  return success;
}

static void
frame_init (struct frame *frame)
{
  list_init (&frame->page_info_list);
  cond_init (&frame->io_done);
}

static struct frame *
allocate_frame (void)
{
  struct frame *frame;
  void *kpage;
  
  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL)
    {
      frame = calloc (1, sizeof *frame);
      if (frame != NULL)
        {
          frame_init (frame);
          frame->kpage = kpage;
          /* Add the frame to the end of the list so it becomes eligible 
             for eviction. */
          if (!list_empty (&frame_list))
            list_insert (clock_hand, &frame->list_elem);
          else
            {
              list_push_front (&frame_list, &frame->list_elem);
              clock_hand = list_begin (&frame_list);
            }
        }
      else
        palloc_free_page (kpage);
    }
  else
    frame = evict_frame ();
  return frame;
}

static void
map_page (struct page_info *page_info, struct frame *frame, const void *upage)
{
  page_info->frame = frame;
  list_push_back (&frame->page_info_list, &page_info->elem);
  pagedir_set_page (page_info->pd, upage, frame->kpage,
                    page_info->writable != 0);
  pagedir_set_dirty (page_info->pd, upage, false);
  pagedir_set_accessed (page_info->pd, upage, true);
}

static void
wait_for_io_done (struct frame **frame)
{
  while (*frame != NULL && (*frame)->io)
    cond_wait (&(*frame)->io_done, &frame_lock);
}

/* Evicts and returns a free frame. */
static void *
evict_frame (void)
{
  struct frame *frame;
  struct page_info *page_info;
  struct file_info *file_info;
  off_t bytes_written;
  block_sector_t swap_sector;
  struct list_elem *e;
  bool dirty = false;

  frame = get_frame_to_evict();
  for (e = list_begin (&frame->page_info_list);
       e != list_end (&frame->page_info_list); e = list_next (e))
    {
      page_info = list_entry (e, struct page_info, elem);
      dirty = dirty || pagedir_is_dirty (page_info->pd, page_info->upage);
      /* Make sure to cause page faults before the data is written out or
         else it's possible for a process to be writing to memory as the
         data is being written or swapped. This could result in lost data. */
      pagedir_clear_page (page_info->pd, page_info->upage);
      /* Make sure to set the frame to NULL in page_info after swapping or
         writing the data.  Both load and unload_frame need the frame to wait
         until evicition is finished.  If not, it's possible for the file 
         to be closed or the swap sector to be released while the frame is 
         still being evicted. */
    }
  /* If a frame is writable to swap, it doesn't matter whether or not the
     frame is dirty it must be written to swap.  There is no other place aside
     from swap to read the data back into a frame. */
  if (dirty || page_info->writable & WRITABLE_TO_SWAP)
    {
      ASSERT (page_info->writable != 0);
      frame->io = true;
      frame->lock++;
      if (page_info->writable & WRITABLE_TO_FILE)
        {
          file_info = &page_info->data.file_info;
          lock_release (&frame_lock);
          bytes_written = process_file_write_at (file_info->file,
                                                 frame->kpage,
                                                 size (file_info->end_offset),
                                                 offset (file_info->end_offset));
          ASSERT (bytes_written == size (file_info->end_offset));          
        }
      else
        {
          lock_release (&frame_lock);
          swap_sector = swap_write (frame->kpage);
        }
      lock_acquire (&frame_lock);
      frame->lock--;
      frame->io = false;
      cond_broadcast (&frame->io_done, &frame_lock);
    }
  else if (page_info->type & PAGE_TYPE_FILE && page_info->writable == 0)
    {
      ASSERT (hash_find (&read_only_frames, &frame->hash_elem) != NULL);
      hash_delete (&read_only_frames, &frame->hash_elem);
    }
  for (e = list_begin (&frame->page_info_list);
       e != list_end (&frame->page_info_list); )
    {
      page_info = list_entry (list_front (&frame->page_info_list),
                              struct page_info, elem);
      page_info->frame = NULL;
      if (page_info->writable & WRITABLE_TO_SWAP)
        {
          page_info->swapped = true;
          page_info->data.swap_sector = swap_sector;
        }
      e = list_remove (e);
    }
  memset (frame->kpage, 0, PGSIZE);
  return frame;
}

/* Implementation of the clock page replacement algorithm. A list of frames
   is maintained for eviction.  The "clock hand" points to the next frame to 
   examine.  A frame is eligible for eviction if the access bit is set and it's
   not locked.  If the page is not eligible the access bit is cleared and the
   next frame is examined.  In both cases, the clock hand is moved forward. */ 
static void *
get_frame_to_evict (void)
{
  struct frame *frame;
  struct frame *start;
  struct frame *found = NULL;
  struct page_info *page_info;
  struct list_elem *e;
  bool accessed;

  ASSERT (!list_empty (&frame_list));
  start = list_entry (clock_hand, struct frame, list_elem);
  frame = start;
  do
    {
      accessed = false;
      ASSERT (!list_empty (&frame->page_info_list));
      for (e = list_begin (&frame->page_info_list);
           e != list_end (&frame->page_info_list); e = list_next (e))
        {
          page_info = list_entry (e, struct page_info, elem);
          accessed = accessed || pagedir_is_accessed (page_info->pd,
                                                      page_info->upage);
          pagedir_set_accessed (page_info->pd, page_info->upage, false);

        }
      if (!accessed && frame->lock == 0)
        found = frame;
      clock_hand = list_next (clock_hand);
      if (clock_hand == list_end (&frame_list))
        clock_hand = list_begin (&frame_list);
      frame = list_entry (clock_hand, struct frame, list_elem);
    } while (!found && frame != start);
  if (found == NULL)
    {
      /* Iterated through the entire list and ended up back at the start. */
      ASSERT (frame == start);
      if (frame->lock > 0)
        PANIC ("no frame available for eviction");
      found = frame;
      clock_hand = list_next (clock_hand);
      if (clock_hand == list_end (&frame_list))
        clock_hand = list_begin (&frame_list);
    }

  return found;
}

static struct frame *
lookup_read_only_frame (struct page_info *page_info)
{
  struct frame frame;
  struct hash_elem *e;

  list_init (&frame.page_info_list);
  list_push_back (&frame.page_info_list, &page_info->elem);
  e = hash_find (&read_only_frames, &frame.hash_elem);
  return e != NULL ? hash_entry (e, struct frame, hash_elem) : NULL;
}

static unsigned
frame_hash (const struct hash_elem *e, void *aux UNUSED)
{
  struct frame *frame = hash_entry (e, struct frame, hash_elem);
  struct page_info *page_info;
  block_sector_t sector;

  ASSERT (!list_empty (&frame->page_info_list));
  page_info = list_entry (list_front (&frame->page_info_list),
                          struct page_info, elem);
  ASSERT (page_info->type & PAGE_TYPE_FILE && page_info->writable == 0);
  sector = inode_get_inumber (file_get_inode (page_info->data.file_info.file));
  return hash_bytes (&sector, sizeof sector)
    ^ hash_bytes (&page_info->data.file_info.end_offset,
                  sizeof page_info->data.file_info.end_offset);
}

static bool
frame_less (const struct hash_elem *a_, const struct hash_elem *b_,
            void *aux UNUSED)
{
  struct frame *frame_a = hash_entry (a_, struct frame, hash_elem);
  struct frame *frame_b = hash_entry (b_, struct frame, hash_elem);
  struct page_info *page_info_a, *page_info_b;
  block_sector_t sector_a, sector_b;
  struct inode *inode_a, *inode_b;

  ASSERT (!list_empty (&frame_a->page_info_list));
  ASSERT (!list_empty (&frame_b->page_info_list));
  page_info_a = list_entry (list_front (&frame_a->page_info_list),
                            struct page_info, elem);
  page_info_b = list_entry (list_front (&frame_b->page_info_list),
                            struct page_info, elem);
  inode_a = file_get_inode (page_info_a->data.file_info.file);
  inode_b = file_get_inode (page_info_b->data.file_info.file);
  sector_a = inode_get_inumber (inode_a);
  sector_b = inode_get_inumber (inode_b);
  if (sector_a < sector_b)
    return true;
  else if (sector_a > sector_b)
    return false;
  else
    if (page_info_a->data.file_info.end_offset
        < page_info_b->data.file_info.end_offset)
      return true;
    else
      return false;
}
