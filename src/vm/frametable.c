#include <stdio.h>
#include <stdbool.h>
#include <list.h>
#include <hash.h>
#include <debug.h>
#include <string.h>
#include "userprog/pagedir.h"
#include "vm/pageinfo.h"
#include "vm/frametable.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/inode.h"

#define DEBUG_FRAMETABLE 1

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
     from swap_block. */
  bool swapped;
  /* Information about the frame backing the page. */
  struct frame *frame;
  /* Depending on the type this can be information about the backing file, the 
     swap block if the page is swapped out, or the kernel virtual page address
     of content to to initialize the page with. */
  union
  {
    struct file_info file_info;
    block_sector_t swap_block;
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
struct hash read_only_frames;

void pageinfo_print (struct page_info *page_info, bool tab);
static void frame_print (struct frame *frame);
static void frame_init (struct frame *frame);
static void map_page (struct page_info *page_info, struct frame *frame,
                      const void *upage);
static void unmap_page (struct page_info *page_info, struct frame *frame,
                        const void *upage);
static void wait_for_io_done (struct frame **frame);
static struct frame *lookup_read_only_frame (struct page_info *page_info);
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

static const char *
type_string (int type)
{
  if (type & PAGE_TYPE_ZERO)
    return "zero"; 
  else if (type & PAGE_TYPE_KERNEL)
    return "kernel";
  else if (type & PAGE_TYPE_FILE)
    return "file";
  else
    return "unknown";
}

static const char *
writable_string (int writable)
{
  if (writable & WRITABLE_TO_FILE)
    return "file";
  else if (writable & WRITABLE_TO_SWAP)
    return "swap";
  else
    return "no";
}

void
pageinfo_print (struct page_info *page_info, bool tab)
{
#if DEBUG_FRAMETABLE
  if (tab)
    printf ("\t");
  printf ("page info w: %s, t: %s, upg: %p, sw: %u, fr: %p\n",
          writable_string (page_info->writable), type_string (page_info->type),
          page_info->upage, page_info->swapped, page_info->frame);
  if (page_info->type & PAGE_TYPE_KERNEL)
    printf ("\tkpg: %p\n", page_info->data.kpage); 
  else if (page_info->type & PAGE_TYPE_FILE)
    printf ("\toff: %u, sz: %u\n",
            offset (page_info->data.file_info.end_offset),
            size (page_info->data.file_info.end_offset));
  if (page_info->swapped)
    printf ("\tswap: %u\n", page_info->data.swap_block);
#endif
}

static void
frame_print (struct frame *frame)
{
#if DEBUG_FRAMETABLE
  struct list_elem *e;
    
  printf ("frame kpg: %p, lck: %u, io: %u\n", frame->kpage, frame->lock,
          frame->io);
  for (e = list_begin (&frame->page_info_list);
       e != list_end (&frame->page_info_list); e = list_next (e))
    pageinfo_print (list_entry (e, struct page_info, elem), true);
#endif
}

void
frametable_init (void)
{
  lock_init (&frame_lock);
  hash_init (&read_only_frames, frame_hash, frame_less, NULL);
}

/* TODO comment general policy of releasing lock before doing io */
bool
frametable_load_frame (uint32_t *pd, const void *upage)
{
  struct page_info *page_info;
  struct file_info *file_info;
  struct frame *frame;
  const void *kpage;
  off_t bytes_read;
  bool success = false;

#if DEBUG_FRAMETABLE
  printf ("frametable_load_frame\n");
#endif  
  page_info = pagedir_get_info (pd, upage);  
  ASSERT (page_info != NULL);
  lock_acquire (&frame_lock);
  /* It's possible the frame could be in the process of being swapped out.
     If so, wait for the swap to finish before continuing. When 
     wait_for_io_done returns, frame_lock will be held. */
  wait_for_io_done (&page_info->frame);
  ASSERT (page_info->frame == NULL);
  pageinfo_print (page_info, false);
  /* Attempt to satisfy a read only page by looking it up in the 
     cache. */
  if (page_info->type & PAGE_TYPE_FILE
      && page_info->writable == 0)
    {
      frame = lookup_read_only_frame (page_info);
      if (frame != NULL)
        {
          /* If another process is loading the frame in, wait for it to
             finish.  Lock the frame so it won't get swapped out right
             after it's loaded in and before the page is mapped. */
          frame->lock++;
          wait_for_io_done (&frame);
          frame->lock--;
#if DEBUG_FRAMETABLE          
          printf ("mapping existing read only frame\n");
#endif          
          frame_print (frame);
          map_page (page_info, frame, upage);
          success = true;
          goto done;
        }
    }
  /* Allocate and fill a new frame. */
  frame = calloc (1, sizeof *frame);
  if (frame == NULL)
    goto done;
  frame_init (frame);
  /* Map page to frame. */
  map_page (page_info, frame, upage);
  if (page_info->type & PAGE_TYPE_FILE)
    {
      /* Add the read only frame to the cache before the data is read
         from the file to ensure that the next process that tries to
         read it in will wait for the read to complete instead of 
         reading the data into a new frame. */
#if DEBUG_FRAMETABLE      
      printf ("reading from file\n");
#endif      
      if (page_info->writable == 0)
        {
#if DEBUG_FRAMETABLE          
          printf ("adding read only frame\n");
#endif          
          hash_insert (&read_only_frames, &frame->hash_elem);
        }
      file_info = &page_info->data.file_info;
      frame->io = true;
      frame->lock++;
      lock_release (&frame_lock);
      bytes_read = file_read_at (file_info->file, frame->kpage,
                                 size (file_info->end_offset),
                                 offset (file_info->end_offset));
      ASSERT (bytes_read == size (file_info->end_offset));
      if (bytes_read != size (file_info->end_offset))
        goto done;
      lock_acquire (&frame_lock);
      frame->lock--;
      frame->io = false;
      cond_signal (&frame->io_done, &frame_lock);
    }
  else if (page_info->type & PAGE_TYPE_KERNEL)
    {
#if DEBUG_FRAMETABLE      
      printf ("reading from kernel page\n");
#endif      
      kpage = page_info->data.kpage;
      memcpy (frame->kpage, kpage, PGSIZE);
      palloc_free_page ((void *) kpage);
      page_info->data.kpage = NULL;
    }
  success = true;

 done:
  if (!success)
    if (frame != NULL)
      free (frame);
  if (lock_held_by_current_thread (&frame_lock))
    lock_release (&frame_lock);
  return success;
}

void
frametable_unload_frame (uint32_t *pd, const void *upage)
{
    struct page_info *page_info;
    struct file_info *file_info;
    void *kpage;
    struct frame *frame;
    off_t bytes_written;

#if DEBUG_FRAMETABLE    
    printf ("frametable_unload_frame\n");
#endif    
    page_info = pagedir_get_info (pd, upage);
    ASSERT (page_info != NULL);
    /* It's possible the frame could be in the process of being swapped out.
       If so, wait for the swap to finish before continuing. When 
       wait_for_io_done returns, frame_lock will be held. */
    wait_for_io_done (&page_info->frame);
    pageinfo_print (page_info, false);
    if (page_info->frame != NULL)
      {
        frame = page_info->frame;
        unmap_page (page_info, frame, upage);
        /* If there are no more read only pages mapped to this frame
           remove it from the cache. */
        if (list_empty (&frame->page_info_list)
            && page_info->type & PAGE_TYPE_FILE && page_info->writable == 0)
          hash_delete (&read_only_frames, &frame->hash_elem);
        /* At this point the frame has been removed from the shared data
           structures and it's safe to release the lock and, if necessary,
           free the resources associated with the frame. */
        lock_release (&frame_lock);
        if (list_empty (&frame->page_info_list))
          {
            if (page_info->type & PAGE_TYPE_FILE
                && pagedir_is_dirty (page_info->pd, upage))
              {
                ASSERT (page_info->writable != 0);
                file_info = &page_info->data.file_info;
                bytes_written = file_write_at (file_info->file, frame->kpage,
                                               size (file_info->end_offset),
                                               offset (file_info->end_offset));
                ASSERT (bytes_written == size (file_info->end_offset));
              }
            palloc_free_page (frame->kpage);
            free (frame);
          }
      }
    else 
      lock_release (&frame_lock);
    /* Free resources associated with page info. */
    if (page_info->type & PAGE_TYPE_KERNEL)
      {
        kpage = (void *) page_info->data.kpage;
        if (kpage != NULL)
          palloc_free_page (kpage);
      }
    pagedir_set_info (page_info->pd, upage, NULL);
    free (page_info);
}

static void
frame_init (struct frame *frame)
{
  list_init (&frame->page_info_list);
  cond_init (&frame->io_done);
  frame->kpage = palloc_get_page (PAL_USER | PAL_ZERO | PAL_ASSERT);
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
unmap_page (struct page_info *page_info, struct frame *frame, const void *upage)
{
  struct page_info *pi;
  struct list_elem *e;
  
  page_info->frame = NULL;
  for (e = list_begin (&frame->page_info_list);
       e != list_end (&frame->page_info_list); e = list_next (e))
    {
      pi = list_entry (e, struct page_info, elem);
      if (page_info == pi)
        {
          list_remove (e);
          break;
        }
    }
  pagedir_clear_page (page_info->pd, upage);
}

static void
wait_for_io_done (struct frame **frame)
{
  while (*frame != NULL && (*frame)->io)
    cond_wait (&(*frame)->io_done, &frame_lock);
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

