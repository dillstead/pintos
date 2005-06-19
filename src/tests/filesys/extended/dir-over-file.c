#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (mkdir ("abc"), "mkdir \"abc\"");
  CHECK (!create ("abc", 0), "create \"abc\" (must return false)");
}
