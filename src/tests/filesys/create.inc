/* -*- c -*- */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

static char buf[TEST_SIZE];

void
test_main (void) 
{
  const char *filename = "blargle";
  CHECK (create (filename, TEST_SIZE), "create \"%s\"", filename);
  check_file (filename, buf, TEST_SIZE);
}
