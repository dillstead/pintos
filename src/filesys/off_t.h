#ifndef HEADER_OFF_T_H
#define HEADER_OFF_T_H 1

#include <stdint.h>

/* An offset within a file.
   This is a separate header because multiple headers want this
   definition but not any others. */
typedef int32_t off_t;

#endif /* off_t.h */
