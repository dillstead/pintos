/* -*- c -*- */

#include <syscall.h>
#include "fslib.h"

const char test_name[] = "grow-file-size";
static char buf[2134];

static size_t
return_block_size (void) 
{
  return 37;
}

static void
check_file_size (int fd, long ofs) 
{
  long size = filesize (fd);
  if (size != ofs)
    fail ("filesize not updated properly: should be %ld, actually %ld",
          ofs, size);
}

void
test_main (void) 
{
  seq_test ("testfile",
            buf, sizeof buf, 0,
            return_block_size, check_file_size);
}
