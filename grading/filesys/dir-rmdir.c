#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-rmdir";

void
test_main (void) 
{
  check (mkdir ("a"), "mkdir \"a\"");
  check (remove ("a"), "rmdir \"a\"");
  check (!chdir ("a"), "chdir \"a\" (must return false)");
}
