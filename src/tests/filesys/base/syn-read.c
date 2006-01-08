/* Spawns 10 child processes, all of which read from the same
   file and make sure that the contents are what they should
   be. */

#include <random.h>
#include <stdio.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"
#include "tests/filesys/base/syn-read.h"

static char buf[BUF_SIZE];

#define CHILD_CNT 10

void
test_main (void) 
{
  pid_t children[CHILD_CNT];
  int fd;

  CHECK (create (filename, sizeof buf), "create \"%s\"", filename);
  CHECK ((fd = open (filename)) > 1, "open \"%s\"", filename);
  random_bytes (buf, sizeof buf);
  CHECK (write (fd, buf, sizeof buf) > 0, "write \"%s\"", filename);
  msg ("close \"%s\"", filename);
  close (fd);

  exec_children ("child-syn-read", children, CHILD_CNT);
  wait_children (children, CHILD_CNT);
}
