/* Invokes a system call with the stack pointer (%esp) set to a
   bad address.  The process must be terminated with -1 exit
   code. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  asm volatile ("movl $0x20101234, %esp; int $0x30");
  fail ("should have called exit(-1)");
}
