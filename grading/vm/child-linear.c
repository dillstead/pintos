#include <stdio.h>
#include <string.h>
#include "../lib/arc4.h"

#define SIZE (128 * 1024)

static char buf[SIZE];

int
main (int argc, char *argv[]) 
{
  const char *key = argv[argc - 1];
  struct arc4 arc4;
  size_t i;

  /* Encrypt zeros. */
  arc4_init (&arc4, key, strlen (key));
  arc4_crypt (&arc4, buf, SIZE);

  /* Decrypt back to zeros. */
  arc4_init (&arc4, key, strlen (key));
  arc4_crypt (&arc4, buf, SIZE);

  /* Check that it's all zeros. */
  for (i = 0; i < SIZE; i++)
    if (buf[i] != '\0')
      {
        printf ("(child-linear) byte %zu != 0\n", i);
        return 1;
      }

  return 0x42;
}
