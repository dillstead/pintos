#ifndef HEADER_FILE_H
#define HEADER_FILE_H 1

#include <stdint.h>
#include <stddef.h>

typedef int32_t off_t;

struct file;
void file_close (struct file *);
off_t file_read (struct file *, void *, off_t);
off_t file_write (struct file *, const void *, off_t);
off_t file_length (struct file *);
void file_seek (struct file *, off_t);
off_t file_tell (struct file *);

#endif /* file.h */
