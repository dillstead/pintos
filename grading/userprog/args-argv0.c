#include <debug.h>
#include <stdio.h>
#include <syscall.h>

int
main (int argc UNUSED, char *argv[]) 
{
  printf ("(args-argv0) argv[0] = '%s'\n", argv[0]);
  return 0;
}
