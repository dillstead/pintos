#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(create-empty) begin\n");
  create ("", 0);
  printf ("(create-empty) end\n");
  return 0;
}
