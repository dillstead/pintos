#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(exec-multiple) begin\n");
  join (exec ("child-simple"));
  join (exec ("child-simple"));
  join (exec ("child-simple"));
  join (exec ("child-simple"));
  printf ("(exec-multiple) end\n");
  return 0;
}
