#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(exec-multiple) begin\n");
  wait (exec ("child-simple"));
  wait (exec ("child-simple"));
  wait (exec ("child-simple"));
  wait (exec ("child-simple"));
  printf ("(exec-multiple) end\n");
  return 0;
}
