#include "debug.h"
#include <stdarg.h>
#include "interrupt.h"
#include "lib.h"

void
panic (const char *format, ...)
{
  va_list args;

  intr_disable ();

  va_start (args, format);
  vprintk (format, args);
  printk ("\n");
  va_end (args);

  for (;;);
}
