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
  char buf[1024];

  printf ("(mmap-write) begin\n");

  /* Write file via mmap. */
  if (!create ("sample.txt", strlen (sample)))
    {
      printf ("(mmap-write) create() failed\n");
      return 1;
    }
  
  fd = open ("sample.txt");
  if (fd < 0) 
    {
      printf ("(mmap-write) open() failed\n");
      return 1;
    }

  if (!mmap (fd, ACTUAL, strlen (sample)))
    {
      printf ("(mmap-write) mmap() failed\n");
      return 1;
    }
  memcpy (ACTUAL, sample, strlen (sample));
  munmap (ACTUAL, strlen (sample));

  /* Read back via read(). */
  read (fd, buf, strlen (sample));
  if (memcmp (ACTUAL, sample, strlen (sample)))
    {
      printf ("(mmap-write) read of mmap-written file reported bad data\n");
      return 1;
    }
  close (fd);

  /* Done. */
  printf ("(mmap-write) end\n");

  return 0;
}
