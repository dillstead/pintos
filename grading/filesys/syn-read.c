#include <random.h>
#include <stdio.h>
#include <syscall.h>
#include "fslib.h"
#include "syn-read.h"

const char test_name[] = "syn-read";

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
