#include <syscall.h>
#include "tests/main.h"

void
test_main (void) 
{
  exec ((char *) 0x20101234);
}
