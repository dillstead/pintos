#include <debug.h>
#include <stdio.h>
#include <syscall.h>

int
main (int argc, char *argv[] UNUSED) 
{
  printf ("(args-argc) argc=%d\n", argc);
  return 0;
}
