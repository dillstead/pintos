#include <stdio.h>
#include <stdlib.h>
#ifdef PINTOS
#include <syscall.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif
#include "arc4.h"

#define CHUNK_SIZE (63 * 1024)                  /* Max file size. */
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
sort (void)
{
  size_t i;

#ifdef PINTOS
  create ("buffer", CHUNK_SIZE);
#endif
  for (i = 0; i < CHUNK_CNT; i++) 
    {
      int fd;

      printf ("(page-merge-seq) sort chunk %zu\n", i);

      /* Write this chunk to a file. */
#ifdef PINTOS
      fd = open ("buffer");
#else
      fd = open ("buffer", O_WRONLY | O_CREAT, 0660);
#endif
      if (fd < 0) 
        {
          printf ("(page-merge-seq) open() failed\n");
          exit (1);
        }
      write (fd, buf1 + CHUNK_SIZE * i, CHUNK_SIZE);
      close (fd);

      /* Sort with subprocess. */
#ifdef PINTOS
      pid_t child = exec ("child-sort");
      if (child == -1) 
        {
          printf ("(page-merge-seq) exec() failed\n");
          exit (1);
        }
      if (join (child) != 0x123) 
        {
          printf ("(page-merge-seq) join(exec()) returned bad value\n");
          exit (1);
        }
#else
      system ("./child-sort");
#endif

      /* Read chunk back from file. */
#ifdef PINTOS
      fd = open ("buffer");
#else
      fd = open ("buffer", O_RDONLY);
#endif
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
  sort ();
  merge ();
  verify ();
  printf ("(page-merge-seq) end\n");
  return 0;
}
