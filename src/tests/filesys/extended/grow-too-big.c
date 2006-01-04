#include <limits.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  const char *filename = "fumble";
  char zero = 0;
  int fd;
  
  CHECK (create (filename, 0), "create \"%s\"", filename);
  CHECK ((fd = open (filename)) > 1, "open \"%s\"", filename);
  msg ("seek \"%s\"", filename);
  seek (fd, UINT_MAX);
  msg ("write \"%s\"", filename);
  write (fd, &zero, 1);
  msg ("close \"%s\"", filename);
  close (fd);
}
