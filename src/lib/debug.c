#include <debug.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#ifdef KERNEL
#include "threads/init.h"
#include "threads/interrupt.h"
#include "devices/serial.h"
#else
#include <syscall.h>
#endif

/* Halts the OS or user program, printing the source file name,
   line number, and function name, plus a user-specific
   message. */
void
debug_panic (const char *file, int line, const char *function,
             const char *message, ...)
{
  va_list args;

#ifdef KERNEL
  intr_disable ();
#endif

#ifdef KERNEL
  printf ("Kernel PANIC at %s:%d in %s(): ", file, line, function);
#else
  printf ("User process panic at %s:%d in %s(): ", file, line, function);
#endif

  va_start (args, message);
  vprintf (message, args);
  printf ("\n");
  va_end (args);

  debug_backtrace ();

  printf ("The `backtrace' program can make call stacks useful.\n"
          "Read \"Backtraces\" in the \"Debugging Tools\" chapter\n"
          "of the Pintos documentation for more information.\n");
  
#ifdef KERNEL
  serial_flush ();
  if (power_off_when_done)
    power_off ();
  for (;;);
#else
  exit (1);
#endif
}

/* Prints the call stack, that is, a list of addresses, one in
   each of the functions we are nested within.  gdb or addr2line
   may be applied to kernel.o to translate these into file names,
   line numbers, and function names.  */
void
debug_backtrace (void) 
{
  void **frame;
  
  printf ("Call stack:");
  for (frame = __builtin_frame_address (0);
       frame != NULL && frame[0] != NULL;
       frame = frame[0]) 
    printf (" %p", frame[1]);
  printf (".\n");
}
