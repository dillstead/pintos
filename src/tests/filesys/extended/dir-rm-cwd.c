/* Tries to remove the current directory.
   This is allowed to succeed or fail.
   Then creates a file in the root to make sure that we haven't
   corrupted any directory state. */

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
