/* -*- c -*- */

#include <random.h>
#include "fslib.h"

static char buf[TEST_SIZE];

static size_t
return_random (void) 
{
  return random_ulong () % 1031 + 1;
}

int
main (void) 
{
  msg ("begin");
  random_init (-1);
  seq_test ("nibble", buf, sizeof buf, sizeof buf, 5, return_random, NULL);
  msg ("end");
  return 0;
}
