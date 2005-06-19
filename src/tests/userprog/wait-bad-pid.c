#include <syscall.h>
#include "tests/main.h"

void
test_main (void) 
{
  wait ((pid_t) 0x0c020301);
}
