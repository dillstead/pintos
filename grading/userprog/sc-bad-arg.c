#include <stdio.h>
#include <syscall-nr.h>

int
main (void) 
{
  printf ("(sc-bad-arg) begin\n"); 
  asm volatile ("mov $0xbffffffc, %%esp; movl $%0, (%%esp); int $0x30"
                :
                : "i" (SYS_exit));
  printf ("(sc-bad-arg) end\n"); 
  return 0;
}
