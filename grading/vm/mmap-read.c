#include <stdio.h>
#include <string.h>
#ifdef PINTOS
#include <syscall.h>
#else
#include "posix-compat.h"
#endif
#include "sample.inc"

#define ACTUAL ((void *) 0x10000000)

int
main (void) 
{
  int fd;
  mapid_t map;

  printf ("(mmap-read) begin\n");

  fd = open ("sample.txt");
  if (fd < 0) 
    {
      printf ("(mmap-read) open() failed\n");
      return 1;
    }

  map = mmap (fd, ACTUAL);
  if (map == MAP_FAILED)
    {
      printf ("(mmap-read) mmap() failed\n");
      return 1;
    }

  if (memcmp (ACTUAL, sample, strlen (sample)))
    {
      printf ("(mmap-read) read of mmap'd file reported bad data\n");
      return 1;
    }

  if (!munmap (map))
    {
      printf ("(mmap-read) munmap() failed\n");
      return 1;
    }

  close (fd);

  /* Done. */
  printf ("(mmap-read) end\n");

  return 0;
}
