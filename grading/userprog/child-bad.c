#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(child-bad) begin\n"); 
  asm volatile ("mov %esp, 0x20101234; int 0x30");
  printf ("(child-bad) end\n"); 
  return 0;
}
