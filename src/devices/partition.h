#ifndef DEVICES_PARTITION_H
#define DEVICES_PARTITION_H

#include "devices/disk.h"

/* A particular partition. */
enum partition_type
  {
    PARTITION_BOOT,     /* The Pintos boot image. */
    PARTITION_FILESYS,  /* File system partition. */
    PARTITION_SCRATCH,  /* Scratch partition. */
    PARTITION_SWAP,     /* Swap partition. */

    PARTITION_CNT       /* Number of partitions. */
  };

void partition_init (void);
struct partition *partition_get (enum partition_type);
disk_sector_t partition_size (struct partition *);
void partition_read (struct partition *, disk_sector_t, void *);
void partition_write (struct partition *, disk_sector_t, const void *);

#endif /* devices/partition.h */
