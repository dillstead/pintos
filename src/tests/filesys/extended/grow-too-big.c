/* Checks that trying to grow a file beyond the capacity of the
   file system behaves reasonably (does not crash). */

#include <limits.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  const char *file_name = "fumble";
  char zero = 0;
  int fd;
  
  CHECK (create (file_name, 0), "create \"%s\"", file_name);
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  msg ("seek \"%s\"", file_name);
  seek (fd, UINT_MAX);
  msg ("write \"%s\"", file_name);
  write (fd, &zero, 1);
  msg ("close \"%s\"", file_name);
  close (fd);
}
