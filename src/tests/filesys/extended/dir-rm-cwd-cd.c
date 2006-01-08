/* Tries to remove the current directory.
   This is allowed to succeed or fail.
   If it succeeds, then it must not be possible to chdir back to
   the current directory by name (because it's been deleted).
   If it fails, then it must be possible to chdir back to the
   current directory by name (because it still exists). */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (mkdir ("a"), "mkdir \"a\"");
  CHECK (chdir ("a"), "chdir \"a\"");
  msg ("remove \"/a\" (must not crash)");
  if (remove ("/a"))
    CHECK (!chdir ("/a"),
           "chdir \"/a\" (remove succeeded so this must return false)");
  else
    CHECK (chdir ("/a"), "chdir \"/a\" (remove failed so this must succeed)");
}
