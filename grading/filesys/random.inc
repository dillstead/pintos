/* -*- c -*- */

#include <random.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include "fslib.h"

#if TEST_SIZE % BLOCK_SIZE != 0
#error TEST_SIZE must be a multiple of BLOCK_SIZE
#endif

#define BLOCK_CNT (TEST_SIZE / BLOCK_SIZE)

char buf[TEST_SIZE];
int order[BLOCK_CNT];

int
main (void) 
{
  int fd;
  size_t i;

  msg ("begin");

  random_init (57);
  random_bytes (buf, sizeof buf);

  for (i = 0; i < BLOCK_CNT; i++)
    order[i] = i;

  check (create ("bazzle", TEST_SIZE), "create \"bazzle\"");
  check ((fd = open ("bazzle")) > 1, "open \"bazzle\"");

  msg ("write \"bazzle\" in random order");
  shuffle (order, BLOCK_CNT, sizeof *order);
  for (i = 0; i < BLOCK_CNT; i++) 
    {
      size_t ofs = BLOCK_SIZE * order[i];
      seek (fd, ofs);
      if (write (fd, buf + ofs, BLOCK_SIZE) <= 0)
        fail ("write %d bytes at offset %zu failed", (int) BLOCK_SIZE, ofs);
    }

  msg ("read \"bazzle\" in random order");
  shuffle (order, BLOCK_CNT, sizeof *order);
  for (i = 0; i < BLOCK_CNT; i++) 
    {
      char block[BLOCK_SIZE];
      size_t ofs = BLOCK_SIZE * order[i];
      seek (fd, ofs);
      if (read (fd, block, BLOCK_SIZE) <= 0)
        fail ("read %d bytes at offset %zu failed", (int) BLOCK_SIZE, ofs);
      if (memcmp (block, buf + ofs, BLOCK_SIZE)) 
        {
          printf ("Expected data:\n");
          hex_dump (ofs, buf + ofs, BLOCK_SIZE, false);
          printf ("Actually read data:\n");
          hex_dump (ofs, block, BLOCK_SIZE, false);
          fail ("%d bytes at offset %zu differed from expected",
                (int) BLOCK_SIZE, ofs);
        }
    }

  fail ("close \"bazzle\"");
  close (fd);

  msg ("end");
  return 0;
}
