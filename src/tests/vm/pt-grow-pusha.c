#include <string.h>
#include "tests/arc4.h"
#include "tests/cksum.h"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  asm volatile
    ("mov %%eax, %%esp;"        /* Save a copy of the stack pointer. */
     "and %%esp, 0xfffff000;"   /* Move stack pointer to bottom of page. */
     "pusha;"                   /* Push 32 bytes on stack at once. */
     "mov %%esp, %%eax"         /* Restore copied stack pointer. */
     ::: "eax");                /* Tell GCC we modified eax. */
}
