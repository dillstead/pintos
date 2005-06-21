/* cat.c

   Copies one file to another, using mmap. */

#include <stdio.h>
#include <string.h>
#include <syscall.h>

int
main (int argc, char *argv[]) 
{
  int in_fd, out_fd;
  mapid_t in_map, out_map;
  void *in_data = (void *) 0x10000000;
  void *out_data = (void *) 0x20000000;
  int size;

  if (argc != 3) 
    {
      printf ("usage: cp OLD NEW\n");
      return 1;
    }

  /* Open input file. */
  in_fd = open (argv[1]);
  if (in_fd < 0) 
    {
      printf ("%s: open failed\n", argv[1]);
      return 1;
    }
  size = filesize (in_fd);

  /* Create and open output file. */
  if (!create (argv[2], size)) 
    {
      printf ("%s: create failed\n", argv[2]);
      return 1;
    }
  out_fd = open (argv[2]);
  if (out_fd < 0) 
    {
      printf ("%s: open failed\n", argv[2]);
      return 1;
    }

  /* Map files. */
  in_map = mmap (in_fd, in_data);
  if (in_map == MAP_FAILED) 
    {
      printf ("%s: mmap failed\n", argv[1]);
      return 1;
    }
  out_map = mmap (out_fd, out_data);
  if (out_map == MAP_FAILED)
    {
      printf ("%s: mmap failed\n", argv[2]);
      return 1;
    }

  /* Copy files. */
  memcpy (out_data, in_data, size);

  /* Unmap files (optional). */
  munmap (in_map);
  munmap (out_map);

  return 0;
}
