#include <stdio.h>
#ifdef PINTOS
#include <syscall.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

unsigned char buf[65536];
size_t histogram[256];

int
main (void) 
{
  int fd;
  unsigned char *p;
  size_t size;
  size_t i;
  
#ifdef PINTOS
  fd = open ("buffer");
#else
  fd = open ("buffer", O_RDWR);
#endif
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
#ifdef PINTOS
  seek (fd, 0);
#else
  lseek (fd, 0, SEEK_SET);
#endif
  write (fd, buf, size);
  close (fd);
  
  return 0x123;
}
