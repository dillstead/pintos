#include <stdbool.h>
#include <debug.h>
#include <bitmap.h>
#include "threads/vaddr.h"
#include "vm/swap.h"

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

static bool swap_map_allocate (block_sector_t *sectorp);
static void swap_map_release (block_sector_t sector);
  
struct block *swap_device;
/* Free map, one bit per page size sector chunk. */
static struct bitmap *swap_map;  

void
swap_init(void)
{
  ASSERT (PGSIZE % BLOCK_SECTOR_SIZE == 0);
  swap_device = block_get_role (BLOCK_SWAP);
  swap_map = bitmap_create (block_size (swap_device) / SECTORS_PER_PAGE);
  if (swap_map == NULL)
    PANIC ("bitmap creation failed--swap device is too large");
}

/* Writes a page to swap. */
block_sector_t
swap_write (void *kpage)
{
  int i;
  block_sector_t sector;
  
  if (!swap_map_allocate (&sector))
    PANIC ("no swap space");

  for (i = 0; i < SECTORS_PER_PAGE; i++, sector++, kpage += BLOCK_SECTOR_SIZE)
    block_write (swap_device, sector, kpage);
  return sector - SECTORS_PER_PAGE;
}

/* Reads a page from swap. */
void
swap_read (block_sector_t sector, void *kpage)
{
  int i;
  
  for (i = 0; i < SECTORS_PER_PAGE; i++, sector++, kpage += BLOCK_SECTOR_SIZE)
    block_read (swap_device, sector, kpage);
  swap_release (sector - SECTORS_PER_PAGE);
}

/* Releases a swap sector so it can be reused. */
void
swap_release (block_sector_t sector)
{
  swap_map_release (sector);
}

/* Allocates a page size chunk of consecutive sectors and stores the first
   into *SECTORP.  Returns true if successful, false if not enough sectors 
   are left. */
static bool
swap_map_allocate (block_sector_t *sectorp)
{
  block_sector_t sector = bitmap_scan_and_flip (swap_map, 0, 1, false);
  if (sector != BITMAP_ERROR)
    *sectorp = sector * SECTORS_PER_PAGE;
  return sector != BITMAP_ERROR;  
}

/* Makes a page size chunk of sectors starting at SECTOR available for use. */
static void
swap_map_release (block_sector_t sector)
{
  ASSERT (bitmap_all (swap_map, sector / SECTORS_PER_PAGE, 1));
  bitmap_set_multiple (swap_map, sector / SECTORS_PER_PAGE, 1, false);  
}
