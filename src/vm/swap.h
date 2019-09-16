#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "devices/block.h"

void swap_init(void);
block_sector_t swap_write (void *kpage);
void swap_read (block_sector_t sector, void *kpage);
void swap_release (block_sector_t sector);

#endif /* vm/swap.h */
