#include "devices/partition.h"
#include <stdio.h>
#include "threads/malloc.h"

struct partition
  {
    struct disk *disk;
    disk_sector_t first_sector;
    disk_sector_t sector_cnt;
  };

struct partition partitions[PARTITION_CNT];

static void scan_partition_table (struct disk *, disk_sector_t);

void
partition_init (void)
{
  int chan, dev;

  for (chan = 0; chan < 2; chan++)
    for (dev = 0; dev < 2; dev++)
      {
        struct disk *disk = disk_get (chan, dev);
        if (disk != NULL)
          scan_partition_table (disk, 0);
      }

}

struct partition *
partition_get (enum partition_type type)
{
  ASSERT (type < PARTITION_CNT);

  return &partitions[type];
}

disk_sector_t
partition_size (struct partition *p)
{
  ASSERT (p != NULL);

  return p->sector_cnt;
}

void
partition_read (struct partition *p, disk_sector_t sector, void *buffer)
{
  ASSERT (p != NULL);
  ASSERT (sector < p->sector_cnt);
  ASSERT (buffer != NULL);

  disk_read (p->disk, p->first_sector + sector, buffer);
}

void
partition_write (struct partition *p, disk_sector_t sector, const void *buffer)
{
  ASSERT (p != NULL);
  ASSERT (sector < p->sector_cnt);
  ASSERT (buffer != NULL);

  disk_write (p->disk, p->first_sector + sector, buffer);
}

#define PARTITION_CNT 4
#define PARTITION_TABLE_SIGNATURE 0xaa55

struct partition_table_entry
  {
    uint8_t bootable;
    uint8_t start_chs[3];
    uint8_t type;
    uint8_t end_chs[3];
    uint32_t first_sector;
    uint32_t sector_cnt;
  };

struct partition_table
  {
    uint8_t boot_loader[446];
    struct partition_table_entry partitions[PARTITION_CNT];
    uint16_t signature;
  };

static void register_partition (struct disk *,
                                const struct partition_table_entry *,
                                enum partition_type);

static void
scan_partition_table (struct disk *disk, disk_sector_t sector)
{
  struct partition_table *pt;

  ASSERT (sizeof *pt == DISK_SECTOR_SIZE);
  pt = xmalloc (sizeof *pt);
  disk_read (disk, sector, pt);

  if (pt->signature == PARTITION_TABLE_SIGNATURE)
    {
      size_t i;

      for (i = 0; i < PARTITION_CNT; i++)
        {
          struct partition_table_entry *pte = &pt->partitions[i];

          switch (pte->type)
            {
            case 0x05: /* DOS extended partition. */
            case 0x0f: /* Windows 95 extented partition. */
            case 0x85: /* Linux extended partition. */
            case 0xc5: /* DRDOS "secured" extended partition. */
              scan_partition_table (disk, pte->first_sector);
              break;

            case 0x20: /* Pintos boot partition. */
              register_partition (disk, pte, PARTITION_BOOT);
              break;

            case 0x21: /* Pintos file system partition. */
              register_partition (disk, pte, PARTITION_FILESYS);
              break;

            case 0x22: /* Pintos scratch partition. */
              register_partition (disk, pte, PARTITION_SCRATCH);
              break;

            case 0x23: /* Pintos swap partition. */
              register_partition (disk, pte, PARTITION_SWAP);
              break;

            default:
              /* We don't know anything about this kind of
                 partition.  Ignore it. */
              break;
            }

        }
    }

  free (pt);
}

static void
register_partition (struct disk *disk, const struct partition_table_entry *pte,
                    enum partition_type type)
{
  static const char *partition_names[] =
    {"boot", "file system", "scratch", "swap"};
  struct partition *p;

  ASSERT (type < PARTITION_CNT);

  printf ("%s: Found %"PRDSNu" sector (", disk_name (disk), pte->sector_cnt);
  print_human_readable_size ((uint64_t) pte->sector_cnt * DISK_SECTOR_SIZE);
  printf (") %s partition starting at sector %"PRDSNu"\n",
          partition_names[pte->type], pte->first_sector);

  p = &partitions[type];
  if (p->disk != NULL)
    PANIC ("Can't handle multiple %s partitions", partition_names[pte->type]);
  p->disk = disk;
  p->first_sector = pte->first_sector;
  p->sector_cnt = pte->sector_cnt;
}
