#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-empty-name";

void
test_main (void) 
{
  check (!create ("", 0), "create \"\" (must return false)");
}
