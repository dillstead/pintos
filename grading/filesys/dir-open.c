#include <syscall.h>
#include "fslib.h"

const char test_name[] = "dir-open";

void
test_main (void) 
{
  int fd;
  
  check (mkdir ("xyzzy"), "mkdir \"xyzzy\"");
  msg ("open \"xyzzy\"");
  fd = open ("xyzzy");
  if (fd == -1) 
    msg ("open returned -1 -- ok");
  else 
    {
      int retval = write (fd, "foobar", 6);
      check (retval == -1, "write \"xyzzy\" (must return -1, actually %d)",
             retval);
    }
}
