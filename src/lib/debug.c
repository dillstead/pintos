#include "debug.h"
#include <stdarg.h>
#include "lib.h"
#include "threads/interrupt.h"

#define MAX_CLASSES 16
static bool all_enabled;
static const char *enabled_classes[MAX_CLASSES];
static size_t enabled_cnt;

static bool class_is_enabled (const char *class);

/* Enables the debug message classes specified in CLASSES.  The
   string CLASSES is modified by and becomes owned by this
   function. */
void
debug_enable (char *classes) 
{
  char *class, *save;

  for (class = strtok_r (classes, ",", &save); class != NULL;
       class = strtok_r (NULL, ",", &save))
    {
      if (strcmp (class, "all") && enabled_cnt < MAX_CLASSES)
        enabled_classes[enabled_cnt++] = class;
      else
        all_enabled = true;
    }
}

/* Prints a debug message along with the source file name, line
   number, and function name of where it was emitted.  CLASS is
   used to filter out unwanted messages. */
void
debug_message (const char *file, int line, const char *function,
               const char *class, const char *message, ...) 
{
  if (class_is_enabled (class)) 
    {
      va_list args;

      enum intr_level old_level = intr_disable ();
      printk ("%s:%d: %s(): ", file, line, function);
      va_start (args, message);
      vprintk (message, args);
      printk ("\n");
      va_end (args);
      intr_set_level (old_level);
    }
}

/* Halts the OS, printing the source file name, line number, and
   function name, plus a user-specific message. */
void
debug_panic (const char *file, int line, const char *function,
             const char *message, ...)
{
  va_list args;

  intr_disable ();

  printk ("PANIC at %s:%d in %s(): ", file, line, function);
  va_start (args, message);
  vprintk (message, args);
  printk ("\n");
  va_end (args);

  debug_backtrace ();

  for (;;);
}

/* Prints the call stack, that is, a list of addresses in each of
   the functions we are nested within.  gdb or addr2line may be
   applied to kernel.o to translate these into file names, line
   numbers, and function names.  */
void
debug_backtrace (void) 
{
  void **frame;
  
  printk ("Call stack:");
  for (frame = __builtin_frame_address (0);
       frame != NULL && frame[0] != NULL;
       frame = frame[0]) 
    printk (" %p", frame[1]);
  printk (".\n");
}


/* Returns true if CLASS is enabled, false otherwise. */
static bool
class_is_enabled (const char *class) 
{
  size_t i;
  
  if (all_enabled)
    return true;

  for (i = 0; i < enabled_cnt; i++)
    if (!strcmp (enabled_classes[i], class))
      return true;

  return false;
}
