#include <stdio.h>
#include <string.h>
#include "../lib/arc4.h"
#include "sample.inc"
#ifdef PINTOS
#include <syscall.h>
#else
#include "posix-compat.h"
#endif

#define ACTUAL ((void *) 0x10000000)

int
main (void) 
{
  int fd;
  mapid_t map;

  printf ("(mmap-close) begin\n");

  fd = open ("sample.txt");
  if (fd < 0) 
    {
      printf ("(mmap-close) open() failed\n");
      return 1;
    }

  map = mmap (fd, ACTUAL);
  if (map == MAP_FAILED)
    {
      printf ("(mmap-close) mmap() failed\n");
      return 1;
    }

  close (fd);

  if (memcmp (ACTUAL, sample, strlen (sample)))
    {
      printf ("(mmap-close) read of mmap'd file reported bad data\n");
      return 1;
    }

  munmap (map);

  /* Done. */
  printf ("(mmap-close) end\n");

  return 0;
}
