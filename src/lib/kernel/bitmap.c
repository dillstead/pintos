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

/* Initializes B to be a bitmap of BIT_CNT bits.
   Returns true if successfalse, false if memory allocation
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

/* Destroys bitmap B, freeing its storage. */
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
  size_t i;
  
  ASSERT (b != NULL);

  if (b->bit_cnt > 0)
    {
      for (i = 0; i < elem_cnt (b); i++)
        b->bits[i] = value ? (elem_type) -1 : 0;
      b->bits[elem_cnt (b) - 1] &= last_mask (b); 
    }
}

/* Sets the bit numbered IDX in B to true. */
void
bitmap_mark (struct bitmap *b, size_t idx) 
{
  bitmap_set (b, idx, true);
}

/* Sets the bit numbered IDX in B to false. */
void
bitmap_reset (struct bitmap *b, size_t idx) 
{
  bitmap_set (b, idx, false);
}

/* Toggles the bit numbered IDX in B;
   that is, if it is true, makes it false,
   and if it is false, makes it true. */
void
bitmap_flip (struct bitmap *b, size_t idx) 
{
  ASSERT (b != NULL);
  ASSERT (idx < b->bit_cnt);
  b->bits[elem_idx (idx)] ^= bit_mask (idx);
}

/* Returns the value of the bit numbered IDX in B. */
bool
bitmap_test (const struct bitmap *b, size_t idx) 
{
  ASSERT (b != NULL);
  ASSERT (idx < b->bit_cnt);
  return (b->bits[elem_idx (idx)] & bit_mask (idx)) != 0;
}

/* Returns the smallest index of a bit set to VALUE in B.
   If no bits in B are set to VALUE, returns BITMAP_ERROR. */
size_t
bitmap_scan (const struct bitmap *b, bool value) 
{
  elem_type ignore = value ? 0 : (elem_type) -1;
  size_t idx;
  
  ASSERT (b != NULL);
  for (idx = 0; idx < elem_cnt (b); idx++)
    {
      elem_type e = b->bits[idx];
      if (e != ignore)
        {
          idx *= ELEM_BITS;

          while ((e & 1) != value)
            {
              e >>= 1;
              idx++;
            }

          return idx < b->bit_cnt ? idx : BITMAP_ERROR;
        }
    }
  return BITMAP_ERROR;
}

/* Finds the smallest index of a bit set to false in B,
   sets it to true, and returns the index.
   If no bits in B are set to false, changes no bits and
   returns BITMAP_ERROR. */
size_t
bitmap_find_and_set (struct bitmap *b) 
{
  size_t idx = bitmap_scan (b, false);
  if (idx != BITMAP_ERROR) 
    bitmap_mark (b, idx);
  return idx;
}

/* Finds the smallest index of a bit set to true in B,
   sets it to false, and returns the index.
   If no bits in B are set to true, changes no bits and
   returns BITMAP_ERROR. */
size_t
bitmap_find_and_clear (struct bitmap *b) 
{
  size_t idx = bitmap_scan (b, true);
  if (idx != BITMAP_ERROR) 
    bitmap_reset (b, idx);
  return idx;
}

/* Returns the number of bits in B set to true. */
size_t
bitmap_set_cnt (const struct bitmap *b) 
{
  size_t cnt;
  size_t i;

  ASSERT (b != NULL);
  cnt = 0;
  for (i = 0; i < elem_cnt (b); i++)
    {
      elem_type e = b->bits[i];
      while (e != 0) 
        {
          cnt++;
          e &= e - 1;
        }
    }
  return cnt;
}

/* Returns true if any bits in B are set to true,
   and false otherwise.*/
bool
bitmap_any (const struct bitmap *b) 
{
  size_t i;

  ASSERT (b != NULL);
  for (i = 0; i < elem_cnt (b); i++)
    if (b->bits[i])
      return true;
  return false;
}

/* Returns the number of bits in B set to false. */
bool
bitmap_clear_cnt (const struct bitmap *b) 
{
  return b->bit_cnt - bitmap_set_cnt (b);
}

/* Returns true if no bits in B are set to true,
   and false otherwise.*/
bool
bitmap_none (const struct bitmap *b) 
{
  return !bitmap_any (b); 
}

/* Returns true if every bit in B is set to true,
   and false otherwise. */
bool
bitmap_all (const struct bitmap *b) 
{
  size_t i;

  ASSERT (b != NULL);

  if (b->bit_cnt == 0)
    return true;
  
  for (i = 0; i < elem_cnt (b) - 1; i++)
    if (b->bits[i] != (elem_type) -1)
      return false;
  return b->bits[i] == last_mask (b);
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
