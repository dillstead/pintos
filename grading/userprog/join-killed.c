#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(join-killed) begin\n");
  printf ("(join-killed) join(exec()) = %d\n", join (exec ("child-bad")));
  printf ("(join-killed) end\n");
  return 0;
}
