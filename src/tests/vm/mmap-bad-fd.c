#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (mmap (0x5678, (void *) 0x10000000) == MAP_FAILED,
         "try to mmap invalid fd");
}

