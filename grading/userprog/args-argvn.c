#include <stdio.h>
#include <syscall.h>

int
main (int argc, char *argv[]) 
{
  printf ("(args-argvn) argv[argc] = %p\n", argv[argc]);
  return 0;
}
