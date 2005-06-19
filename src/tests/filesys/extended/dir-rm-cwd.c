#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (mkdir ("a"), "mkdir \"a\"");
  CHECK (chdir ("a"), "chdir \"a\"");
  msg ("remove \"/a\" (must not crash)");
  remove ("/a");
  msg ("create \"b\" (must not crash)");
  create ("b", 123);
}
