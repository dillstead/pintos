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
#define CHUNK_CNT 16                            /* Number of chunks. */
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

  printf ("page-merge-seq) init\n");

  arc4_init (&arc4, "foobar", 6);
  arc4_crypt (&arc4, buf1, sizeof buf1);
  for (i = 0; i < sizeof buf1; i++)
    histogram[buf1[i]]++;
}

/* Sort each chunk of buf1 using a subprocess. */
static void
sort_chunks (void)
{
  size_t i;

  create ("buffer", CHUNK_SIZE);
  for (i = 0; i < CHUNK_CNT; i++) 
    {
      int fd;

      printf ("(page-merge-seq) sort chunk %zu\n", i);

      /* Write this chunk to a file. */
      fd = open ("buffer");

      if (fd < 0) 
        {
          printf ("(page-merge-seq) open() failed\n");
          exit (1);
        }
      write (fd, buf1 + CHUNK_SIZE * i, CHUNK_SIZE);
      close (fd);

      /* Sort with subprocess. */
      pid_t child = exec ("child-sort buffer");
      if (child == -1) 
        {
          printf ("(page-merge-seq) exec() failed\n");
          exit (1);
        }
      if (wait (child) != 123) 
        {
          printf ("(page-merge-seq) wait(exec()) returned bad value\n");
          exit (1);
        }

      /* Read chunk back from file. */
      fd = open ("buffer");
      if (fd < 0) 
        {
          printf ("(page-merge-seq) open() failed\n");
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

  printf ("(page-merge-seq) merge\n");

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

  printf ("(page-merge-seq) verify\n");

  buf_idx = 0;
  for (hist_idx = 0; hist_idx < sizeof histogram / sizeof *histogram;
       hist_idx++)
    {
      while (histogram[hist_idx]-- > 0) 
        {
          if (buf2[buf_idx] != hist_idx)
            {
              printf ("(page-merge-seq) bad value %d in byte %zu\n",
                      buf2[buf_idx], buf_idx);
              exit (1);
            }
          buf_idx++;
        } 
    }

  printf ("(page-merge-seq) success, buf_idx=%zu\n", buf_idx);
}

int
main (void) 
{
  printf ("(page-merge-seq) begin\n");
  init ();
  sort_chunks ();
  merge ();
  verify ();
  printf ("(page-merge-seq) end\n");
  return 0;
}
