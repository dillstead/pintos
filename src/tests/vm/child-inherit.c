#include <string.h>
#include "tests/vm/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  memset ((char *) 0x54321000, 0, 4096);
  fail ("child can modify parent's memory mappings");
}

