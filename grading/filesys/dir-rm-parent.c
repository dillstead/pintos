#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-rm-parent";

void
test_main (void) 
{
  check (mkdir ("a"), "mkdir \"a\"");
  check (chdir ("a"), "chdir \"a\"");
  check (mkdir ("b"), "mkdir \"b\"");
  check (chdir ("b"), "chdir \"b\"");
  msg ("remove \"/b\" (must not crash)");
  remove ("/b");
  msg ("remove \"/a\" (must not crash)");
  remove ("/a");
}
