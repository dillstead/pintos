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

  printf ("(child-mm-wrt) begin\n");

  /* Write file via mmap. */
  if (!create ("sample.txt", strlen (sample)))
    {
      printf ("(child-mm-wrt) create() failed\n");
      return 1;
    }
  
  fd = open ("sample.txt");
  if (fd < 0) 
    {
      printf ("(child-mm-wrt) open() failed\n");
      return 1;
    }

  if (mmap (fd, ACTUAL) == MAP_FAILED)
    {
      printf ("(child-mm-wrt) mmap() failed\n");
      return 1;
    }
  memcpy (ACTUAL, sample, strlen (sample));

  printf ("(child-mm-wrt) end\n");

  return 234;
}

