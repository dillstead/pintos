#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>

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

  printf ("(open-boundary) begin\n");
  handle = open (mk_boundary_string ("sample.txt"));
  if (handle < 2)
    printf ("(open-boundary) fail: open() returned %d\n", handle);
  printf ("(open-boundary) end\n");
  return 0;
}
