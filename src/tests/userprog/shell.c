#include <stdio.h>
#include <string.h>
#include <syscall.h>

int
main (void)
{
  printf ("Shell starting...\n");
  for (;;) 
    {
      char command[80], *cp;

      /* Prompt. */
      printf ("--");

      /* Read and echo command. */
      cp = command;
      for (;;)
        {
          char c;
          read (STDIN_FILENO, &c, 1);

          switch (c) 
            {
            case '\n':
              /* Done. */
              goto got_cmd;

            case '\b':
              /* Back up cursor, overwrite character, back up again. */
              printf ("\b \b");
              break;

            case 27:                    /* Escape. */
            case ('U' - 'A') + 1:       /* Ctrl+U. */
              /* Clear entire line. */
              printf ("\n--");
              cp = command;
              break;

            default:
              /* Add character to line. */
              *cp++ = c;
              if (cp >= command + sizeof command - 1)
                goto got_cmd;
              break;
            }
        }
    got_cmd:
      *cp = '\0';
      putchar ('\n');
      
      /* Execute command. */
      if (!strcmp (command, "exit"))
        break;
      else if (cp == command) 
        {
          /* Empty command. */
        }
      else
        {
          pid_t pid = exec (command);
          if (pid != PID_ERROR)
            printf ("\"%s\": exit code %d\n", command, wait (pid));
          else
            printf ("exec failed\n");
        }
    }

  printf ("Shell exiting.");
  return 0;
}
