#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int
main (int argc, char *argv[])
{
  char buffer[128];
  pid_t pid;
  int retval = 0;

  if (argc != 4) 
    {
      printf ("usage: recursor <string> <depth> <joinp>\n");
      exit (1);
    }

  /* Print args. */
  printf ("%s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);

  /* Execute child and wait for it to finish if requested. */
  snprintf (buffer, sizeof buffer,
            "recursor fork %d %s", atoi (argv[2]) + 1, argv[3]);
  pid = exec (buffer);
  if (atoi (argv[3]))
    retval = join (pid);

  /* Done. */
  printf ("%s: dying, retval=%d\n", argv[1], retval);
  exit (retval);
}
