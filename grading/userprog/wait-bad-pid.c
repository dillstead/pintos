#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(wait-bad-pid) begin\n");
  wait ((pid_t) 0x20101234);
  printf ("(wait-bad-pid) end\n");
  return 0;
}
