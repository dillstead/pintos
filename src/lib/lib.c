#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include "debug.h"
#include "interrupt.h"
#include "lib.h"
#include "serial.h"
#include "vga.h"

static void
vprintf_core (const char *format, va_list args,
              void (*output) (char, void *), void *aux);

/* <string.h> */

/* Sets the SIZE bytes in DST to VALUE. */
void *
memset (void *dst_, int value, size_t size) 
{
  unsigned char *dst = dst_;

  ASSERT (dst != NULL || size == 0);
  
  while (size-- > 0)
    *dst++ = value;

  return dst_;
}

/* Copies SIZE bytes from SRC to DST, which must not overlap.
   Returns DST. */
void *
memcpy (void *dst_, const void *src_, size_t size) 
{
  unsigned char *dst = dst_;
  const unsigned char *src = src_;

  ASSERT (dst != NULL || size == 0);
  ASSERT (src != NULL || size == 0);

  while (size-- > 0)
    *dst++ = *src++;

  return dst_;
}

/* Copies SIZE bytes from SRC to DST, which are allowed to
   overlap.  Returns DST. */
void *
memmove (void *dst_, const void *src_, size_t size) 
{
  unsigned char *dst = dst_;
  const unsigned char *src = src_;

  ASSERT (dst != NULL || size == 0);
  ASSERT (src != NULL || size == 0);

  if (dst < src) 
    {
      while (size-- > 0)
        *dst++ = *src++;
    }
  else 
    {
      dst += size;
      src += size;
      while (size-- > 0)
        *--dst = *--src;
    }

  return dst;
}

/* Returns a pointer to the first occurrence of CH in the first
   SIZE bytes starting at BLOCK.  Returns a null pointer if CH
   does not occur in BLOCK. */
void *
memchr (const void *block_, int ch_, size_t size) 
{
  const unsigned char *block = block_;
  unsigned char ch = ch_;

  ASSERT (block != NULL || size == 0);

  for (; size-- > 0; block++)
    if (*block == ch)
      return (void *) block;

  return NULL;
}

/* Find the first differing byte in the two blocks of SIZE bytes
   at A and B.  Returns a positive value if the byte in A is
   greater, a negative value if the byte in B is greater, or zero
   if blocks A and B are equal. */
int
memcmp (const void *a_, const void *b_, size_t size) 
{
  const unsigned char *a = a_;
  const unsigned char *b = b_;

  ASSERT (a != NULL || size == 0);
  ASSERT (b != NULL || size == 0);

  for (; size-- > 0; a++, b++)
    if (*a != *b)
      return *a > *b ? +1 : -1;
  return 0;
}

/* Finds and returns the first occurrence of C in STRING, or a
   null pointer if C does not appear in STRING.  If C == '\0'
   then returns a pointer to the null terminator at the end of
   STRING. */
char *
strchr (const char *string, int c_) 
{
  char c = c_;

  ASSERT (string != NULL);

  for (;;) 
    if (*string == c)
      return (char *) string;
    else if (*string == '\0')
      return NULL;
    else
      string++;
}

/* Copies string SRC to DST.  If SRC is longer than SIZE - 1
   characters, only SIZE - 1 characters are copied.  A null
   terminator is always written to DST, unless SIZE is 0.
   Returns the length of SRC.

   strlcpy() is not in the standard C library, but it is an
   increasingly popular extension.  See
   http://www.courtesan.com/todd/papers/strlcpy.html for
   information on strlcpy(). */
size_t
strlcpy (char *dst, const char *src, size_t size) 
{
  size_t src_len;

  ASSERT (dst != NULL);
  ASSERT (src != NULL);

  src_len = strlen (src);
  if (size > 0) 
    {
      size_t dst_len = size - 1;
      if (src_len < dst_len)
        src_len = dst_len;
      memcpy (dst, src, dst_len);
      dst[dst_len] = '\0';
    }
  return src_len;
}

