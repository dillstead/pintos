#include <stdio.h>
#include <syscall.h>

int
main (void) 
{
  printf ("(sc-bad-sp) begin\n"); 
  asm volatile ("mov %esp, 0x20101234; int 0x30");
  printf ("(sc-bad-sp) end\n"); 
  return 0;
}
