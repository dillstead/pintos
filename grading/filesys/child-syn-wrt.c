#include <random.h>
#include <stdlib.h>
#include <syscall.h>
#include "fslib.h"
#include "syn-write.h"

const char test_name[] = "child-syn-wrt";

char buf[BUF_SIZE];

int
main (int argc, char *argv[])
{
  int child_idx;
  int fd;

  quiet = true;
  
  CHECK (argc == 2, "argc must be 2, actually %d", argc);
  child_idx = atoi (argv[1]);

  random_init (0);
  random_bytes (buf, sizeof buf);

  CHECK ((fd = open (filename)) > 1, "open \"%s\"", filename);
  seek (fd, CHUNK_SIZE * child_idx);
  CHECK (write (fd, buf + CHUNK_SIZE * child_idx, CHUNK_SIZE) > 0,
         "write \"%s\"", filename);
  msg ("close \"%s\"", filename);
  close (fd);

  return child_idx;
}
