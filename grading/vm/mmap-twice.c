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
  char *actual[2] = {(char *) 0x10000000, (char *) 0x20000000};
  size_t i;
  int fd[2];

  printf ("(mmap-twice) begin\n");

  for (i = 0; i < 2; i++) 
    {
      fd[i] = open ("sample.txt");
      if (fd[i] < 0) 
        {
          printf ("(mmap-twice) open() #%zu failed\n", i);
          return 1;
        }
      if (!mmap (fd[i], actual[i], 8192))
        {
          printf ("(mmap-twice) mmap() #%zu failed\n", i);
          return 1; 
        }
    }

  for (i = 0; i < 2; i++)
    if (memcmp (actual[i], sample, strlen (sample)))
      {
        printf ("(mmap-twice) read of mmap'd file %zu reported bad data\n",
                i);
        return 1;
      }

  printf ("(mmap-twice) end\n");

  return 0;
}
