/* ls.c
  
   Lists the current directory. */

#include <syscall.h>

int
main (void) 
{
  lsdir ();
  return 0;
}
