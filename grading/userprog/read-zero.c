#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  int handle, byte_cnt;
  char buf;
  printf ("(read-zero) begin\n");

  handle = open ("sample.txt");
  if (handle < 2)
    printf ("(read-zero) fail: open() returned %d\n", handle);

  buf = 123;
  byte_cnt = read (handle, &buf, 0);
  if (byte_cnt != 0)
    printf ("(read-zero) fail: read() returned %d instead of 0\n", byte_cnt);
  else if (buf != 123)
    printf ("(read-zero) fail: 0-byte read() modified buffer\n");
  
  printf ("(read-zero) end\n");
  return 0;
}