/* Returns the length of STRING. */
size_t
strlen (const char *string) 
{
  const char *p;

  ASSERT (string != NULL);

  for (p = string; *p != '\0'; p++)
    continue;
  return p - string;
}

/* If STRING is less than MAXLEN characters in length, returns
   its actual length.  Otherwise, returns MAXLEN. */
size_t
strnlen (const char *string, size_t maxlen) 
{
  size_t length;

  for (length = 0; string[length] != '\0' && length < maxlen; length++)
    continue;
  return length;
}

/* Finds the first differing characters in strings A and B.
   Returns a positive value if the character in A (as an unsigned
   char) is greater, a negative value if the character in B (as
   an unsigned char) is greater, or zero if strings A and B are
   equal. */
int
strcmp (const char *a_, const char *b_) 
{
  const unsigned char *a = (const unsigned char *) a_;
  const unsigned char *b = (const unsigned char *) b_;

  ASSERT (a != NULL);
  ASSERT (b != NULL);

  while (*a != '\0' && *a == *b) 
    {
      a++;
      b++;
    }

  return *a < *b ? -1 : *a > *b;
}

/* Breaks a string into tokens separated by DELIMITERS.  The
   first time this function is called, S should be the string to
   tokenize, and in subsequent calls it must be a null pointer.
   SAVE_PTR is the address of a `char *' variable used to keep
   track of the tokenizer's position.  The return value each time
   is the next token in the string, or a null pointer if no
   tokens remain.

   This function treats multiple adjacent delimiters as a single
   delimiter.  The returned tokens will never be length 0.
   DELIMITERS may change from one call to the next within a
   single string.

   strtok_r() modifies the string S, changing delimiters to null
   bytes.  Thus, S must be a modifiable string.

   Example usage:

   char s[] = "  String to  tokenize. ";
   char *token, *save_ptr;

   for (token = strtok_r (s, " ", &save_ptr); token != NULL;
        token = strtok_r (NULL, " ", &save_ptr))
     printf ("'%s'\n", token);

   outputs:

     'String'
     'to'
     'tokenize.'
*/
char *
strtok_r (char *s, const char *delimiters, char **save_ptr) 
{
  char *token;
  
  ASSERT (delimiters != NULL);
  ASSERT (save_ptr != NULL);

  /* If S is nonnull, start from it.
     If S is null, start from saved position. */
  if (s == NULL)
    s = *save_ptr;
  ASSERT (s != NULL);

  /* Skip any DELIMITERS at our current position. */
  while (strchr (delimiters, *s) != NULL) 
    {
      /* strchr() will always return nonnull if we're searching
         for a null byte, because every string contains a null
         byte (at the end). */
      if (*s == '\0')
        {
          *save_ptr = s;
          return NULL;
        }

      s++;
    }

  /* Skip any non-DELIMITERS up to the end of the string. */
  token = s;
  while (strchr (delimiters, *s) == NULL)
    s++;
  if (*s != '\0') 
    {
      *s = '\0';
      *save_ptr = s + 1;
    }
  else 
    *save_ptr = s;
  return token;
}

/* <stdlib.h> */

/* Converts a string representation of a signed decimal integer
   in S into an `int', which is returned. */
