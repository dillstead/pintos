#ifndef HEADER_ADDRSPACE_H
#define HEADER_ADDRSPACE_H 1

#include "list.h"

struct vma 
  {
    struct list_elem elem;
    uint32_t start, end;
    void **pages;
  };

struct addrspace 
  {
    uint32_t *page_dir;
    struct list vmas;
  };

void addrspace_load (struct addrspace *, const char *);

#endif /* addrspace.h */
