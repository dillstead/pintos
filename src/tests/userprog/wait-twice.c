#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  pid_t child = exec ("child-simple");
  msg ("wait(exec()) = %d", wait (child));
  msg ("wait(exec()) = %d", wait (child));
}
