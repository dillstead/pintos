#include <random.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include "fslib.h"
#include "syn-read.h"

const char test_name[128] = "child-syn-read";

static char buf[BUF_SIZE];

int
main (int argc, const char *argv[]) 
{
  int child_idx;
  int fd;
  size_t i;

  quiet = true;
  
  CHECK (argc == 2, "argc must be 2, actually %d", argc);
  child_idx = atoi (argv[1]);

  random_init (0);
  random_bytes (buf, sizeof buf);

  CHECK ((fd = open (filename)) > 1, "open \"%s\"", filename);
  for (i = 0; i < sizeof buf; i++) 
    {
      char c;
      CHECK (read (fd, &c, 1) > 0, "read \"%s\"", filename);
      compare_bytes (&c, buf + i, 1, i, filename);
    }
  close (fd);

  return child_idx;
}

