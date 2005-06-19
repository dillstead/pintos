#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  msg ("create(0x20101234): %d", create ((char *) 0x20101234, 0));
}
