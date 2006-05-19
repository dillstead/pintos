/* Lists the contents of a directory using readdir. */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  int fd;
  char name[READDIR_MAX_LEN + 1];

  CHECK ((fd = open (".")) > 1, "open .");
  CHECK (isdir (fd), "isdir(.)");

  while (readdir (fd, name))
    msg ("readdir: \"%s\"", name);

  msg ("close .");
  close (fd);
}
