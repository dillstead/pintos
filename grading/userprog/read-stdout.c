#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  char buf;
  printf ("(read-stdout) begin\n");
  read (1, &buf, 1);
  printf ("(read-stdout) end\n");
  return 0;
}
