#include <string.h>
#include <syscall.h>
#include "tests/vm/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

#define ACTUAL ((void *) 0x10000000)

void
test_main (void)
{
  int handle;
  mapid_t map;
  char buf[1024];

  /* Write file via mmap. */
  CHECK (create ("sample.txt", strlen (sample)), "create \"sample.txt\"");
  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  CHECK ((map = mmap (handle, ACTUAL)) != MAP_FAILED, "mmap \"sample.txt\"");
  memcpy (ACTUAL, sample, strlen (sample));
  munmap (map);

  /* Read back via read(). */
  read (handle, buf, strlen (sample));
  CHECK (!memcmp (buf, sample, strlen (sample)),
         "compare read data against written data");
  close (handle);
}
