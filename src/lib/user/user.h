#ifndef LIB_USER_H
#define LIB_USER_H 1

#ifdef KERNEL
#error This header is user-only.
#endif

/* <stdlib.h> */
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
void exit (int);
void abort (void);

#endif /* lib/user.h */
