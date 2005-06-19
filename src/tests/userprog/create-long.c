#include <string.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  static char name[512];
  memset (name, 'x', sizeof name);
  name[sizeof name - 1] = '\0';
  
  msg ("create(\"x...\"): %d", create (name, 0));
}
