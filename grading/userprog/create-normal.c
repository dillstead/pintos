#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(create-normal) begin\n");
  printf ("(create-normal) create(): %d\n", create ("quux.dat", 0));
  printf ("(create-normal) end\n");
  return 0;
}
