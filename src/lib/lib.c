#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include "debug.h"
#include "lib.h"
#include "serial.h"
#include "vga.h"

void *
memset (void *dst_, int value, size_t cnt) 
{
  uint8_t *dst = dst_;
  
  while (cnt-- > 0)
    *dst++ = value;

  return dst_;
}

void *
memcpy (void *dst_, const void *src_, size_t cnt) 
{
  uint8_t *dst = dst_;
  const uint8_t *src = src_;

  while (cnt-- > 0)
    *dst++ = *src++;

  return dst_;
}

void *
memmove (void *dst_, const void *src_, size_t cnt) 
{
  uint8_t *dst = dst_;
  const uint8_t *src = src_;

  if (dst < src) 
    {
      while (cnt-- > 0)
        *dst++ = *src++;
    }
  else 
    {
      dst += cnt;
      src += cnt;
      while (cnt-- > 0)
        *--dst = *--src;
    }

  return dst;
}

void *
memchr (const void *block_, int ch_, size_t size) 
{
  const uint8_t *block = block_;
  uint8_t ch = ch_;

  for (; size-- > 0; block++)
    if (*block == ch)
      return (void *) block;

  return NULL;
}

size_t
strlcpy (char *dst, const char *src, size_t size) 
{
  size_t src_len = strlen (src);
  if (size > 0) 
    {
      size_t dst_len_max = size - 1;
      size_t dst_len = src_len < dst_len_max ? src_len : dst_len_max;
      memcpy (dst, src, dst_len);
      dst[dst_len] = '\0';
    }
  return src_len;
}

size_t
strlen (const char *string) 
{
  const char *p;

  for (p = string; *p != '\0'; p++)
    continue;
  return p - string;
}

char *
strchr (const char *string, int c_) 
{
  char c = c_;

  for (;;) 
    if (*string == c)
      return (char *) string;
    else if (*string == '\0')
      return NULL;
    else string++;
}

static void
vprintf_core (const char *format, va_list args,
              void (*output) (char, void *), void *aux);

static void
output_console (char ch, void *aux __attribute__ ((unused))) 
{
  vga_putc (ch);
  serial_outb (ch);
}

void
vprintk (const char *format, va_list args) 
{
  vprintf_core (format, args, output_console, NULL);
}

void
printk (const char *format, ...) 
{
  va_list args;

  va_start (args, format);
  vprintk (format, args);
  va_end (args);
}

/* printf() and friends internals.  You do not need to understand
   this code. */

struct printf_conversion 
  {
    enum 
      {
        MINUS = 1 << 0,
        PLUS = 1 << 1,
        SPACE = 1 << 2,
        POUND = 1 << 3,
        ZERO = 1 << 4
      }
    flags;

    int width;
    int precision;

    enum 
      {
        CHAR = 1,
        SHORT = 2,
        INT = 3,
        INTMAX = 4,
        LONG = 5,
        LONGLONG = 6,
        PTRDIFFT = 7,
        SIZET = 8
      }
    type;
  };

