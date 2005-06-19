#include <syscall-nr.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  asm volatile ("mov %%esp, 0xbffffffc; mov [dword ptr %%esp], %0; int 0x30"
                :: "i" (SYS_exit));
  fail ("should have called exit(-1)");
}
