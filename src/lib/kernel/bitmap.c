#include "bitmap.h"
#include <debug.h>
#include <limits.h>
#include <round.h>
#include <stdio.h>
#include "threads/malloc.h"
#ifdef FILESYS
#include "filesys/file.h"
#endif

/* Element type.

   This must be an unsigned integer type at least as wide as int.

   Each bit represents one bit in the bitmap.
   If bit 0 in an element represents bit K in the bitmap,
   then bit 1 in the element represents bit K+1 in the bitmap,
   and so on. */
typedef unsigned long elem_type;

/* Number of bits in an element. */
#define ELEM_BITS (sizeof (elem_type) * CHAR_BIT)

/* From the outside, a bitmap is an array of bits.  From the
   inside, it's an array of elem_type (defined above) that
   simulates an array of bits. */
struct bitmap
  {
    size_t bit_cnt;     /* Number of bits. */
    elem_type *bits;    /* Elements that represent bits. */
  };

/* Returns the index of the element that contains the bit
   numbered BIT_IDX. */
static inline size_t
elem_idx (size_t bit_idx) 
{
  return bit_idx / ELEM_BITS;
}

/* Returns an elem_type where only the bit corresponding to
   BIT_IDX is turned on. */
static inline elem_type
bit_mask (size_t bit_idx) 
{
  return (elem_type) 1 << (bit_idx % ELEM_BITS);
}

/* Returns the number of elements required for B's bits. */
static inline size_t
elem_cnt (const struct bitmap *b) 
{
  return DIV_ROUND_UP (b->bit_cnt, ELEM_BITS);
}

/* Returns the number of bytes required by B's elements. */
static inline size_t
byte_cnt (const struct bitmap *b) 
{
  return sizeof (elem_type) * elem_cnt (b);
}

/* Returns a bit mask in which the bits actually used in the last
   element of B's bits are set to 1 and the rest are set to 0. */
static inline elem_type
last_mask (const struct bitmap *b) 
{
  int last_bits = b->bit_cnt % ELEM_BITS;
  return last_bits ? ((elem_type) 1 << last_bits) - 1 : (elem_type) -1;
}

/* Initializes B to be a bitmap of BIT_CNT bits
   and sets all of its bits to false.
   Returns true if success, false if memory allocation
   failed. */
struct bitmap *
bitmap_create (size_t bit_cnt) 
{
  struct bitmap *b = malloc (sizeof *b);
  if (b != NULL)
    {
      b->bit_cnt = bit_cnt;
      b->bits = malloc (byte_cnt (b));
      if (b->bits != NULL || bit_cnt == 0)
        {
          bitmap_set_all (b, false);
          return b;
        }
      free (b);
    }
  return NULL;
}

/* Destroys bitmap B, freeing its storage.
   Not for use on bitmaps created by
   bitmap_create_preallocated(). */
void
bitmap_destroy (struct bitmap *b) 
{
  if (b != NULL) 
    {
      free (b->bits);
      free (b);
    }
}

/* Returns the number of bits in B. */
size_t
bitmap_size (const struct bitmap *b)
{
  return b->bit_cnt;
}

/* Sets the bit numbered IDX in B to VALUE. */
void
bitmap_set (struct bitmap *b, size_t idx, bool value) 
{
  ASSERT (b != NULL);
  ASSERT (idx < b->bit_cnt);
  if (value)
    b->bits[elem_idx (idx)] |= bit_mask (idx);
  else
    b->bits[elem_idx (idx)] &= ~bit_mask (idx);
}

/* Sets all bits in B to VALUE. */
void
bitmap_set_all (struct bitmap *b, bool value) 
{
  ASSERT (b != NULL);

  bitmap_set_multiple (b, 0, bitmap_size (b), value);
}

/* Sets the bits numbered START through END, exclusive, in B to
   VALUE. */
void
bitmap_set_multiple (struct bitmap *b, size_t start, size_t end, bool value) 
{
  size_t idx;
  
  ASSERT (b != NULL);
  ASSERT (start <= end);
  ASSERT (end <= b->bit_cnt);

  for (idx = start; idx < end; idx++)
    bitmap_set (b, idx, value);
}

/* Atomically sets the bit numbered IDX in B to true. */
void
bitmap_mark (struct bitmap *b, size_t idx) 
{
  asm ("orl %1, %0"
       : "=m" (b->bits[elem_idx (idx)])
       : "r" (bit_mask (idx)));
}

/* Atomically sets the bit numbered IDX in B to false. */
void
bitmap_reset (struct bitmap *b, size_t idx) 
{
  asm ("andl %1, %0"
       : "=m" (b->bits[elem_idx (idx)])
       : "r" (~bit_mask (idx)));
}

/* Atomically toggles the bit numbered IDX in B;
   that is, if it is true, makes it false,
   and if it is false, makes it true. */
void
bitmap_flip (struct bitmap *b, size_t idx) 
{
  ASSERT (b != NULL);
  ASSERT (idx < b->bit_cnt);
  asm ("xorl %1, %0"
       : "=m" (b->bits[elem_idx (idx)])
       : "r" (bit_mask (idx)));
}

