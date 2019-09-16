#ifndef VM_MMAP_H
#define VM_MMAP_H

#include <stddef.h>

/* Per process maximum number of files that can be memory mapped
   at any one time. */
#define MAX_MMAP_FILES 128

struct mmap
{
  /* Address of the start of the file mapping. */
  void *upage;
  /* The file being mapped. */
  struct file *file;
  /* The number of pages mapped for this file. */
  size_t num_pages;
};

int mmap (int fd, void *addr);
void munmap (int md);

#endif /* vm/mmap.h */
