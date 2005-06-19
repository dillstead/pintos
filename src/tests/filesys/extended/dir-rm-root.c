#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (!remove ("/"), "remove \"/\" (must return false)");
  CHECK (create ("/a", 243), "create \"/a\"");
}
