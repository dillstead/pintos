#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-rm-cwd-cd";

void
test_main (void) 
{
  check (mkdir ("a"), "mkdir \"a\"");
  check (chdir ("a"), "chdir \"a\"");
  msg ("remove \"/a\" (must not crash)");
  if (remove ("/a"))
    check (!chdir ("/a"), "chdir \"/a\" (remove succeeded so this must fail)");
  else
    check (chdir ("/a"), "chdir \"/a\" (remove failed so this must succeed)");
}
