#ifndef HEADER_FILEHDR_H
#define HEADER_FILEHDR_H 1

#include <stdbool.h>
#include <stddef.h>
#include "off_t.h"
#include "devices/disk.h"

/* Number of direct sector pointers in a file header. */
#define DIRECT_CNT ((DISK_SECTOR_SIZE - sizeof (off_t) * 2)     \
                    / sizeof (disk_sector_t))

/* File header.
   This is both an in-memory and on-disk structure. */
struct filehdr 
  {
    off_t length;                       /* File size in bytes. */
    size_t sector_cnt;                  /* File size in sectors. */
    disk_sector_t sectors[DIRECT_CNT];  /* Sectors allocated for file. */
  };

struct bitmap;
struct filehdr *filehdr_allocate (struct bitmap *, off_t length);
void filehdr_deallocate (struct filehdr *, struct bitmap *);
struct filehdr *filehdr_read (disk_sector_t);
void filehdr_write (const struct filehdr *, disk_sector_t);
void filehdr_destroy (struct filehdr *);
disk_sector_t filehdr_byte_to_sector (const struct filehdr *, off_t);
off_t filehdr_length (const struct filehdr *);
void filehdr_print (const struct filehdr *);

#endif /* filehdr.h */
