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

  backtrace ();

  for (;;);
}

void
backtrace (void) 
{
  void **frame;
  
  printk ("Call stack:");
  for (frame = __builtin_frame_address (0);
       frame != NULL && frame[0] != NULL;
       frame = frame[0]) 
    printk (" %p", frame[1]);
  printk (".\n");
}
