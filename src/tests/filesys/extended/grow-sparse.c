#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

static char buf[76543];

void
test_main (void) 
{
  const char *filename = "testfile";
  char zero = 0;
  int fd;
  
  CHECK (create (filename, 0), "create \"%s\"", filename);
  CHECK ((fd = open (filename)) > 1, "open \"%s\"", filename);
  msg ("seek \"%s\"", filename);
  seek (fd, sizeof buf - 1);
  CHECK (write (fd, &zero, 1) > 0, "write \"%s\"", filename);
  msg ("close \"%s\"", filename);
  close (fd);
  check_file (filename, buf, sizeof buf);
}
