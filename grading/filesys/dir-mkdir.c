#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-mkdir";

void
test_main (void) 
{
  check (mkdir ("a"), "mkdir \"a\"");
  check (create ("a/b", 512), "create \"a/b\"");
  check (chdir ("a"), "chdir \"a\"");
  check (open ("b") > 1, "open \"b\"");
}

