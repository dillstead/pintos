/* -*- c -*- */

#include <syscall.h>
#include <stdio.h>
#include "fslib.h"

static char buf[512];

static size_t
return_block_size (void) 
{
  return sizeof buf;
}

int
main (void) 
{
  size_t i;
  
  msg ("begin");
#ifdef DIRECTORY
  check (mkdir (DIRECTORY), "mkdir %s", DIRECTORY);
#define DIR_PREFIX DIRECTORY "/"
#else
#define DIR_PREFIX ""
#endif
  for (i = 0; i < FILE_CNT; i++) 
    {
      char filename[128];
      snprintf (filename, sizeof filename, "%sfile%zu", DIR_PREFIX, i);

      msg ("creating and checking \"%s\"", filename);

      quiet = true;
      seq_test (filename, buf, sizeof buf, sizeof buf, 0,
                return_block_size, NULL); 
      quiet = false;
    }
  msg ("end");
  return 0;
}
