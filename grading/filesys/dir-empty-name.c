#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-empty-name";

void
test_main (void) 
{
  CHECK (!create ("", 0), "create \"\" (must return false)");
}
