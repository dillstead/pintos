#ifndef HEADER_ADDRSPACE_H
#define HEADER_ADDRSPACE_H 1

#include <stdint.h>
#include "hash.h"

struct addrspace 
  {
    uint32_t *pagedir;
  };

bool addrspace_load (struct addrspace *, const char *);
void addrspace_destroy (struct addrspace *);

#endif /* addrspace.h */
