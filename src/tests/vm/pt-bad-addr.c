#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  fail ("bad addr read as %d", *(int *) 0x04000000);
}
