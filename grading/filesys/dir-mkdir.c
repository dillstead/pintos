#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-mkdir";

void
test_main (void) 
{
  CHECK (mkdir ("a"), "mkdir \"a\"");
  CHECK (create ("a/b", 512), "create \"a/b\"");
  CHECK (chdir ("a"), "chdir \"a\"");
  CHECK (open ("b") > 1, "open \"b\"");
}