/* Returns the value of the bit numbered IDX in B. */
bool
bitmap_test (const struct bitmap *b, size_t idx) 
{
  ASSERT (b != NULL);
  ASSERT (idx < b->bit_cnt);
  return (b->bits[elem_idx (idx)] & bit_mask (idx)) != 0;
}

/* Returns true if any bit from START to END, exclusive, is set
   to VALUE. */
static bool
contains (const struct bitmap *b, size_t start, size_t end, bool value) 
{
  size_t idx;
  
  ASSERT (b != NULL);
  ASSERT (start <= end);
  ASSERT (end <= b->bit_cnt);

  for (idx = start; idx < end; idx++)
    if (bitmap_test (b, idx) == value)
      return true;
  return false;
}

/* Finds and returns the starting index of the first group of CNT
   consecutive bits in B at or after START that are all set to
   VALUE.
   If there is no such group, returns BITMAP_ERROR. */
size_t
bitmap_scan (const struct bitmap *b, size_t start, size_t cnt, bool value) 
{
  size_t idx, last;
  
  ASSERT (b != NULL);
  ASSERT (start <= b->bit_cnt);

  if (cnt <= b->bit_cnt)
    for (idx = start, last = b->bit_cnt - cnt; idx <= last; idx++)
      if (!contains (b, idx, idx + cnt, !value))
        return idx;
  return BITMAP_ERROR;
}

/* Finds the first group of CNT consecutive bits in B at or after
   START that are all set to VALUE, flips them all to !VALUE,
   and returns the index of the first bit in the group.
   If there is no such group, returns BITMAP_ERROR.
   Bits are set atomically, but testing bits is not atomic with
   setting them. */
size_t
bitmap_scan_and_flip (struct bitmap *b, size_t start, size_t cnt, bool value)
{
  size_t idx = bitmap_scan (b, start, cnt, value);
  if (idx != BITMAP_ERROR) 
    bitmap_set_multiple (b, idx, idx + cnt, !value);
  return idx;
}

/* Returns the number of bits in B between START and END,
   exclusive, that are set to VALUE. */
size_t
bitmap_count (const struct bitmap *b, size_t start, size_t end, bool value) 
{
  size_t idx, cnt;

  ASSERT (b != NULL);
  ASSERT (start <= end);
  ASSERT (end <= b->bit_cnt);

  cnt = 0;
  for (idx = start; idx < end; idx++)
    cnt += bitmap_test (b, idx) == value;
  return cnt;
}

/* Returns true if any bits in B between START and END,
   exclusive, are set to true, and false otherwise.*/
bool
bitmap_any (const struct bitmap *b, size_t start, size_t end) 
{
  return contains (b, start, end, true);
}

/* Returns true if no bits in B between START and END, exclusive,
   are set to true, and false otherwise.*/
bool
bitmap_none (const struct bitmap *b, size_t start, size_t end) 
{
  return !contains (b, start, end, true);
}

/* Returns true if every bit in B between START and END,
   exclusive, is set to true, and false otherwise. */
bool
bitmap_all (const struct bitmap *b, size_t start, size_t end) 
{
  return !contains (b, start, end, false);
}

#ifdef FILESYS
/* Returns the number of bytes needed to store B in a file. */
size_t
bitmap_file_size (const struct bitmap *b) 
{
  return byte_cnt (b);
}

/* Reads FILE into B, ignoring errors. */
void
bitmap_read (struct bitmap *b, struct file *file) 
{
  if (b->bit_cnt > 0) 
    {
      file_read_at (file, b->bits, byte_cnt (b), 0);
      b->bits[elem_cnt (b) - 1] &= last_mask (b);
    }
}

/* Writes FILE to B, ignoring errors. */
void
bitmap_write (const struct bitmap *b, struct file *file)
{
  file_write_at (file, b->bits, byte_cnt (b), 0);
}
#endif /* FILESYS */

/* Dumps the contents of B to the console as hexadecimal. */
void
bitmap_dump (const struct bitmap *b) 
{
  hex_dump (0, b->bits, byte_cnt (b), false);
}

/* Returns the number of bytes required to accomodate a bitmap
   with BIT_CNT bits. */
size_t
bitmap_needed_bytes (size_t bit_cnt) 
{
  struct bitmap b;
  b.bit_cnt = bit_cnt;
  return byte_cnt (&b) + sizeof b;
}

/* Creates and returns a bitmap with BIT_CNT bits in the
   BLOCK_SIZE bytes of storage preallocated at BLOCK.
   BLOCK_SIZE must be at least bitmap_needed_bytes(BIT_CNT). */
struct bitmap *
bitmap_create_preallocated (size_t bit_cnt, void *block, size_t block_size) 
{
  struct bitmap *b = block;
  
  ASSERT (block_size >= bitmap_needed_bytes (bit_cnt));

  b->bit_cnt = bit_cnt;
  b->bits = (elem_type *) (b + 1);
  bitmap_set_all (b, false);
  return b;
}

