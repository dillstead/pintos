/* -*- c -*- */

#include <stdio.h>
#include <syscall.h>
#include "fslib.h"

static char buf[TEST_SIZE];

int
main (void) 
{
  msg ("begin");
  check (create ("testfile", TEST_SIZE), "create testfile");
  check_file ("testfile", buf, TEST_SIZE);
  msg ("end");
  return 0;
}
