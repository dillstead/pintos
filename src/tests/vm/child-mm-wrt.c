#include <string.h>
#include <syscall.h>
#include "tests/vm/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

#define ACTUAL ((void *) 0x10000000)

void
test_main (void)
{
  int handle;

  CHECK (create ("sample.txt", sizeof sample), "create \"sample.txt\"");
  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  CHECK (mmap (handle, ACTUAL) != MAP_FAILED, "mmap \"sample.txt\"");
  memcpy (ACTUAL, sample, sizeof sample);
}

