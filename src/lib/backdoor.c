#include "backdoor.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void
backdoor_put_int32 (int32_t value,
                    void (*out) (uint8_t, void *aux), void *aux)
{
  out ((value >> 24) & 0xff, aux);
  out ((value >> 16) & 0xff, aux);
  out ((value >> 8) & 0xff, aux);
  out (value & 0xff, aux);
}

void
backdoor_put_uint32 (uint32_t value,
                     void (*out) (uint8_t, void *aux), void *aux)
{
  out ((value >> 24) & 0xff, aux);
  out ((value >> 16) & 0xff, aux);
  out ((value >> 8) & 0xff, aux);
  out (value & 0xff, aux);
}

void
backdoor_put_bytes (const void *buffer, size_t cnt,
                    void (*out) (uint8_t, void *aux), void *aux) 
{
  const uint8_t *p = buffer;
  size_t i;

  for (i = 0; i < cnt; i++)
    out (p[i], aux);
}

void
backdoor_put_string (const char *string,
                     void (*out) (uint8_t, void *aux), void *aux) 
{
  size_t length = strlen (string);

  backdoor_put_uint32 (length, out, aux);
  backdoor_put_bytes (string, length, out, aux);
}

void
backdoor_put_bool (bool b,
                   void (*out) (uint8_t, void *aux), void *aux) 
{
  backdoor_put_uint32 (b, out, aux);
}

int32_t
backdoor_get_int32 (uint8_t (*in) (void *aux), void *aux) 
{
  int32_t value;
  int i;

  value = 0;
  for (i = 0; i < 4; i++)
    value = (value << 8) | in (aux);
  return value;
}

uint32_t
backdoor_get_uint32 (uint8_t (*in) (void *aux), void *aux) 
{
  return backdoor_get_int32 (in, aux);
}

char *
backdoor_get_string (uint8_t (*in) (void *aux), void *aux) 
{
  size_t length = backdoor_get_uint32 (in, aux);
  char *string = malloc (length + 1);
  backdoor_get_bytes (string, length, in, aux);
  string[length] = '\0';
  return string;
}

void
backdoor_get_bytes (void *buffer, size_t cnt,
                    uint8_t (*in) (void *aux), void *aux)
{
  uint8_t *p = buffer;
  size_t i;

  for (i = 0; i < cnt; i++)
    p[i] = in (aux);
}

bool
backdoor_get_bool (uint8_t (*in) (void *aux), void *aux) 
{
  return backdoor_get_uint32 (in, aux) != 0;
}
