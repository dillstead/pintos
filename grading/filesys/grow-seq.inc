/* -*- c -*- */

#include "fslib.h"

static char buf[TEST_SIZE];

static size_t
return_block_size (void) 
{
  return 1234;
}

int
main (void) 
{
  msg ("begin");
  seq_test ("testme", buf, sizeof buf, 0, 3, return_block_size, NULL);
  msg ("end");
  return 0;
}
