#include <ctype.h>
#include <debug.h>
#include <random.h>
#include <stdlib.h>
#include <stdbool.h>

/* Converts a string representation of a signed decimal integer
   in S into an `int', which is returned. */
int
atoi (const char *s) 
{
  bool negative;
  int value;

  ASSERT (s != NULL);

  /* Skip white space. */
  while (isspace ((unsigned char) *s))
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

/* Compares A and B by calling the AUX function. */
static int
compare_thunk (const void *a, const void *b, void *aux) 
{
  int (**compare) (const void *, const void *) = aux;
  return (*compare) (a, b);
}

/* Sorts ARRAY, which contains CNT elements of SIZE bytes each,
   using COMPARE.  When COMPARE is passed a pair of elements A
   and B, respectively, it must return a strcmp()-type result,
   i.e. less than zero if A < B, zero if A == B, greater than
   zero if A > B. */
void
qsort (void *array, size_t cnt, size_t size,
       int (*compare) (const void *, const void *)) 
{
  quick_sort (array, cnt, size, compare_thunk, &compare);
}

/* Swaps the SIZE bytes at A and B. */
static void
swap (unsigned char *a, unsigned char *b, size_t size) 
{
  size_t i;

  for (i = 0; i < size; i++)
    {
      unsigned char t = a[i];
      a[i] = b[i];
      b[i] = t;
    }
}

/* Selects a random "pivot" element in ARRAY, which contains CNT
   elements of SIZE bytes each, and partitions ARRAY into two
   contiguous subranges around the pivot, so that the first N
   elements are less than or equal to the pivot and the remaining
   elements are greater than the pivot.  Returns N.

   Uses COMPARE to compare elements, passing AUX as auxiliary
   data.  When COMPARE is passed a pair of elements A and B,
   respectively, it must return a strcmp()-type result, i.e. less
   than zero if A < B, zero if A == B, greater than zero if A >
   B. */
static size_t
randomized_partition (void *array, size_t cnt, size_t size,
                      int (*compare) (const void *, const void *, void *aux),
                      void *aux) 
{
  unsigned char *first = array;                 /* Beginning of array. */
  unsigned char *last = first + size * cnt;     /* End of array. */
  unsigned char *pivot = last - size;           /* Element used as pivot. */
  unsigned char *middle;        /* Invariant: elements on left are <= pivot. */
  unsigned char *current;

  ASSERT (array != NULL);
  ASSERT (cnt > 1);
  ASSERT (size > 0);
  ASSERT (compare != NULL);
  
  /* Choose a random pivot. */
  swap (pivot, first + (random_ulong () % cnt) * size, size);

  /* Iterate through the array moving elements less than or equal
     to the pivot to the middle. */
  middle = array;
  for (current = first; current < last; current += size)
    if (compare (current, pivot, aux) <= 0) 
      {
        swap (middle, current, size);
        middle += size; 
      }
  
  return (middle - first) / size;
}

/* Sorts ARRAY, which contains CNT elements of SIZE bytes each,
   using COMPARE to compare elements, passing AUX as auxiliary
   data.  When COMPARE is passed a pair of elements A and B,
   respectively, it must return a strcmp()-type result, i.e. less
   than zero if A < B, zero if A == B, greater than zero if A >
   B. */
void
quick_sort (void *array_, size_t cnt, size_t size,
            int (*compare) (const void *, const void *, void *aux),
            void *aux) 
{
  unsigned char *array = array_;

  ASSERT (array != NULL || cnt == 0);
  ASSERT (compare != NULL);
  ASSERT (size > 0);
  
  if (cnt > 1) 
    {
      size_t q = randomized_partition (array, cnt, size, compare, aux);

      /* Sort smaller partition first to guarantee O(lg n) stack
         depth limit. */
      if (q < cnt - q) 
        {
          quick_sort (array, q, size, compare, aux);
          quick_sort (array + q * size, cnt - q, size, compare, aux);
        }
      else 
        {
          quick_sort (array + q * size, cnt - q, size, compare, aux);
          quick_sort (array, q, size, compare, aux);
        }
    }
}

/* Searches ARRAY, which contains CNT elements of SIZE bytes
   each, for the given KEY.  Returns a match is found, otherwise
   a null pointer.  If there are multiple matches, returns an
   arbitrary one of them.

   ARRAY must be sorted in order according to COMPARE.

   Uses COMPARE to compare elements.  When COMPARE is passed a
   pair of elements A and B, respectively, it must return a
   strcmp()-type result, i.e. less than zero if A < B, zero if A
   == B, greater than zero if A > B. */
void *
bsearch (const void *key, const void *array, size_t cnt,
         size_t size, int (*compare) (const void *, const void *)) 
{
  return binary_search (key, array, cnt, size, compare_thunk, &compare);
}

/* Searches ARRAY, which contains CNT elements of SIZE bytes
   each, for the given KEY.  Returns a match is found, otherwise
   a null pointer.  If there are multiple matches, returns an
   arbitrary one of them.

   ARRAY must be sorted in order according to COMPARE.

   Uses COMPARE to compare elements, passing AUX as auxiliary
   data.  When COMPARE is passed a pair of elements A and B,
   respectively, it must return a strcmp()-type result, i.e. less
   than zero if A < B, zero if A == B, greater than zero if A >
   B. */
void *
binary_search (const void *key, const void *array, size_t cnt, size_t size,
               int (*compare) (const void *, const void *, void *aux),
               void *aux) 
{
  const unsigned char *first = array;
  const unsigned char *last = array + size * cnt;

  while (first < last) 
    {
      size_t range = (last - first) / size;
      const unsigned char *middle = first + (range / 2) * size;
      int cmp = compare (key, middle, aux);

      if (cmp < 0) 
        last = middle;
      else if (cmp > 0) 
        first = middle + size;
      else
        return (void *) middle;
    }
  
  return NULL;
}

