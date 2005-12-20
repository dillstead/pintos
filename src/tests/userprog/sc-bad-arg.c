#include <syscall-nr.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  asm volatile ("movl $0xbffffffc, %%esp; movl %0, (%%esp); int $0x30"
                :: "i" (SYS_exit));
  fail ("should have called exit(-1)");
}
