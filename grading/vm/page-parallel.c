#include <syscall.h>

#define CHILD_CNT 3

int
main (void) 
{
  pid_t children[CHILD_CNT];
  int i;

  printf ("(page-parallel) begin\n");
  for (i = 0; i < CHILD_CNT; i++) 
    {
      printf ("(page-parallel) start child %d\n", i);
      children[i] = exec ("child-linear");
      if (children[i] == -1) 
        {
          printf ("(page-parallel) exec() returned pid -1\n", children[i]);
          return 1;
        }
    }

  for (i = 0; i < CHILD_CNT; i++) 
    {
      int code;
      printf ("(page-parallel) join child %d\n", i);
      code = join (children[i]);
      if (code != 0x42)
        printf ("(page-parallel) child %d returned bad exit code\n", i);
    }
  printf ("(page-parallel) end\n");

  return 0;
}
