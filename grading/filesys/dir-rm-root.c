#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-rm-root";

void
test_main (void) 
{
  CHECK (!remove ("/"), "remove \"/\" (must return false)");
  CHECK (create ("/a", 243), "create \"/a\"");
}
