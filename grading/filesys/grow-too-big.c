/* -*- c -*- */

#include <limits.h>
#include <syscall.h>
#include "fslib.h"

const char test_name[] = "grow-sparse";

void
test_main (void) 
{
  char zero = 0;
  int fd;
  
  check (create ("testfile", 0), "create \"testfile\"");
  check ((fd = open ("testfile")) > 1, "open \"testfile\"");
  msg ("seek \"testfile\"");
  seek (fd, UINT_MAX);
  check (write (fd, &zero, 1) > 0, "write \"testfile\"");
  msg ("close \"testfile\"");
  close (fd);
}
