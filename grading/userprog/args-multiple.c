#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[]) 
{
  int i;

  for (i = 0; i < argc - 2; i++)
    {
      int j;

      for (j = i; j < i + 4; j++)
        if (argv[j] == NULL)
          goto error;
      if (!strcmp (argv[i], "some")
          && !strcmp (argv[i + 1], "arguments")
          && !strcmp (argv[i + 2], "for")
          && !strcmp (argv[i + 3], "you!"))
        {
          printf ("(args-multiple) success\n");
          return 0;
        }
    error:;
    }
  
  printf ("(args-multiple) failure\n");
  printf ("(args-multiple) argc=%d\n", argc);
  for (i = 0; i <= argc; i++)
    if (argv[i] >= (char *) 0xbffff000 && argv[i] < (char *) 0xc0000000)
      printf ("(args-multiple) argv[%d]='%s'\n", i, argv[i]);
    else
      printf ("(args-multiple) argv[%d]=%p\n", i, argv[i]);
  return 1;
}
