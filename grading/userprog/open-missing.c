#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  int handle;
  printf ("(open-missing) begin\n");
  handle = open ("no-such-file");
  if (handle != -1)
    printf ("(open-missing) fail: open() returned %d\n", handle);
  printf ("(open-missing) end\n");
  return 0;
}
