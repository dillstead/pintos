#include "backdoor.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void
marshal_int32 (int32_t value, void (*out) (uint8_t, void *aux), void *aux) 
{
  out ((value >> 24) & 0xff, aux);
  out ((value >> 16) & 0xff, aux);
  out ((value >> 8) & 0xff, aux);
  out (value & 0xff, aux);
}

static void
marshal_bytes (const void *buffer, size_t cnt,
               void (*out) (uint8_t, void *aux), void *aux) 
{
  const uint8_t *p = buffer;
  size_t i;

  for (i = 0; i < cnt; i++)
    out (p[i], aux);
}

enum backdoor_error
backdoor_vmarshal (const char *types, va_list args,
                   void (*out) (uint8_t, void *aux), void *aux) 
{
  const char *p = types;

  for (;;) 
    {
      /* Find next type character. */
      while (*p == ' ')
        p++;
      if (*p == '\0')
        return BACKDOOR_OK;
      
      out (*p, aux);
      switch (*p++) 
        {
        case 's':
          if (*p == '\'') 
            {
              const char *end = strchr (++p, '\'');
              marshal_int32 (end - p, out, aux);
              marshal_bytes (p, end - p, out, aux);
              p = end + 1;
            }
          else
            {
              const char *s = va_arg (args, const char *);
              marshal_int32 (strlen (s), out, aux);
              marshal_bytes (s, strlen (s), out, aux);
            }
          break;

        case 'i':
          marshal_int32 (va_arg (args, int32_t), out, aux);
          break;

        case 'B':
          {
            const void *buffer = va_arg (args, const void *);
            size_t size = va_arg (args, size_t);
            marshal_int32 (size, out, aux);
            marshal_bytes (buffer, size, out, aux);
          }
          break;

        case 'b':
          marshal_int32 (va_arg (args, int), out, aux);
          break;

        default:
          return BACKDOOR_BAD_TYPE;
        }
    }
}

static bool
unmarshal_int32 (int32_t *value, 
                 bool (*in) (uint8_t *, void *aux), void *aux) 
{
  int32_t tmp;
  int i;

  tmp = 0;
  for (i = 0; i < 4; i++) 
    {
      uint8_t b;
      if (!in (&b, aux))
        return false;

      tmp = (tmp << 8) | b;
    }
  *value = tmp;
  return true;
}

static bool
unmarshal_bytes (void *buffer, size_t cnt,
                 bool (*in) (uint8_t *, void *aux), void *aux) 
{
  uint8_t *p = buffer;
  size_t i;

  for (i = 0; i < cnt; i++) 
    if (!in (&p[i], aux))
      return false;
  return true;
}

enum backdoor_error
backdoor_vunmarshal (const char *types, va_list args,
                     bool (*in) (uint8_t *, void *aux), void *aux) 
{
  const char *p = types;

  for (;;) 
    {
      uint8_t c;
      
      /* Find next type character. */
      while (*p == ' ')
        p++;
      if (*p == '\0')
        return BACKDOOR_OK;

      /* Check type character in input. */
      if (!in (&c, aux))
        return BACKDOOR_UNEXPECTED_EOF;
      if (c != *p++)
        return BACKDOOR_TYPE_MISMATCH;

      switch (c) 
        {
        case 's': 
          {
            int32_t length;
            
            if (!unmarshal_int32 (&length, in, aux))
              return BACKDOOR_UNEXPECTED_EOF;
            if (length < 0)
              return BACKDOOR_NEGATIVE_SIZE;
            if (*p == '\'')
              {
                const char *end = strchr (++p, '\'');
                if (length != end - p)
                  return BACKDOOR_STRING_MISMATCH;
                while (p < end) 
                  {
                    uint8_t q;
                    if (!in (&q, aux))
                      return BACKDOOR_UNEXPECTED_EOF;
                    if (q != *p++)
                      return BACKDOOR_STRING_MISMATCH; 
                  }
                p++;
              }
            else
              {
                char *s = malloc (length + 1);
                if (s == NULL)
                  return BACKDOOR_NOMEM;
                if (!unmarshal_bytes (s, length, in, aux)) 
                  {
                    free (s);
                    return BACKDOOR_UNEXPECTED_EOF;
                  }
                s[length] = '\0';
                *va_arg (args, char **) = s;
              }
          }
          break;

        case 'i':
          if (!unmarshal_int32 (va_arg (args, int32_t *), in, aux))
            return BACKDOOR_UNEXPECTED_EOF;
          break;

        case 'B':
          {
            int32_t size;
            void *buffer;

            if (!unmarshal_int32 (&size, in, aux))
              return BACKDOOR_UNEXPECTED_EOF;
            if (size < 0)
              return BACKDOOR_NEGATIVE_SIZE;
            buffer = malloc (size);
            if (size > 0 && buffer == NULL)
              return BACKDOOR_NOMEM;
            if (!unmarshal_bytes (buffer, size, in, aux))
              {
                free (buffer);
                return BACKDOOR_UNEXPECTED_EOF;
              }
            *va_arg (args, size_t *) = size;
            *va_arg (args, void **) = buffer;
          }
          break;

        case 'b':
          {
            int32_t b;
            if (!unmarshal_int32 (&b, in, aux))
              return BACKDOOR_UNEXPECTED_EOF;
            *va_arg (args, bool *) = b;
          }
          break;

        default:
          return BACKDOOR_BAD_TYPE;
        }
    }
}
