#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(wait-simple) begin\n");
  printf ("(wait-simple) wait(exec()) = %d\n", wait (exec ("child-simple")));
  printf ("(wait-simple) end\n");
  return 0;
}
