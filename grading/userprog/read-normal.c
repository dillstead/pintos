#include <stdio.h>
#include <string.h>
#include <syscall.h>

char expected[] = {
  "Amazing Electronic Fact: If you scuffed your feet long enough without\n"
  "touching anything, you would build up so many electrons that your\n"
  "finger would explode!  But this is nothing to worry about unless you\n"
  "have carpeting.\n" 
};

char actual[sizeof expected];

int
main (void) 
{
  int handle, byte_cnt;
  printf ("(read-normal) begin\n");

  handle = open ("sample.txt");
  if (handle < 2)
    printf ("(read-normal) fail: open() returned %d\n", handle);

  byte_cnt = read (handle, actual, sizeof actual - 1);
  if (byte_cnt != sizeof actual - 1)
    printf ("(read-normal) fail: read() returned %d instead of %d\n",
            byte_cnt, sizeof actual - 1);
  else if (strcmp (expected, actual))
    printf ("(read-normal) fail: expected text differs from actual:\n%s",
            actual);
  
  printf ("(read-normal) end\n");
  return 0;
}
