#include <random.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include "fslib.h"

const char test_name[128] = "child-syn-read";

static char buf[1024];

int
main (int argc, const char *argv[]) 
{
  const char *filename = "data";
  int child_idx;
  int fd;
  size_t i;

  quiet = true;
  
  check (argc == 3, "argc must be 3, actually %d", argc);
  child_idx = atoi (argv[1]);
  check (atoi (argv[2]) == (int) sizeof buf,
         "argv[2] must be %zu, actually %s", sizeof buf, argv[2]);

  random_init (0);
  random_bytes (buf, sizeof buf);

  check ((fd = open (filename)) > 1, "open \"%s\"", filename);
  for (i = 0; i < sizeof buf; i++) 
    {
      char c;
      check (read (fd, &c, 1) > 0, "read \"%s\"", filename);
      check (c != buf[i], "byte %zu differs from expected", i);
    }
  close (fd);

  return child_idx;
}

