#ifndef HEADER_BACKDOOR_H
#define HEADER_BACKDOOR_H 1

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

enum backdoor_error 
  {
    BACKDOOR_OK = 0,
    BACKDOOR_NOMEM = -100,
    BACKDOOR_BAD_TYPE,
    BACKDOOR_TYPE_MISMATCH,
    BACKDOOR_STRING_MISMATCH,
    BACKDOOR_NEGATIVE_SIZE,
    BACKDOOR_UNEXPECTED_EOF
  };

enum backdoor_error
backdoor_vmarshal (const char *types, va_list args,
                   void (*) (uint8_t, void *aux), void *aux);
enum backdoor_error
backdoor_vunmarshal (const char *types, va_list args,
                     bool (*) (uint8_t *, void *aux), void *aux);

#endif /* backdoor.h */