static const char *
parse_conversion (const char *format, struct printf_conversion *c, va_list *args) 
{
  /* Parse flag characters. */
  c->flags = 0;
  for (;;) 
    {
      switch (*format++) 
        {
        case '-':
          c->flags |= MINUS;
          break;
        case '+':
          c->flags |= PLUS;
          break;
        case ' ':
          c->flags |= SPACE;
          break;
        case '#':
          c->flags |= POUND;
          break;
        case '0':
          c->flags |= ZERO;
          break;
        default:
          format--;
          goto not_a_flag;
        }
    }
 not_a_flag:
  if (c->flags & MINUS)
    c->flags &= ~ZERO;
  if (c->flags & PLUS)
    c->flags &= ~SPACE;

  /* Parse field width. */
  c->width = 0;
  if (*format == '*')
    {
      format++;
      c->width = va_arg (*args, int);
    }
  else 
    {
      for (; isdigit (*format); format++)
        c->width = c->width * 10 + *format - '0';
    }
  if (c->width < 0) 
    {
      c->width = -c->width;
      c->flags |= MINUS;
    }
      
  /* Parse precision. */
  c->precision = -1;
  if (*format == '.') 
    {
      format++;
      if (*format == '*') 
        {
          format++;
          c->precision = va_arg (*args, int);
        }
      else 
        {
          c->precision = 0;
          for (; isdigit (*format); format++)
            c->precision = c->precision * 10 + *format - '0';
        }
      if (c->precision < 0) 
        c->precision = -1;
    }
  if (c->precision >= 0)
    c->flags &= ~ZERO;

  /* Parse type. */
  c->type = INT;
  switch (*format++) 
    {
    case 'h':
      if (*format == 'h') 
        {
          format++;
          c->type = CHAR;
        }
      else
        c->type = SHORT;
      break;
      
    case 'j':
      c->type = INTMAX;
      break;

    case 'l':
      if (*format == 'l')
        {
          format++;
          c->type = LONGLONG;
        }
      else
        c->type = LONG;
      break;

    case 'L':
    case 'q':
      c->type = LONGLONG;
      break;

    case 't':
      c->type = PTRDIFFT;
      break;

    case 'z':
    case 'Z':
      c->type = SIZET;
      break;

    default:
      format--;
      break;
    }

  return format;
}

static void
output_dup (char ch, size_t cnt, void (*output) (char, void *), void *aux) 
{
  while (cnt-- > 0)
    output (ch, aux);
}

static void
printf_integer (uintmax_t value, bool negative, const char *digits,
                struct printf_conversion *c,
                void (*output) (char, void *), void *aux)

{
  int base = strlen (digits);

  char buf[64];
  char *const buf_end = buf + sizeof buf;
  char *bp;

  int pad;

  /* Accumulate digits into buffer.
     This algorithm produces digits in reverse order, so we count
     backward from the end of the array. */
  bp = buf_end;
  while (value > 0) 
    {
      *--bp = digits[value % base];
      value /= base;
    }

  /* Prepend enough zeros to match precision.
     If precision is 0, then a value of zero is rendered as a
     null string.  Otherwise at least one digit is presented. */
  if (c->precision < 0)
    c->precision = 1;
  while (buf_end - bp < c->precision && bp > buf + 3)
    *--bp = '0';

  /* Prepend sign. */
  if (c->flags & PLUS)
    *--bp = negative ? '-' : '+';
  else if (c->flags & SPACE)
    *--bp = negative ? '-' : ' ';
  else if (negative)
    *--bp = '-';

  /* Prepend base. */
  if ((c->flags & POUND) && base != 10) 
    {
      *--bp = digits[0xa] == 'a' ? 'x' : 'X';
      *--bp = '0'; 
    }

  /* Calculate number of pad characters to fill field width. */
  pad = c->width - (buf_end - bp);
  if (pad < 0)
    pad = 0;

  /* Do output. */
  if ((c->flags & (MINUS | ZERO)) == 0)
    output_dup (' ', pad, output, aux);
  if (bp < buf_end && strchr (digits, *bp) == NULL)
    output (*bp++, aux);
  if (c->flags & ZERO)
    output_dup ('0', pad, output, aux);
  while (bp < buf_end)
    output (*bp++, aux);
  if (c->flags & MINUS)
    output_dup (' ', pad, output, aux);
}

static void
printf_string (const char *string, size_t length,
               struct printf_conversion *c,
               void (*output) (char, void *), void *aux) 
{
  if (c->width > 1 && (c->flags & MINUS) == 0)
    output_dup (' ', c->width - 1, output, aux);
  while (length-- > 0)
    output (*string++, aux);
  if (c->width > 1 && (c->flags & MINUS) != 0)
    output_dup (' ', c->width - 1, output, aux);
}

