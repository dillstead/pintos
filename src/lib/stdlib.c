#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

/* Converts a string representation of a signed decimal integer
   in S into an `int', which is returned. */
int
atoi (const char *s) 
{
  bool negative;
  int value;

  /* Skip white space. */
  while (isspace (*s))
    s++;

  /* Parse sign. */
  negative = false;
  if (*s == '+')
    s++;
  else if (*s == '-')
    {
      negative = true;
      s++;
    }

  /* Parse digits.  We always initially parse the value as
     negative, and then make it positive later, because the
     negative range of an int is bigger than the positive range
     on a 2's complement system. */
  for (value = 0; isdigit (*s); s++)
    value = value * 10 - (*s - '0');
  if (!negative)
    value = -value;

  return value;
}
