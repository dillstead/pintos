#include <stdio.h>
#include <stdlib.h>
#ifdef PINTOS
#include <syscall.h>
#else
#include "posix-compat.h"
#endif
#include "../lib/arc4.h"

/* This is the max file size for an older version of the Pintos
   file system that had 126 direct blocks each pointing to a
   single disk sector.  We could raise it now. */
#define CHUNK_SIZE (126 * 512)
#define CHUNK_CNT 8                             /* Number of chunks. */
#define DATA_SIZE (CHUNK_CNT * CHUNK_SIZE)      /* Buffer size. */

unsigned char buf1[DATA_SIZE], buf2[DATA_SIZE];
size_t histogram[256];

/* Initialize buf1 with random data,
   then count the number of instances of each value within it. */
static void
init (void) 
{
  struct arc4 arc4;
  size_t i;

  printf ("(page-merge-par) init\n");

  arc4_init (&arc4, "foobar", 6);
  arc4_crypt (&arc4, buf1, sizeof buf1);
  for (i = 0; i < sizeof buf1; i++)
    histogram[buf1[i]]++;
}

/* Sort each chunk of buf1 using a subprocess. */
static void
sort (void)
{
  pid_t children[CHUNK_CNT];
  size_t i;

  for (i = 0; i < CHUNK_CNT; i++) 
    {
      char fn[128];
      char cmd[128];
      int fd;

      printf ("(page-merge-par) sort chunk %zu\n", i);

      /* Write this chunk to a file. */
      snprintf (fn, sizeof fn, "buf%zu", i);
      create (fn, CHUNK_SIZE);
      fd = open (fn);
      if (fd < 0) 
        {
          printf ("(page-merge-par) open() failed\n");
          exit (1);
        }
      write (fd, buf1 + CHUNK_SIZE * i, CHUNK_SIZE);
      close (fd);

      /* Sort with subprocess. */
      snprintf (cmd, sizeof cmd, "child-sort %s", fn);
      children[i] = exec (cmd);
      if (children[i] == -1) 
        {
          printf ("(page-merge-par) exec() failed\n");
          exit (1);
        } 
    }

  for (i = 0; i < CHUNK_CNT; i++) 
    {
      char fn[128];
      int fd;

      if (join (children[i]) != 123) 
        {
          printf ("(page-merge-par) join(exec()) returned bad value\n");
          exit (1);
        }

      /* Read chunk back from file. */
      snprintf (fn, sizeof fn, "buf%zu", i);
      fd = open (fn);
      if (fd < 0) 
        {
          printf ("(page-merge-par) open() failed\n");
          exit (1);
        }
      read (fd, buf1 + CHUNK_SIZE * i, CHUNK_SIZE);
      close (fd);
    }
}

/* Merge the sorted chunks in buf1 into a fully sorted buf2. */
static void
merge (void) 
{
  unsigned char *mp[CHUNK_CNT];
  size_t mp_left;
  unsigned char *op;
  size_t i;

  printf ("(page-merge-par) merge\n");

  /* Initialize merge pointers. */
  mp_left = CHUNK_CNT;
  for (i = 0; i < CHUNK_CNT; i++)
    mp[i] = buf1 + CHUNK_SIZE * i;

  /* Merge. */
  op = buf2;
  while (mp_left > 0) 
    {
      /* Find smallest value. */
      size_t min = 0;
      for (i = 1; i < mp_left; i++)
        if (*mp[i] < *mp[min])
          min = i;

      /* Append value to buf2. */
      *op++ = *mp[min];

      /* Advance merge pointer.
         Delete this chunk from the set if it's emptied. */
      if ((++mp[min] - buf1) % CHUNK_SIZE == 0) 
        mp[min] = mp[--mp_left];
    }
}

static void
verify (void) 
{
  size_t buf_idx;
  size_t hist_idx;

  printf ("(page-merge-par) verify\n");

  buf_idx = 0;
  for (hist_idx = 0; hist_idx < sizeof histogram / sizeof *histogram;
       hist_idx++)
    {
      while (histogram[hist_idx]-- > 0) 
        {
          if (buf2[buf_idx] != hist_idx)
            {
              printf ("(page-merge-par) bad value %d in byte %zu\n",
                      buf2[buf_idx], buf_idx);
              exit (1);
            }
          buf_idx++;
        } 
    }

  printf ("(page-merge-par) success, buf_idx=%zu\n", buf_idx);
}

int
main (void) 
{
  printf ("(page-merge-par) begin\n");
  init ();
  sort ();
  merge ();
  verify ();
  printf ("(page-merge-par) end\n");
  return 0;
}
