/* -*- c -*- */

#include <syscall.h>
#include "fslib.h"

const char test_name[] = "grow-sparse";
static char buf[76543];

int
main (void) 
{
  char zero = 0;
  int fd;
  
  msg ("begin");
  check (create ("testfile", 0), "create \"testfile\"");
  check ((fd = open ("testfile")) > 1, "open \"testfile\"");
  msg ("seek \"testfile\"");
  seek (fd, sizeof buf - 1);
  check (write (fd, &zero, 1) > 0, "write \"testfile\"");
  msg ("close \"testfile\"");
  close (fd);
  check_file ("testfile", buf, sizeof buf);
  msg ("end");
  return 0;
}
