#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include "sample.inc"

char actual[sizeof sample];

int
main (void) 
{
  char child_cmd[128];
  int byte_cnt;
  int handle;

  printf ("(multi-child-fd) begin\n");

  handle = open("sample.txt");
  if (handle < 2)
    printf ("(multi-child-fd) fail: open() returned %d\n", handle);

  snprintf (child_cmd, sizeof child_cmd, "child-close %d", handle);
  
  printf ("(multi-child-fd) wait(exec()) = %d\n", wait (exec (child_cmd)));

  byte_cnt = read (handle, actual, sizeof actual - 1);
  if (byte_cnt != sizeof actual - 1)
    printf ("(multi-child-fd) fail: read() returned %d instead of %d\n",
            byte_cnt, sizeof actual - 1);
  else if (strcmp (sample, actual))
    printf ("(multi-child-fd) fail: expected text differs from actual:\n%s",
            actual);
  
  printf ("(multi-child-fd) end\n");
  return 0;
}
