#include <stdio.h>
#include <syscall.h>

int
main (void)
{
  for (;;) 
    {
      char command[80], *cp;

      /* Prompt. */
      printf ("--");

      /* Read and echo command. */
      for (cp = command; cp < command + sizeof command - 1; cp++)
        {
          read (STDIN_FILENO, cp, 1);
          putchar (*cp);
          if (*cp == '\n')
            break;
        }
      *cp = '\0';
      
      /* Execute command. */
      if (cp > command) 
        {
          pid_t pid = exec (command);
          if (pid != PID_ERROR)
            wait (pid);
          else
            printf ("exec failed\n");
        }
    }
}
