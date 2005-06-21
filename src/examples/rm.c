/* rm.c

   Removes files specified on command line. */

#include <stdio.h>
#include <syscall.h>

int
main (int argc, char *argv[]) 
{
  int i;
  
  for (i = 1; i < argc; i++)
    if (!remove (argv[i]))
      printf ("%s: remove failed\n", argv[i]);
  return 0;
}
