/* -*- c -*- */

#include "fslib.h"

static char buf[TEST_SIZE];

static size_t
return_block_size (void) 
{
  return BLOCK_SIZE;
}

int
main (void) 
{
  msg ("begin");
  seq_test ("noodle", buf, sizeof buf, sizeof buf, 4,
            return_block_size, NULL);
  msg ("end");
  return 0;
}
