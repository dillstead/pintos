#include <stdio.h>
#include "../lib/arc4.h"

#define SIZE (128 * 1024)

static char buf[SIZE];

int
main (void) 
{
  struct arc4 arc4;
  size_t i;

  printf ("(page-linear) begin\n");

  /* Encrypt zeros. */
  printf ("(page-linear) read/modify/write pass one\n");
  arc4_init (&arc4, "foobar", 6);
  arc4_crypt (&arc4, buf, SIZE);

  /* Decrypt back to zeros. */
  printf ("(page-linear) read/modify/write pass two\n");
  arc4_init (&arc4, "foobar", 6);
  arc4_crypt (&arc4, buf, SIZE);

  /* Check that it's all zeros. */
  printf ("(page-linear) read pass\n");
  for (i = 0; i < SIZE; i++)
    if (buf[i] != '\0')
      {
        printf ("(page-linear) byte %zu != 0\n", i);
        return 1;
      }

  /* Done. */
  printf ("(page-linear) end\n");

  return 0;
}
