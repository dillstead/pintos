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
  pid_t child;
  int code;
  int fd;
  char buf[1024];

  printf ("(mmap-exit) begin\n");

  /* Make child write file. */
  printf ("(mmap-exit) run child\n");
  child = exec ("child-mm-wrt");
  if (child == -1) 
    {
      printf ("(mmap-exit) exec() failed\n");
      return 1;
    }
  code = wait (child);
  if (code != 234) 
    {
      printf ("(mmap-exit) wait() returned bad exit code: %d\n", code);
      return 1;
    }
  printf ("(mmap-exit) child finished\n");

  /* Read back via read(). */
  fd = open ("sample.txt");
  if (fd < 0) 
    {
      printf ("(mmap-exit) open() failed\n");
      return 1;
    }

  read (fd, buf, strlen (sample));
  if (memcmp (buf, sample, strlen (sample)))
    {
      printf ("(mmap-exit) read of mmap-written file reported bad data\n");
      return 1;
    }
  close (fd);

  /* Done. */
  printf ("(mmap-exit) end\n");

  return 0;
}
