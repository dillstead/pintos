#include "fslib.h"
#include <random.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include "../lib/arc4.h"

bool quiet = false;

static void
vmsg (const char *format, va_list args, bool failure) 
{
  if (quiet && !failure)
    return;
  
  printf ("(%s) ", test_name);
  vprintf (format, args);
  if (failure)
    printf (": FAILED");
  printf ("\n");
}

void
msg (const char *format, ...) 
{
  va_list args;

  va_start (args, format);
  vmsg (format, args, false);
  va_end (args);
}

void
fail (const char *format, ...) 
{
  va_list args;

  quiet = false;
  
  va_start (args, format);
  vmsg (format, args, true);
  va_end (args);

  exit (1);
}

void
check (bool success, const char *format, ...) 
{
  va_list args;

  va_start (args, format);
  if (success) 
    {
      if (!quiet)
        vmsg (format, args, false); 
    }
  else 
    {
      vmsg (format, args, true);
      exit (1);
    }
  va_end (args);
}

void 
seq_test (const char *filename, void *buf, size_t size,
          size_t initial_size, int seed,
          size_t (*block_size_func) (void),
          void (*check_func) (int fd, long ofs)) 
{
  static struct arc4 arc4;
  size_t ofs;
  int fd;
  
  arc4_init (&arc4, &seed, sizeof seed);
  arc4_crypt (&arc4, buf, size);

  check (create (filename, initial_size), "create \"%s\"", filename);
  check ((fd = open (filename)) > 1, "open \"%s\"", filename);

  ofs = 0;
  msg ("writing \"%s\"", filename);
  while (ofs < size) 
    {
      size_t block_size = block_size_func ();
      if (block_size > size - ofs)
        block_size = size - ofs;

      if (write (fd, buf + ofs, block_size) <= 0)
        fail ("write %zu bytes at offset %zu in \"%s\" failed",
              block_size, ofs, filename);

      ofs += block_size;
      if (check_func != NULL)
        check_func (fd, ofs);
    }
  msg ("close \"%s\"", filename);
  close (fd);
  check_file (filename, buf, size);
}

static void
swap (void *a_, void *b_, size_t size) 
{
  uint8_t *a = a_;
  uint8_t *b = b_;
  size_t i;

  for (i = 0; i < size; i++) 
    {
      uint8_t t = a[i];
      a[i] = b[i];
      b[i] = t;
    }
}

void
shuffle (void *buf_, size_t cnt, size_t size) 
{
  char *buf = buf_;
  size_t i;

  for (i = 0; i < cnt; i++)
    {
      size_t j = i + random_ulong () % (cnt - i);
      swap (buf + i * size, buf + j * size, size);
    }
}

void
check_file (const char *filename, const void *buf_, size_t size) 
{
  const char *buf = buf_;
  size_t ofs;
  char block[512];
  int fd;

  check ((fd = open (filename)) > 1, "open \"%s\" for verification", filename);

  ofs = 0;
  while (ofs < size)
    {
      size_t block_size = size - ofs;
      if (block_size > sizeof block)
        block_size = sizeof block;

      if (read (fd, block, block_size) <= 0)
        fail ("read %zu bytes at offset %zu in \"%s\" failed",
              block_size, ofs, filename);

      if (memcmp (buf + ofs, block, block_size))
        {
          if (block_size <= 512) 
            {
              printf ("Expected data:\n");
              hex_dump (ofs, buf + ofs, block_size, false);
              printf ("Actually read data:\n");
              hex_dump (ofs, block, block_size, false); 
            }
          fail ("%zu bytes at offset %zu differed from expected",
                block_size, ofs);
        }

      ofs += block_size;
    }

  msg ("close \"%s\"", filename);
  close (fd);
}
