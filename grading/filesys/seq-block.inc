/* -*- c -*- */

#include "fslib.h"

static char buf[TEST_SIZE];

static size_t
return_block_size (void) 
{
  return BLOCK_SIZE;
}

void
test_main (void) 
{
  seq_test ("noodle",
            buf, sizeof buf, sizeof buf,
            return_block_size, NULL);
}
