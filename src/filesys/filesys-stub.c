#include "filesys-stub.h"
#include <stdarg.h>
#include "backdoor.h"
#include "debug.h"
#include "io.h"

static void
out_byte (uint8_t byte, void *aux UNUSED) 
{
  outb (0x8901, byte);
}

void
filesys_stub_send (const char *types, ...) 
{
  va_list args;

  va_start (args, types);
  backdoor_vmarshal (types, args, out_byte, NULL);
  va_end (args);
}

static bool
in_byte (uint8_t *byte, void *aux UNUSED) 
{
  *byte = inb (0x8901);
  return true;
}

void
filesys_stub_receive (const char *types, ...) 
{
  va_list args;

  va_start (args, types);
  backdoor_vunmarshal (types, args, in_byte, NULL);
  va_end (args);
}
