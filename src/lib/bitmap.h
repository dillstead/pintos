#ifndef HEADER_BITMAP_H
#define HEADER_BITMAP_H 1

#include <stdbool.h>
#include <stddef.h>

/* Bitmap abstract data type. */

/* Element type.

   This must be an unsigned integer type at least as wide as int.

   Each bit represents one bit in the bitmap.
   If bit 0 in an element represents bit K in the bitmap,
   then bit 1 in the element represents bit K+1 in the bitmap,
   and so on. */
typedef unsigned long elem_type;

/* From the outside, a bitmap is an array of bits.  From the
   inside, it's an array of elem_type (defined above) that
   simulates an array of bits. */
struct bitmap
  {
    size_t bit_cnt;     /* Number of bits. */
    elem_type *bits;    /* Elements that represent bits. */
  };

bool bitmap_init (struct bitmap *, size_t bit_cnt);
void bitmap_destroy (struct bitmap *);

size_t bitmap_size (const struct bitmap *);

void bitmap_set (struct bitmap *, size_t idx, bool);
void bitmap_set_all (struct bitmap *, bool);

void bitmap_mark (struct bitmap *, size_t idx);
void bitmap_reset (struct bitmap *, size_t idx);
void bitmap_flip (struct bitmap *, size_t idx);

bool bitmap_test (const struct bitmap *, size_t idx);

#define BITMAP_ERROR ((size_t) -1)
size_t bitmap_scan (const struct bitmap *, bool);
size_t bitmap_find_and_set (struct bitmap *);
size_t bitmap_find_and_clear (struct bitmap *);

size_t bitmap_set_cnt (const struct bitmap *);
bool bitmap_clear_cnt (const struct bitmap *);

bool bitmap_any (const struct bitmap *);
bool bitmap_none (const struct bitmap *);
bool bitmap_all (const struct bitmap *);

#ifdef FILESYS
struct file;
size_t bitmap_file_size (const struct bitmap *);
void bitmap_read (struct bitmap *, struct file *);
void bitmap_write (const struct bitmap *, struct file *);
#endif

void bitmap_dump (const struct bitmap *);

#endif /* bitmap.h */