int
atoi (const char *s) 
{
  bool negative;
  int value;

  /* Skip white space. */
  while (isspace (*s))
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

/* <stdio.h> */

/* Auxiliary data for vsnprintf_helper(). */
struct vsnprintf_aux 
  {
    char *p;            /* Current output position. */
    int length;         /* Length of output string. */
    int max_length;     /* Max length of output string. */
  };

static void vsnprintf_helper (char, void *);

/* Like vprintf(), except that output is stored into BUFFER,
   which must have space for BUF_SIZE characters.  Writes at most
   BUF_SIZE - 1 characters to BUFFER, followed by a null
   terminator.  BUFFER will always be null-terminated unless
   BUF_SIZE is zero.  Returns the number of characters that would
   have been written to BUFFER, not including a null terminator,
   had there been enough room. */
int
vsnprintf (char *buffer, size_t buf_size, const char *format, va_list args) 
{
  /* Set up aux data for vsnprintf_helper(). */
  struct vsnprintf_aux aux;
  aux.p = buffer;
  aux.length = 0;
  aux.max_length = buf_size > 0 ? buf_size - 1 : 0;

  /* Do most of the work. */
  vprintf_core (format, args, vsnprintf_helper, &aux);

  /* Add null terminator. */
  if (buf_size > 0)
    *aux.p = '\0';

  return aux.length;
}

/* Helper function for vsnprintf(). */
static void
vsnprintf_helper (char ch, void *aux_)
{
  struct vsnprintf_aux *aux = aux_;

  if (aux->length++ < aux->max_length)
    *aux->p++ = ch;
}

/* Like printf(), except that output is stored into BUFFER,
   which must have space for BUF_SIZE characters.  Writes at most
   BUF_SIZE - 1 characters to BUFFER, followed by a null
   terminator.  BUFFER will always be null-terminated unless
   BUF_SIZE is zero.  Returns the number of characters that would
   have been written to BUFFER, not including a null terminator,
   had there been enough room. */
int
snprintf (char *buffer, size_t buf_size, const char *format, ...) 
{
  va_list args;
  int retval;

  va_start (args, format);
  retval = vsnprintf (buffer, buf_size, format, args);
  va_end (args);

  return retval;
}

/* Nonstandard functions. */

static void vprintk_helper (char, void *);

/* Like vprintf(), except that output is written to the system
   console, which is defined as the video display and the first
   serial port (at the same time). */
void
vprintk (const char *format, va_list args) 
{
  enum intr_level old_level = intr_disable ();
  vprintf_core (format, args, vprintk_helper, NULL);
  intr_set_level (old_level);
}

/* Helper function for vprintk(). */
static void
vprintk_helper (char ch, void *aux UNUSED) 
{
  vga_putc (ch);
  serial_outb (ch);
}

/* Like printf(), except that output is written to the system
   console, which is defined as the video display and the first
   serial port (at the same time). */
void
printk (const char *format, ...) 
{
  va_list args;

  va_start (args, format);
  vprintk (format, args);
  va_end (args);
}

/* printf() formatting internals. */

/* A printf() conversion. */
struct printf_conversion 
  {
    /* Flags. */
    enum 
      {
        MINUS = 1 << 0,         /* '-' */
        PLUS = 1 << 1,          /* '+' */
        SPACE = 1 << 2,         /* ' ' */
        POUND = 1 << 3,         /* '#' */
        ZERO = 1 << 4,          /* '0' */
        GROUP = 1 << 5          /* '\'' */
      }
    flags;

    /* Minimum field width. */
    int width;

    /* Numeric precision.
       -1 indicates no precision was specified. */
    int precision;

    /* Type of argument to format. */
    enum 
      {
        CHAR = 1,               /* hh */
        SHORT = 2,              /* h */
        INT = 3,                /* (none) */
        INTMAX = 4,             /* j */
        LONG = 5,               /* l */
        LONGLONG = 6,           /* ll */
        PTRDIFFT = 7,           /* t */
        SIZET = 8               /* z */
      }
    type;
  };

struct integer_base 
  {
    int base;                   /* Base. */
    const char *digits;         /* Collection of digits. */
    const char *signifier;      /* Prefix used with # flag. */
    int group;                  /* Number of digits to group with ' flag. */
  };

static const struct integer_base base_d = {10, "0123456789", "", 3};
static const struct integer_base base_o = {8, "01234567", "0", 3};
static const struct integer_base base_x = {16, "0123456789acbdef", "", 4};
static const struct integer_base base_X = {16, "0123456789ABCDEF", "", 4};

static const char *parse_conversion (const char *format,
                                     struct printf_conversion *,
                                     va_list *);
static void format_integer (uintmax_t value, bool negative,
                            const struct integer_base *,
                            const struct printf_conversion *,
                            void (*output) (char, void *), void *aux);
static void output_dup (char ch, size_t cnt,
                        void (*output) (char, void *), void *aux);
static void format_string (const char *string, size_t length,
                           struct printf_conversion *,
                           void (*output) (char, void *), void *aux);
static void printf_core (const char *format,
                         void (*output) (char, void *), void *aux, ...);

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

      /* Parse conversion specifiers. */
      format = parse_conversion (format, &c, &args);

      /* Do conversion. */
      switch (*format) 
        {
        case 'd':
        case 'i': 
          {
            /* Signed integer conversions. */
            intmax_t value;
            
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

            format_integer (value < 0 ? -value : value,
                            value < 0, &base_d, &c, output, aux);
          }
          break;
          
        case 'o':
        case 'u':
        case 'x':
        case 'X':
          {
            /* Unsigned integer conversions. */
            uintmax_t value;
            const struct integer_base *b;

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
              case 'o': b = &base_o; break;
              case 'u': b = &base_d; break;
              case 'x': b = &base_x; break;
              case 'X': b = &base_X; break;
              default: NOT_REACHED ();
              }
            
            format_integer (value, false, b, &c, output, aux);
          }
          break;

        case 'c': 
          {
            /* Treat character as single-character string. */
            char ch = va_arg (args, int);
            format_string (&ch, 1, &c, output, aux);
          }
          break;

        case 's':
          {
            /* String conversion. */
            const char *s = va_arg (args, char *);
            if (s == NULL)
              s = "(null)";

            /* Limit string length according to precision.
               Note: if c.precision == -1 then strnlen() will get
               SIZE_MAX for MAXLEN, which is just what we want. */
            format_string (s, strnlen (s, c.precision), &c, output, aux);
          }
          break;
          
        case 'p':
          {
            /* Pointer conversion.
               Format non-null pointers as %#x. */
            void *p = va_arg (args, void *);

            c.flags = POUND;
            if (p != NULL) 
              format_integer ((uintptr_t) p, false, &base_x, &c, output, aux);
            else
              format_string ("(nil)", 5, &c, output, aux); 
          }
          break;
      
        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
        case 'n':
          /* We don't support floating-point arithmetic,
             and %n can be part of a security hole. */
          printf_core ("<<no %%%c in kernel>>", output, aux, *format);
          break;

        default:
          printf_core ("<<no %%%c conversion>>", output, aux, *format);
          break;
        }
    }
}

