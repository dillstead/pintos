#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include "sample.inc"

static char *
mk_boundary_string (const char *src) 
{
  static char dst[8192];
  char *p = dst + (4096 - (uintptr_t) dst % 4096 - strlen (src) / 2);
  strlcpy (p, src, 4096);
  return p;
}

int
main (void) 
{
  int handle;
  int byte_cnt;
  char *sample_p;

  sample_p = mk_boundary_string (sample);

  printf ("(write-boundary) begin\n");

  handle = open ("sample.txt");
  if (handle < 2)
    printf ("(write-boundary) fail: open() returned %d\n", handle);

  byte_cnt = write (handle, sample_p, sizeof sample - 1);
  if (byte_cnt != sizeof sample - 1)
    printf ("(write-boundary) fail: write() returned %d instead of %zu\n",
            byte_cnt, sizeof sample - 1);
  
  printf ("(write-boundary) end\n");
  return 0;
}
