#include <random.h>
#include <stdio.h>
#include <syscall.h>
#include "fslib.h"
#include "syn-rw.h"

const char test_name[] = "syn-rw";

char buf[BUF_SIZE];

#define CHILD_CNT 4

void
test_main (void) 
{
  pid_t children[CHILD_CNT];
  size_t ofs;
  int fd;
  int i;

  check (create (filename, 0), "create \"%s\"", filename);
  check ((fd = open (filename)) > 1, "open \"%s\"", filename);

  for (i = 0; i < CHILD_CNT; i++) 
    {
      char cmd_line[128];
      snprintf (cmd_line, sizeof cmd_line, "child-syn-rw %d", i);
      check ((children[i] = exec (cmd_line)) != PID_ERROR,
             "exec child %d of %d: \"%s\"", i + 1, (int) CHILD_CNT, cmd_line);
    }

  random_bytes (buf, sizeof buf);
  quiet = true;
  for (ofs = 0; ofs < BUF_SIZE; ofs += CHUNK_SIZE)
    check (write (fd, buf + ofs, CHUNK_SIZE) > 0,
           "write %d bytes at offset %zu in \"%s\"",
           (int) CHUNK_SIZE, ofs, filename);
  quiet = false;

  for (i = 0; i < CHILD_CNT; i++) 
    {
      int status = join (children[i]);
      check (status == i, "join child %d of %d", i + 1, (int) CHILD_CNT);
    }
}
