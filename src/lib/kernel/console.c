#include <console.h>
#include <stdarg.h>
#include <stdio.h>
#include "devices/serial.h"
#include "devices/vga.h"
#include "threads/interrupt.h"
#include "threads/synch.h"

static void vprintf_helper (char, void *);
static void putchar_unlocked (uint8_t c);

/* The console lock.
   Both the vga and serial layers do their own locking, so it's
   safe to call them at any time.
   But this lock is useful to prevent simultaneous printf() calls
   from mixing their output, which looks confusing. */
static struct lock console_lock;

void
console_init (void) 
{
  lock_init (&console_lock, "console");
}

/* The standard vprintf() function,
   which is like printf() but uses a va_list.
   Writes its output to both vga display and serial port. */
int
vprintf (const char *format, va_list args) 
{
  int char_cnt = 0;

  if (!intr_context ())
    lock_acquire (&console_lock);

  __vprintf (format, args, vprintf_helper, &char_cnt);

  if (!intr_context ())
    lock_release (&console_lock);

  return char_cnt;
}

/* Writes string S to the console, followed by a new-line
   character. */
int
puts (const char *s) 
{
  if (!intr_context ())
    lock_acquire (&console_lock);

  while (*s != '\0')
    putchar_unlocked (*s++);
  putchar_unlocked ('\n');

  if (!intr_context ())
    lock_release (&console_lock);

  return 0;
}

/* Writes the N characters in BUFFER to the console. */
void
putbuf (const char *buffer, size_t n) 
{
  if (!intr_context ())
    lock_acquire (&console_lock);

  while (n-- > 0)
    putchar_unlocked (*buffer++);

  if (!intr_context ())
    lock_release (&console_lock);
}

/* Writes C to the vga display and serial port. */
int
putchar (int c) 
{
  if (!intr_context ())
    lock_acquire (&console_lock);

  putchar_unlocked (c);

  if (!intr_context ())
    lock_release (&console_lock);

  return c;
}

/* Helper function for vprintf(). */
static void
vprintf_helper (char c, void *char_cnt_) 
{
  int *char_cnt = char_cnt_;
  (*char_cnt)++;
  putchar_unlocked (c);
}

/* Writes C to the vga display and serial port.
   The caller has already acquired the console lock if
   appropriate. */
static void
putchar_unlocked (uint8_t c) 
{
  serial_putc (c);
  vga_putc (c);
}
