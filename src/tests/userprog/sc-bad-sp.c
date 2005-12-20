#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  asm volatile ("movl $0x20101234, %esp; int $0x30");
  fail ("should have called exit(-1)");
}
