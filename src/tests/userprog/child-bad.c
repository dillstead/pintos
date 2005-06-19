#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  asm volatile ("mov %esp, 0x20101234; int 0x30");
  fail ("should have exited with -1");
}
