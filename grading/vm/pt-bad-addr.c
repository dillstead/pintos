#include <stdio.h>

int
main (void) 
{
  printf ("(pt-bad-addr) begin\n");
  printf ("(pt-bad-addr) FAIL: bad addr read as %d\n", *(int *) 0x04000000);
  printf ("(pt-bad-addr) end\n");
  return 0;
}
