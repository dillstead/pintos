#ifndef HEADER_FILEHDR_H
#define HEADER_FILEHDR_H 1

#include <stdbool.h>
#include <stddef.h>
#include "disk.h"
#include "off_t.h"

#define DIRECT_CNT ((DISK_SECTOR_SIZE - sizeof (off_t) * 2)     \
                    / sizeof (disk_sector_no))

struct filehdr 
  {
    off_t length;
    size_t sector_cnt;
    disk_sector_no sectors[DIRECT_CNT];
  };

struct bitmap;
struct filehdr *filehdr_allocate (struct bitmap *, off_t length);
void filehdr_deallocate (struct filehdr *, struct bitmap *);
struct filehdr *filehdr_read (disk_sector_no);
void filehdr_write (const struct filehdr *, disk_sector_no);
void filehdr_destroy (struct filehdr *);
disk_sector_no filehdr_byte_to_sector (const struct filehdr *, off_t);
off_t filehdr_length (const struct filehdr *);
void filehdr_print (const struct filehdr *);

#endif /* filehdr.h */