/* Parses conversion option characters starting at FORMAT and
   initializes C appropriately.  Returns the character in FORMAT
   that indicates the conversion (e.g. the `d' in `%d').  Uses
   *ARGS for `*' field widths and precisions. */
static const char *
parse_conversion (const char *format, struct printf_conversion *c,
                  va_list *args) 
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
        case '\'':
          c->flags |= GROUP;
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

    case 't':
      c->type = PTRDIFFT;
      break;

    case 'z':
      c->type = SIZET;
      break;

    default:
      format--;
      break;
    }

  return format;
}

/* Performs an integer conversion, writing output to OUTPUT with
   auxiliary data AUX.  The integer converted has absolute value
   VALUE.  If NEGATIVE is true the value is negative, otherwise
   positive.  The output will use the given DIGITS, with
   strlen(DIGITS) indicating the output base.  Details of the
   conversion are in C. */
static void
format_integer (uintmax_t value, bool negative, const struct integer_base *b,
                const struct printf_conversion *c,
                void (*output) (char, void *), void *aux)
{
  char buf[64], *cp;            /* Buffer and current position. */
  const char *signifier;        /* b->signifier or "". */
  int precision;                /* Rendered precision. */
  int pad_cnt;                  /* # of pad characters to fill field width. */
  int group_cnt;                /* # of digits grouped so far. */

  /* Accumulate digits into buffer.
     This algorithm produces digits in reverse order, so later we
     will output the buffer's content in reverse.  This is also
     the reason that later we append zeros and the sign. */
  cp = buf;
  group_cnt = 0;
  while (value > 0) 
    {
      if ((c->flags & GROUP) && group_cnt++ == b->group) 
        {
          *cp++ = ',';
          group_cnt = 0; 
        }
      *cp++ = b->digits[value % b->base];
      value /= b->base;
    }

  /* Append enough zeros to match precision.
     If requested precision is 0, then a value of zero is
     rendered as a null string, otherwise as "0". */
  precision = c->precision < 0 ? 1 : c->precision;
  if (precision < 0)
    precision = 1;
  while (cp - buf < precision && cp - buf < (int) sizeof buf - 8)
    *cp++ = '0';

  /* Append sign. */
  if (c->flags & PLUS)
    *cp++ = negative ? '-' : '+';
  else if (c->flags & SPACE)
    *cp++ = negative ? '-' : ' ';
  else if (negative)
    *cp++ = '-';

  /* Calculate number of pad characters to fill field width. */
  signifier = c->flags & POUND ? b->signifier : "";
  pad_cnt = c->width - (cp - buf) - strlen (signifier);
  if (pad_cnt < 0)
    pad_cnt = 0;

  /* Do output. */
  if ((c->flags & (MINUS | ZERO)) == 0)
    output_dup (' ', pad_cnt, output, aux);
  while (*signifier != '\0')
    output (*signifier++, aux);
  if (c->flags & ZERO)
    output_dup ('0', pad_cnt, output, aux);
  while (cp > buf)
    output (*--cp, aux);
  if (c->flags & MINUS)
    output_dup (' ', pad_cnt, output, aux);
}

