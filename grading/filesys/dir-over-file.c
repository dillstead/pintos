#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-over-file";

void
test_main (void) 
{
  check (mkdir ("abc"), "mkdir \"abc\"");
  check (!create ("abc", 0), "create \"abc\" (must return false)");
}
