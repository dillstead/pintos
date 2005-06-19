#include <syscall.h>
#include "tests/main.h"

void
test_main (void) 
{
  close (0x20101234);
}
