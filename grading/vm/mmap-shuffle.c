#include <stdio.h>
#include <string.h>
#ifdef PINTOS
#include <syscall.h>
#else
#include "posix-compat.h"
#endif
#include "arc4.h"
#include "cksum.h"

#define SIZE (63 * 1024)        /* Max file size. */
static char *buf = (char *) 0x10000000;

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

  for (i = 0; i < SIZE; i++)
    {
      size_t j = i + random_ulong () % (SIZE - i);
      char t = buf[i];
      buf[i] = buf[j];
      buf[j] = t;
    }
}

int
main (void) 
{
  size_t i;
  int fd;

  printf ("(mmap-shuffle) begin\n");

  /* Create file, mmap. */
  if (!create ("buffer", SIZE))
    {
      printf ("(mmap-shuffle) create() failed\n");
      return 1;
    }
  
  fd = open ("buffer");
  if (fd < 0) 
    {
      printf ("(mmap-shuffle) open() failed\n");
      return 1;
    }

  if (!mmap (fd, buf, SIZE))
    {
      printf ("(mmap-shuffle) mmap() failed\n");
      return 1;
    }

  /* Initialize. */
  for (i = 0; i < SIZE; i++)
    buf[i] = i * 257;
  printf ("(mmap-shuffle) init: cksum=%lu\n", cksum (buf, SIZE));
    
  /* Shuffle repeatedly. */
  for (i = 0; i < 10; i++)
    {
      shuffle ();
      printf ("(mmap-shuffle) shuffle %zu: cksum=%lu\n",
              i, cksum (buf, SIZE));
    }
  
  /* Done. */
  printf ("(mmap-shuffle) end\n");

  return 0;
}
