#include <random.h>
#include <stdio.h>
#include <syscall.h>
#include "fslib.h"

const char test_name[] = "syn-read";

static char buf[1024];

#define CHILD_CNT 10

void
test_main (void) 
{
  const char *filename = "data";
  pid_t children[CHILD_CNT];
  int fd;
  int i;

  check (create (filename, sizeof buf), "create \"%s\"", filename);
  check ((fd = open (filename)) > 1, "open \"%s\"", filename);
  random_bytes (buf, sizeof buf);
  check (write (fd, buf, sizeof buf) > 0, "write \"%s\"", filename);
  msg ("close \"%s\"", filename);
  close (fd);

  for (i = 0; i < CHILD_CNT; i++) 
    {
      char cmd_line[128];
      snprintf (cmd_line, sizeof cmd_line,
                "child-syn-read %d %zu", i, sizeof buf);
      check ((children[i] = exec (cmd_line)) != PID_ERROR,
             "exec \"%s\"", cmd_line);
    }

  for (i = 0; i < CHILD_CNT; i++) 
    {
      int status = join (children[i]);
      check (status == i, "join child %d of %d", i + 1, (int) CHILD_CNT);
    }
}
