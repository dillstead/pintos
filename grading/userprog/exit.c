#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(exit) begin\n");
  exit (57);
  printf ("(exit) fail\n");
  return 0;
}
