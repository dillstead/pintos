#ifndef __LIB_USER_USER_H
#define __LIB_USER_USER_H

#ifdef KERNEL
#error This header is user-only.
#endif

/* <stdlib.h> */
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
void exit (int);
void abort (void);

#endif /* lib/user/user.h */
