#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  int handle;
  printf ("(open-empty) begin\n");
  handle = open ("");
  if (handle != -1)
    printf ("(open-empty) fail: open() returned %d\n", handle);
  printf ("(open-empty) end\n");
  return 0;
}
