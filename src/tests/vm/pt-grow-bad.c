#include <string.h>
#include "tests/arc4.h"
#include "tests/cksum.h"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  /* Read from an address 4,096 bytes below the stack pointer.
     Must kill the program. */
  asm volatile ("mov %eax, [%esp - 4096]");
}
