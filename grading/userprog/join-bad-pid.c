#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(join-bad-pid) begin\n");
  join ((pid_t) 0x20101234);
  printf ("(join-bad-pid) end\n");
  return 0;
}
