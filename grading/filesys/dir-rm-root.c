#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-rm-root";

void
test_main (void) 
{
  check (!remove ("/"), "remove \"/\" (must return false)");
  check (create ("/a", 243), "create \"/a\"");
}
