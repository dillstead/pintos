#include <stdarg.h>
#include <stdio.h>

#include "devices/serial.h"
#include "devices/vga.h"
#include "threads/interrupt.h"

static void vprintf_helper (char, void *);

int
vprintf (const char *format, va_list args) 
{
  enum intr_level old_level;
  int char_cnt = 0;

  old_level = intr_disable ();
  __vprintf (format, args, vprintf_helper, &char_cnt);
  intr_set_level (old_level);

  return char_cnt;
}

/* Helper function for vprintf(). */
static void
vprintf_helper (char c, void *char_cnt_) 
{
  int *char_cnt = char_cnt_;
  (*char_cnt)++;
  putchar (c);
}

/* Writes C to the console. */
int
putchar (int c) 
{
  serial_outb (c);
  vga_putc (c);
  return c;
}
