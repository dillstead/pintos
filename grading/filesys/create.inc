/* -*- c -*- */

#include <stdio.h>
#include <syscall.h>
#include "fslib.h"

static char buf[TEST_SIZE];

void
test_main (void) 
{
  check (create ("testfile", TEST_SIZE), "create testfile");
  check_file ("testfile", buf, TEST_SIZE);
}
