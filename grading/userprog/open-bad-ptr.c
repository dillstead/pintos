#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(open-bad-ptr) begin\n");
  open ((char *) 0xc0101234);
  printf ("(open-bad-ptr) end\n");
  return 0;
}
