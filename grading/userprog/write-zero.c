#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  int handle, byte_cnt;
  char buf;
  printf ("(write-zero) begin\n");

  handle = open ("sample.txt");
  if (handle < 2)
    printf ("(write-zero) fail: open() returned %d\n", handle);

  buf = 123;
  byte_cnt = write (handle, &buf, 0);
  if (byte_cnt != 0)
    printf ("(write-zero) fail: write() returned %d instead of 0\n", byte_cnt);
  
  printf ("(write-zero) end\n");
  return 0;
}
