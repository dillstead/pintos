#include <stdio.h>

int
main (void) 
{
  printf ("(pt-write-code) begin\n");
  *(int *) main = 0;
  printf ("(pt-write-code) end\n");
  return 0;
}
