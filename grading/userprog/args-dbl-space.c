#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[]) 
{
  int i;

  for (i = 0; i < argc; i++)
    {
      int j;

      for (j = i; j < i + 2; j++)
        if (argv[j] == NULL)
          goto error;
      if (!strcmp (argv[i], "two")
          && !strcmp (argv[i + 1], "args"))
        {
          printf ("(args-dbl-space) success\n");
          return 0;
        }
    error:;
    }
  
  printf ("(args-dbl-space) failure\n");
  printf ("(args-dbl-space) argc=%d\n", argc);
  for (i = 0; i <= argc; i++)
    if (argv[i] >= (char *) 0xbffff000 && argv[i] < (char *) 0xc0000000)
      printf ("(args-dbl-space) argv[%d]='%s'\n", i, argv[i]);
    else
      printf ("(args-dbl-space) argv[%d]=%p\n", i, argv[i]);
  return 1;
}
