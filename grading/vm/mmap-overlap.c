#include <stdio.h>
#include <string.h>
#ifdef PINTOS
#include <syscall.h>
#else
#include "posix-compat.h"
#endif
#include "sample.inc"

int
main (void) 
{
  char *start = (char *) 0x10000000;
  size_t i;
  int fd[2];

  printf ("(mmap-overlap) begin\n");

  for (i = 0; i < 2; i++) 
    {
      fd[i] = open ("zeros");
      if (fd[i] < 0) 
        {
          printf ("(mmap-overlap) open() failed\n");
          return 1;
        }
      if (!mmap (fd[i], start, 8192))
        {
          if (i == 1) 
            return 0;
          else
            {
              printf ("(mmap-overlap) mmap() failed\n");
              return 1; 
            }
        }
      start += 4096;
    }

  printf ("(mmap-overlap) fail: mmap of overlapped blocks succeeded\n");

  /* Done. */
  printf ("(mmap-overlap) end\n");

  return 0;
}
