#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-over-file";

void
test_main (void) 
{
  CHECK (mkdir ("abc"), "mkdir \"abc\"");
  CHECK (!create ("abc", 0), "create \"abc\" (must return false)");
}
