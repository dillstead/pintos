/* -*- c -*- */

#include "fslib.h"

static char buf[TEST_SIZE];

static size_t
return_test_size (void) 
{
  return TEST_SIZE;
}

int
main (void) 
{
  msg ("begin");
  seq_test ("quux", buf, sizeof buf, sizeof buf, 2, return_test_size, NULL);
  msg ("end");
  return 0;
}
