#ifndef VM_SWAP_MAP_H
#define VM_SWAP_MAP_H

#include <stdbool.h>
#include "devices/block.h"

void swap_map_init (void);
bool swap_map_allocate (block_sector_t *);
void swap_map_release (block_sector_t);

#endif /* vm/swap.h */
