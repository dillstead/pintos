#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[]) 
{
  if (!strcmp (argv[0], "childarg") || !strcmp (argv[1], "childarg")) 
    {
      printf ("(child-arg) success\n");
      return 0; 
    }
  else 
    {
      int i;

      printf ("(child-arg) failure\n");
      printf ("(child-arg) argc=%d\n", argc);
      for (i = 0; i <= argc; i++)
        if (argv[i] >= (char *) 0xbffff000 && argv[i] < (char *) 0xc0000000)
          printf ("(child-arg) argv[%d]='%s'\n", i, argv[i]);
        else
          printf ("(child-arg) argv[%d]=%p\n", i, argv[i]);
      return 1;
    }
}
