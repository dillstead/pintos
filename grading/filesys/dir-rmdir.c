#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-rmdir";

void
test_main (void) 
{
  CHECK (mkdir ("a"), "mkdir \"a\"");
  CHECK (remove ("a"), "rmdir \"a\"");
  CHECK (!chdir ("a"), "chdir \"a\" (must return false)");
}
