/* -*- c -*- */

#include <limits.h>
#include <syscall.h>
#include "fslib.h"

const char test_name[] = "grow-sparse";

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
  CHECK (write (fd, &zero, 1) > 0, "write \"%s\"", filename);
  msg ("close \"%s\"", filename);
  close (fd);
}
