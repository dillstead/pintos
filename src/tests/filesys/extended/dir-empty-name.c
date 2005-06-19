#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (!create ("", 0), "create \"\" (must return false)");
}
