#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(wait-killed) begin\n");
  printf ("(wait-killed) wait(exec()) = %d\n", wait (exec ("child-bad")));
  printf ("(wait-killed) end\n");
  return 0;
}
