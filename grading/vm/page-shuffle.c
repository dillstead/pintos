#include <stdbool.h>
#include <stdio.h>
#include "arc4.h"
#include "cksum.h"

#define SIZE (128 * 1024)

static char buf[SIZE];

static struct arc4 *
get_key (void) 
{
  static struct arc4 arc4;
  static bool inited = false;
  if (!inited) 
    {
      arc4_init (&arc4, "quux", 4);
      inited = true; 
    }
  return &arc4;
}

static unsigned long
random_ulong (void) 
{
  static unsigned long x;
  arc4_crypt (get_key (), &x, sizeof x);
  return x;
}

static void
shuffle (void) 
{
  size_t i;

  for (i = 0; i < sizeof buf; i++)
    {
      size_t j = i + random_ulong () % (sizeof buf - i);
      char t = buf[i];
      buf[i] = buf[j];
      buf[j] = t;
    }
}

int
main (void) 
{
  size_t i;

  printf ("(page-shuffle) begin\n");

  /* Initialize. */
  for (i = 0; i < sizeof buf; i++)
    buf[i] = i * 257;
  printf ("(page-shuffle) init: cksum=%lu\n", cksum (buf, sizeof buf));
    
  /* Shuffle repeatedly. */
  for (i = 0; i < 10; i++)
    {
      shuffle ();
      printf ("(page-shuffle) shuffle %zu: cksum=%lu\n",
              i, cksum (buf, sizeof buf));
    }
  
  /* Done. */
  printf ("(page-shuffle) end\n");

  return 0;
}
