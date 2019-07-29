#include <debug.h>
#include <bitmap.h>
#include "vm/swap-map.h"
#include "devices/block.h"

#define SECTORS_PER_PAGE 4

/* Free map, one bit per page size sector chunk. */
static struct bitmap *swap_map;  

/* Initializes the swap map. */
void
swap_map_init()
{
  struct block *swap_device;

  swap_device = block_get_role (BLOCK_SWAP);
  swap_map = bitmap_create (block_size (swap_device) / SECTORS_PER_PAGE);
  if (swap_map == NULL)
    PANIC ("bitmap creation failed--swap device is too large");
      
}

/* Allocates a page size chunk of consecutive sectors and stores the first
   into *SECTORP.  Returns true if successful, false if not enough sectors 
   are left. */
bool
swap_map_allocate (block_sector_t *sectorp)
{
  block_sector_t sector = bitmap_scan_and_flip (swap_map, 0, 1, false);
  if (sector != BITMAP_ERROR)
    *sectorp = sector * SECTORS_PER_PAGE;
  return sector != BITMAP_ERROR;  
}

/* Makes a page size chunk of sectors starting at SECTOR available for use. */
void
swap_map_release (block_sector_t sector)
{
  ASSERT (bitmap_all (swap_map, sector, 1));
  bitmap_set_multiple (swap_map, sector, 1, false);  
}
