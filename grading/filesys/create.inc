/* -*- c -*- */

#include <stdio.h>
#include <syscall.h>
#include "fslib.h"

static char buf[TEST_SIZE];

void
test_main (void) 
{
  const char *filename = "blargle";
  CHECK (create (filename, TEST_SIZE), "create \"%s\"", filename);
  check_file (filename, buf, TEST_SIZE);
}
