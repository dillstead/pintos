#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(exec-once) begin\n");
  wait (exec ("child-simple"));
  printf ("(exec-once) end\n");
  return 0;
}
