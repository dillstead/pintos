#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-under-file";

void
test_main (void) 
{
  check (create ("abc", 0), "create \"abc\"");
  check (!mkdir ("abc"), "mkdir \"abc\" (must return false)");
}
