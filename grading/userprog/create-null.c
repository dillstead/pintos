#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(create-null) begin\n");
  create (NULL, 0);
  printf ("(create-null) end\n");
  return 0;
}
