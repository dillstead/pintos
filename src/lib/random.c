#include "random.h"
#include <stdint.h>

/* RC4-based pseudo-random state. */
static uint8_t s[256];
static uint8_t s_i, s_j;

static inline void
swap_byte (uint8_t *a, uint8_t *b) 
{
  uint8_t t = *a;
  *a = *b;
  *b = t;
}

static uint8_t
key_byte (int idx) 
{
  return idx ^ 0xff;
}

void
random_init (void)
{
  int i;
  uint8_t j;

  for (i = 0; i < 256; i++) 
    s[i] = i;
  for (i = j = 0; i < 256; i++) 
    {
      j += s[i] + key_byte (i);
      swap_byte (s + i, s + j);
    }

  s_i = s_j = 0;
}

void
random_bytes (void *buf_, size_t size) 
{
  uint8_t *buf;

  for (buf = buf_; size-- > 0; buf++)
    {
      uint8_t s_k;
      
      s_i++;
      s_j += s[s_i];
      swap_byte (s + s_i, s + s_j);

      s_k = s[s_i] + s[s_j];
      *buf = s[s_k];
    }
}

/* Returns a pseudo-random unsigned long. */
unsigned long
random_ulong (void) 
{
  unsigned long ul;
  random_bytes (&ul, sizeof ul);
  return ul;
}
