#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[]) 
{
  if (!strcmp (argv[0], "onearg") || !strcmp (argv[1], "onearg")) 
    {
      printf ("(args-single) success\n");
      return 0; 
    }
  else 
    {
      int i;

      printf ("(args-single) failure\n");
      printf ("(args-single) argc=%d\n", argc);
      for (i = 0; i <= argc; i++)
        if (argv[i] >= (char *) 0xbffff000 && argv[i] < (char *) 0xc0000000)
          printf ("(args-single) argv[%d]='%s'\n", i, argv[i]);
        else
          printf ("(args-single) argv[%d]=%p\n", i, argv[i]);
      return 1;
    }
}
