#include <stdio.h>
#ifdef PINTOS
#include <syscall.h>
#else
#include "posix-compat.h"
#endif

unsigned char buf[65536];
size_t histogram[256];

int
main (int argc __attribute__ ((unused)), char *argv[]) 
{
  int fd;
  unsigned char *p;
  size_t size;
  size_t i;

  fd = open (argv[1]);
  if (fd < 0) 
    {
      printf ("(child-sort) open() failed\n");
      return 1;
    }

  size = read (fd, buf, sizeof buf);
  for (i = 0; i < size; i++)
    histogram[buf[i]]++;
  p = buf;
  for (i = 0; i < sizeof histogram / sizeof *histogram; i++) 
    {
      size_t j = histogram[i];
      while (j-- > 0)
        *p++ = i;
    }
  seek (fd, 0);
  write (fd, buf, size);
  close (fd);
  
  return 123;
}
