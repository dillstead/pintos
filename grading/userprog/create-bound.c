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
  printf ("(create-bound) begin\n");
  printf ("(create-bound) create(): %d\n",
          create (mk_boundary_string ("quux.dat"), 0));
  printf ("(create-bound) end\n");
  return 0;
}
