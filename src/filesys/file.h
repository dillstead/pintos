#ifndef HEADER_FILE_H
#define HEADER_FILE_H 1

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "devices/disk.h"
#include "filesys/off_t.h"

struct file 
  {
    struct filehdr *hdr;
    void *bounce;
    off_t pos;
  };

bool file_open (struct file *, disk_sector_t);
void file_close (struct file *);
off_t file_read (struct file *, void *, off_t);
off_t file_read_at (struct file *, void *, off_t size, off_t start);
off_t file_write (struct file *, const void *, off_t);
off_t file_write_at (struct file *, const void *, off_t size, off_t start);
off_t file_length (struct file *);
void file_seek (struct file *, off_t);
off_t file_tell (struct file *);

#endif /* file.h */
