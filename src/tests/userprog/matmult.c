/* matmult.c 

   Test program to do matrix multiplication on large arrays.
 
   Intended to stress virtual memory system.
   
   Ideally, we could read the matrices off of the file system,
   and store the result back to the file system!
 */

#include <syscall.h>

/* You should define this to be large enough that the arrays
   don't fit in physical memory.

   Dim    Memory
   ---  --------
    20     93 kB
    30    316 kB
    40    750 kB
    50  1,464 kB
    60  2,531 kB
    70  4,019 kB
    80  6,000 kB
    90  8,542 kB
   100 11,718 kB */
#define Dim 20

int A[Dim][Dim];
int B[Dim][Dim];
int C[Dim][Dim];

int
main (void)
{
  int i, j, k;

  /* Initialize the matrices. */
  for (i = 0; i < Dim; i++)
    for (j = 0; j < Dim; j++)
      {
	A[i][j] = i;
	B[i][j] = j;
	C[i][j] = 0;
      }

  /* Multiply matrices. */
  for (i = 0; i < Dim; i++)	
    for (j = 0; j < Dim; j++)
      for (k = 0; k < Dim; k++)
	C[i][j] += A[i][k] * B[k][j];

  /* Done. */
  exit (C[Dim - 1][Dim - 1]);
}
