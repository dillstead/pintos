#include <stdio.h>
#include <syscall-nr.h>

int
main (void) 
{
  printf ("(sc-bad-arg) begin\n"); 
  asm volatile ("mov %%esp, 0xbffffffc; mov [dword ptr %%esp], %0; int 0x30"
                :: "i" (SYS_exit));
  printf ("(sc-bad-arg) end\n"); 
  return 0;
}