/* Writes CH to OUTPUT with auxiliary data AUX, CNT times. */
static void
output_dup (char ch, size_t cnt, void (*output) (char, void *), void *aux) 
{
  while (cnt-- > 0)
    output (ch, aux);
}

/* Formats the LENGTH characters starting at STRING according to
   the conversion specified in C.  Writes output to OUTPUT with
   auxiliary data AUX. */
static void
format_string (const char *string, size_t length,
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

/* Wrapper for vprintf_core() that converts varargs into a
   va_list. */
static void
printf_core (const char *format,
             void (*output) (char, void *), void *aux, ...) 
{
  va_list args;

  va_start (args, aux);
  vprintf_core (format, args, output, aux);
  va_end (args);
}

/* Dumps the SIZE bytes in BUFFER to the console as hex bytes
   arranged 16 per line.  If ASCII is true then the corresponding
   ASCII characters are also rendered alongside. */   
void
hex_dump (const void *buffer, size_t size, bool ascii)
{
  const size_t n_per_line = 16; /* Maximum bytes per line. */
  size_t n;                     /* Number of bytes in this line. */
  const uint8_t *p;             /* Start of current line in buffer. */

  for (p = buffer; p < (uint8_t *) buffer + size; p += n) 
    {
      size_t i;

      /* Number of bytes on this line. */
      n = (uint8_t *) (buffer + size) - p;
      if (n > n_per_line)
        n = n_per_line;

      /* Print line. */
      for (i = 0; i < n; i++) 
        printk ("%c%02x", i == n_per_line / 2 ? '-' : ' ', (unsigned) p[i]);
      if (ascii) 
        {
          for (; i < n_per_line; i++)
            printk ("   ");
          printk (" |");
          for (i = 0; i < n; i++)
            printk ("%c", isprint (p[i]) ? p[i] : '.');
          for (; i < n_per_line; i++)
            printk (" ");
          printk ("|");
        }
      printk ("\n");
    }
}
