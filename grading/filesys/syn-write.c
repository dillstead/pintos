#include <random.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include "fslib.h"
#include "syn-write.h"

const char test_name[] = "syn-write";

char buf1[BUF_SIZE];
char buf2[BUF_SIZE];

void
test_main (void) 
{
  pid_t children[CHILD_CNT];
  int fd;
  int i;

  check (create (filename, sizeof buf1), "create \"%s\"", filename);

  for (i = 0; i < CHILD_CNT; i++) 
    {
      char cmd_line[128];
      snprintf (cmd_line, sizeof cmd_line, "child-syn-wrt %d", i);
      check ((children[i] = exec (cmd_line)) != PID_ERROR,
             "exec child %d of %d: \"%s\"", i + 1, (int) CHILD_CNT, cmd_line);
    }

  for (i = 0; i < CHILD_CNT; i++) 
    {
      int status = join (children[i]);
      check (status == i, "join child %d of %d", i + 1, (int) CHILD_CNT);
    }

  check ((fd = open (filename)) > 1, "open \"%s\"", filename);
  check (read (fd, buf1, sizeof buf1) > 0, "read \"%s\"", filename);
  random_bytes (buf2, sizeof buf2);
  compare_bytes (buf1, buf2, sizeof buf1, 0, filename);
}
