#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(join-simple) begin\n");
  printf ("(join-simple) join(exec()) = %d\n", join (exec ("child-simple")));
  printf ("(join-simple) end\n");
  return 0;
}
