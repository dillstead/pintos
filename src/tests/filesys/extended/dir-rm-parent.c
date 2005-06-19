#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (mkdir ("a"), "mkdir \"a\"");
  CHECK (chdir ("a"), "chdir \"a\"");
  CHECK (mkdir ("b"), "mkdir \"b\"");
  CHECK (chdir ("b"), "chdir \"b\"");
  msg ("remove \"/b\" (must not crash)");
  remove ("/b");
  msg ("remove \"/a\" (must not crash)");
  remove ("/a");
}
