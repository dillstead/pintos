/* Test program for printf() in lib/kernel/stdio.c.

   Attempts to test printf() functionality that is not
   sufficiently tested elsewhere in Pintos.

   This is not a test we will run on your submitted projects.
   It is here for completeness.
*/

#undef NDEBUG
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

/* Test printf() implementation. */
void
test (void) 
{
  long long x;
  int i;

  /* Check that commas show up in the right places, for positive
     numbers. */
  x = i = 0;
  while (x < LLONG_MAX / 10)
    {
      x = x * 10 + ++i % 10;
      printf ("%'26lld %'#29llo %'#19llx\n", x, x, x);
    }

  /* Check that commas show up in the right places, for negative
     numbers. */
  x = i = 0;
  while (x < LLONG_MAX / 10)
    {
      x = x * 10 + ++i % 10;
      printf ("%'26lld %'29llo %'19llx\n", -x, -x, -x);
    }

  /* Check `+', ` ' flags. */
  for (i = -1; i <= +1; i++) 
    {
      printf ("[%d|%+d|% d] ", i, i, i);
      printf ("[%x|%+x|% x]  ", i, i, i);
      printf ("[%o|%+o|% o]\n", i, i, i);
    }

  /* Check `#' flag. */
  for (i = -1; i <= +1; i++) 
    {
      printf ("[%#d|%+#d|% #d] ", i, i, i);
      printf ("[%#x|%+#x|% #x] ", i, i, i);
      printf ("[%#o|%+#o|% #o]\n", i, i, i);
    }

  printf ("0031: [%9s]\n", "abcdefgh");
  printf ("0063: [%- o]\n", 036657730000);
  printf ("0064: [%- u]\n", 4139757568);
  printf ("0065: [%- x]\n", 0xf6bfb000);
  printf ("0178: [%-to]\n", (ptrdiff_t) 036657730000);
  printf ("0191: [%-tu]\n", (ptrdiff_t) 4139757568);
  printf ("0242: [%-zi]\n", (size_t) -155209728);
  printf ("0257: [%-zd]\n", (size_t) -155209728);
  printf ("0347: [%+#o]\n", 036657730000);
  printf ("0349: [%+#x]\n", 0xf6bfb000);
  printf ("0539: [% zi]\n", (size_t) -155209728);
  printf ("0540: [% zd]\n", (size_t) -155209728);
  printf ("0602: [% tu]\n", (ptrdiff_t) 4139757568);
  printf ("0605: [% #o]\n", 036657730000);
  printf ("0607: [% #x]\n", 0xf6bfb000);
  printf ("0702: [%# x]\n", 0xf6bfb000);
  printf ("0875: [%#zd]\n", (size_t) -155209728);
  printf ("1184: [%0zi]\n", (size_t) -155209728);
  printf ("1483: [%'tu]\n", (ptrdiff_t) 4139757568);
  printf ("1557: [%-'d]\n", -155209728);
  printf ("1676: [%.zi]\n", (size_t) -155209728);
  printf ("1897: [%zi]\n", (size_t) -155209728);
  printf ("1899: [%zd]\n", (size_t) -155209728);
  printf ("2791: [%+zi]\n", (size_t) -155209728);
}
