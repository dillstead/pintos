#ifndef HEADER_DISK_H
#define HEADER_DISK_H 1

#include <inttypes.h>
#include <stdint.h>

/* Size of a disk sector in bytes. */
#define DISK_SECTOR_SIZE 512

/* Index of a disk sector within a disk.
   Good enough for disks up to 2 TB. */
typedef uint32_t disk_sector_t;

/* Format specifier for printf(), e.g.:
   printf ("sector=%"PRDSNu"\n", sector); */
#define PRDSNu PRIu32

void disk_init (void);
struct disk *disk_get (int chan_no, int dev_no);
disk_sector_t disk_size (struct disk *);
void disk_read (struct disk *, disk_sector_t, void *);
void disk_write (struct disk *, disk_sector_t, const void *);

#endif /* disk.h */
