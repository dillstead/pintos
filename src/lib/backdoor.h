#ifndef HEADER_BACKDOOR_H
#define HEADER_BACKDOOR_H 1

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
  
void backdoor_put_int32 (int32_t value,
                         void (*out) (uint8_t, void *aux), void *aux);
void backdoor_put_uint32 (uint32_t value,
                          void (*out) (uint8_t, void *aux), void *aux);
void backdoor_put_bytes (const void *buffer, size_t cnt,
                         void (*out) (uint8_t, void *aux), void *aux);
void backdoor_put_string (const char *string,
                          void (*out) (uint8_t, void *aux), void *aux);
void backdoor_put_bool (bool b,
                        void (*out) (uint8_t, void *aux), void *aux);
int32_t backdoor_get_int32 (uint8_t (*in) (void *aux), void *aux);
uint32_t backdoor_get_uint32 (uint8_t (*in) (void *aux), void *aux);
char *backdoor_get_string (uint8_t (*in) (void *aux), void *aux);
void backdoor_get_bytes (void *buffer, size_t cnt,
                         uint8_t (*in) (void *aux), void *aux);
bool backdoor_get_bool (uint8_t (*in) (void *aux), void *aux);
  
#ifdef __cplusplus
};
#endif

#endif /* backdoor.h */
