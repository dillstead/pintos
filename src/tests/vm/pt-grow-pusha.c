#include <string.h>
#include "tests/arc4.h"
#include "tests/cksum.h"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  asm volatile
    ("movl %%esp, %%eax;"        /* Save a copy of the stack pointer. */
     "andl $0xfffff000, %%esp;"  /* Move stack pointer to bottom of page. */
     "pushal;"                   /* Push 32 bytes on stack at once. */
     "movl %%eax, %%esp"         /* Restore copied stack pointer. */
     ::: "eax");                 /* Tell GCC we destroyed eax. */
}
