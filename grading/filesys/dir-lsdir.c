#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-lsdir";

void
test_main (void) 
{
  lsdir ();
}
