/* -*- c -*- */

#include <syscall.h>
#include "fslib.h"

const char test_name[] = "grow-tell";
static char buf[2134];

static size_t
return_block_size (void) 
{
  return 37;
}

static void
check_tell (int fd, long ofs) 
{
  long pos = tell (fd);
  if (pos != ofs)
    fail ("file position not updated properly: should be %ld, actually %ld",
          ofs, pos);
}

int
main (void) 
{
  msg ("begin");
  seq_test ("foobar", buf, sizeof buf, 0, 1, return_block_size, check_tell);
  msg ("end");
  return 0;
}
