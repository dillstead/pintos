#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(exec-missing) begin\n");
  printf ("(exec-missing) exec(\"no-such-file\"): %d\n",
          exec ("no-such-file"));
  printf ("(exec-missing) end\n");
  return 0;
}
