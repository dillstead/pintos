#define NDEBUG
#include "bitmap.h"
#include <limits.h>
#include "debug.h"
#include "lib.h"
#include "malloc.h"
#ifdef FILESYS
#include "file.h"
#endif

#define ELEM_BITS (sizeof (elem_type) * CHAR_BIT)
#define ELEM_IDX(BIT_IDX) ((BIT_IDX) / ELEM_BITS)
#define BIT_MASK(BIT_IDX) ((elem_type) 1 << ((BIT_IDX) % ELEM_BITS))

static inline size_t
elem_cnt (const struct bitmap *b) 
{
  return DIV_ROUND_UP (b->bit_cnt, ELEM_BITS);
}

static inline size_t
byte_cnt (const struct bitmap *b) 
{
  return sizeof (elem_type) * elem_cnt (b);
}

bool
bitmap_init (struct bitmap *b, size_t bit_cnt) 
{
  b->bit_cnt = bit_cnt;
  b->bits = malloc (byte_cnt (b));
  if (b->bits == NULL && bit_cnt > 0) 
    return false;

  bitmap_set_all (b, false);
  return true;
}

size_t
bitmap_storage_size (const struct bitmap *b) 
{
  return byte_cnt (b);
}

void
bitmap_destroy (struct bitmap *b) 
{
  ASSERT (b);
  
  free (b->bits);
}

size_t
bitmap_size (const struct bitmap *b)
{
  return b->bit_cnt;
}

void
bitmap_set (struct bitmap *b, size_t idx, bool value) 
{
  ASSERT (b != NULL);
  ASSERT (idx < b->bit_cnt);
  if (value)
    b->bits[ELEM_IDX (idx)] |= BIT_MASK (idx);
  else
    b->bits[ELEM_IDX (idx)] &= ~BIT_MASK (idx);
}

void
bitmap_set_all (struct bitmap *b, bool value) 
{
  size_t i;
  size_t leftover_bits;
  
  ASSERT (b != NULL);

  for (i = 0; i < elem_cnt (b); i++)
    b->bits[i] = value ? (elem_type) -1 : 0;

  leftover_bits = b->bit_cnt % ELEM_BITS;
  if (leftover_bits != 0) 
    b->bits[i - 1] = ((elem_type) 1 << leftover_bits) - 1;
}

void
bitmap_mark (struct bitmap *b, size_t idx) 
{
  bitmap_set (b, idx, true);
}

void
bitmap_reset (struct bitmap *b, size_t idx) 
{
  bitmap_set (b, idx, false);
}

void
bitmap_flip (struct bitmap *b, size_t idx) 
{
  ASSERT (b != NULL);
  ASSERT (idx < b->bit_cnt);
  b->bits[ELEM_IDX (idx)] ^= BIT_MASK (idx);
}

bool
bitmap_test (const struct bitmap *b, size_t idx) 
{
  ASSERT (b != NULL);
  ASSERT (idx < b->bit_cnt);
  return (b->bits[ELEM_IDX (idx)] & BIT_MASK (idx)) != 0;
}

size_t
bitmap_scan (const struct bitmap *b, bool value) 
{
  elem_type ignore;
  size_t idx;
  
  ASSERT (b != NULL);
  ignore = value ? 0 : (elem_type) -1;
  for (idx = 0; idx < elem_cnt (b); idx++)
    {
      elem_type e = b->bits[idx];
      if (e != (elem_type) -1)
        {
          idx *= ELEM_BITS;

          while ((e & 1) == 1)
            {
              e >>= 1;
              idx++;
            }

          return idx;
        }
    }
  return BITMAP_ERROR;
}

size_t
bitmap_find_and_set (struct bitmap *b) 
{
  size_t idx = bitmap_scan (b, false);
  if (idx != BITMAP_ERROR) 
    bitmap_mark (b, idx);
  return idx;
}

size_t
bitmap_find_and_clear (struct bitmap *b) 
{
  size_t idx = bitmap_scan (b, true);
  if (idx != BITMAP_ERROR) 
    bitmap_reset (b, idx);
  return idx;
}

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

bool
bitmap_clear_cnt (const struct bitmap *b) 
{
  return b->bit_cnt - bitmap_set_cnt (b);
}

bool
bitmap_none (const struct bitmap *b) 
{
  return !bitmap_any (b); 
}

bool
bitmap_all (const struct bitmap *b) 
{
  size_t leftover_bits;
  size_t i;

  ASSERT (b != NULL);

  if (b->bit_cnt == 0)
    return true;
  
  for (i = 0; i < elem_cnt (b) - 1; i++)
    if (b->bits[i] != (elem_type) -1)
      return true;

  leftover_bits = b->bit_cnt % ELEM_BITS;
  if (leftover_bits == 0)
    return b->bits[i] == (elem_type) -1;
  else
    return b->bits[i] == ((elem_type) 1 << leftover_bits) - 1;
}

#ifdef FILESYS
void
bitmap_read (struct bitmap *b, struct file *file) 
{
  file_read_at (file, b->bits, byte_cnt (b), 0);
}

void
bitmap_write (const struct bitmap *b, struct file *file)
{
  file_write_at (file, b->bits, byte_cnt (b), 0);
}
#endif /* FILESYS */
