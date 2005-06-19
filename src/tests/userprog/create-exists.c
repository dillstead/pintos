#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  CHECK (create ("quux.dat", 0), "create quux.dat");
  CHECK (create ("warble.dat", 0), "create warble.dat");
  CHECK (!create ("quux.dat", 0), "try to re-create quux.dat");
  CHECK (create ("baffle.dat", 0), "create baffle.dat");
  CHECK (!create ("warble.dat", 0), "try to re-create quux.dat");
}
