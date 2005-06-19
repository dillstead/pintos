#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  char *data = (char *) 0x7f000000;
  int handle;

  CHECK (create ("empty", 0), "create empty file \"empty\"");
  CHECK ((handle = open ("empty")) > 1, "open \"empty\"");

  /* Calling mmap() might succeed or fail.  We don't care. */
  msg ("mmap \"empty\"");
  mmap (handle, data);

  /* Regardless of whether the call worked, *data should cause
     the process to be terminated. */
  fail ("unmapped memory is readable (%d)", *data);
}

