#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(exec-bad-ptr) begin\n");
  exec ((char *) 0x20101234);
  printf ("(exec-bad-ptr) end\n");
  return 0;
}