static void
printf_core (const char *format,
             void (*output) (char, void *), void *aux, ...) 
{
  va_list args;

  va_start (args, aux);
  vprintf_core (format, args, output, aux);
  va_end (args);
}

static void
vprintf_core (const char *format, va_list args,
              void (*output) (char, void *), void *aux)
{
  for (; *format != '\0'; format++)
    {
      struct printf_conversion c;

      /* Literally copy non-conversions to output. */
      if (*format != '%') 
        {
          output (*format, aux);
          continue;
        }
      format++;

      /* %% => %. */
      if (*format == '%') 
        {
          output ('%', aux);
          continue;
        }

      format = parse_conversion (format, &c, &args);
      switch (*format) 
        {
        case 'd':
        case 'i': 
          {
            intmax_t value;
            uintmax_t abs_value;
            bool negative = false;

            switch (c.type) 
              {
              case CHAR: 
                value = (signed char) va_arg (args, int);
                break;
              case SHORT:
                value = (short) va_arg (args, int);
                break;
              case INT:
                value = va_arg (args, int);
                break;
              case LONG:
                value = va_arg (args, long);
                break;
              case LONGLONG:
                value = va_arg (args, long long);
                break;
              case PTRDIFFT:
                value = va_arg (args, ptrdiff_t);
                break;
              case SIZET:
                value = va_arg (args, size_t);
                break;
              default:
                NOT_REACHED ();
              }

            if (value < 0) 
              {
                negative = true;
                abs_value = -value;
              }
            else
              abs_value = value;

            printf_integer (abs_value, negative, "0123456789",
                            &c, output, aux);
          }
          break;
          
        case 'o':
        case 'u':
        case 'x':
        case 'X':
          {
            uintmax_t value;
            const char *digits;

            switch (c.type) 
              {
              case CHAR: 
                value = (unsigned char) va_arg (args, unsigned);
                break;
              case SHORT:
                value = (unsigned short) va_arg (args, unsigned);
                break;
              case INT:
                value = va_arg (args, unsigned);
                break;
              case LONG:
                value = va_arg (args, unsigned long);
                break;
              case LONGLONG:
                value = va_arg (args, unsigned long long);
                break;
              case PTRDIFFT:
                value = va_arg (args, ptrdiff_t);
                break;
              case SIZET:
                value = va_arg (args, size_t);
                break;
              default:
                NOT_REACHED ();
              }

            switch (*format) 
              {
              case 'o':
                digits = "01234567";
                break;
              case 'u':
                digits = "0123456789";
                break;
              case 'x':
                digits = "0123456789abcdef";
                break;
              case 'X':
                digits = "0123456789ABCDEF";
                break;
              default:
                NOT_REACHED ();
              }
            
            printf_integer (value, false, digits, &c, output, aux);
          }
          break;

        case 'c': 
          {
            char ch = va_arg (args, int);
            printf_string (&ch, 1, &c, output, aux);
          }
          break;

        case 's':
          {
            const char *s;
            size_t length;
            
            s = va_arg (args, char *);
            if (s == NULL)
              s = "(null)";

            if (c.precision >= 0) 
              {
                const char *zero = memchr (s, '\0', c.precision);
                if (zero != NULL)
                  length = zero - s;
                else
                  length = c.precision;
              }
            else
              length = strlen (s);

            printf_string (s, length, &c, output, aux);
          }
          break;
          
        case 'p':
          {
            void *p = va_arg (args, void *);

            c.flags = POUND;
            if (p != NULL) 
              printf_integer ((uintptr_t) p,
                              false, "0123456789abcdef", &c,
                              output, aux); 
            else
              printf_string ("(nil)", 5, &c, output, aux); 
          }
          break;
      
        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
        case 'n':
          printf_core ("<<no %%%c in kernel>>", output, aux, *format);
          break;

        default:
          printf_core ("<<no %%%c conversion>>", output, aux, *format);
          break;
        }
    }
}
