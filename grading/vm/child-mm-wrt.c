#define ACTUAL ((void *) 0x10000000)

int
main (void) 
{
  int fd;

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
  return 234;
}

