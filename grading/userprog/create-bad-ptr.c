#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(create-bad-ptr) begin\n");
  create ((char *) 0x20101234, 0);
  printf ("(create-bad-ptr) end\n");
  return 0;
}
