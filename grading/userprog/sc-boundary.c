#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>

static void *
mk_boundary_array (void) 
{
  static char dst[8192];
  return dst + (4096 - (uintptr_t) dst % 4096);
}

int
main (void) 
{
  int *p;
  
  printf ("(sc-boundary) begin\n");
  p = mk_boundary_array ();
  p--;
  p[0] = SYS_exit;
  p[1] = 42;
  asm volatile ("mov %%esp, %0; int 0x30" :: "g" (p));
  printf ("(sc-boundary) failed\n");
  return 1;
}
