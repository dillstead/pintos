#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int
main (int argc UNUSED, char *argv[]) 
{
  if (isdigit (*argv[0]))
    close (atoi (argv[0])); 
  else if (isdigit (*argv[1]))
    close (atoi (argv[1]));
  else 
    {
      printf ("(child-close) fail: bad command-line arguments\n");
      return 1; 
    }
  printf ("(child-close) success\n");
  return 0;
}
