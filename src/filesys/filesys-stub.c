#include "filesys-stub.h"
#include <stdarg.h>
#include "backdoor.h"
#include "debug.h"
#include "io.h"
#include "lib.h"
#include "synch.h"

static struct lock lock;

void 
filesys_stub_init (void)
{
  lock_init (&lock, "filesys-stub");
}

void 
filesys_stub_lock (void)
{
  lock_acquire (&lock);
}

void 
filesys_stub_unlock (void)
{
  lock_release (&lock);
}

static void
out_byte (uint8_t byte, void *aux UNUSED) 
{
  outb (0x8901, byte);
}

void 
filesys_stub_put_bool (bool b)
{
  backdoor_put_bool (b, out_byte, NULL);
}

void 
filesys_stub_put_bytes (const void *buffer, size_t cnt)
{
  backdoor_put_bytes (buffer, cnt, out_byte, NULL);
}

void 
filesys_stub_put_file (struct file *file)
{
  ASSERT (file != NULL);
  filesys_stub_put_int32 ((int32_t) file - 1);
}

void 
filesys_stub_put_int32 (int32_t value)
{
  backdoor_put_int32 (value, out_byte, NULL);
}

void 
filesys_stub_put_string (const char *string)
{
  backdoor_put_string (string, out_byte, NULL);
}

void 
filesys_stub_put_uint32 (uint32_t value)
{
  backdoor_put_uint32 (value, out_byte, NULL);
}

static uint8_t
in_byte (void *aux UNUSED) 
{
  return inb (0x8901);
}

bool 
filesys_stub_get_bool (void)
{
  return backdoor_get_bool (in_byte, NULL);
}

void 
filesys_stub_get_bytes (void *buffer, size_t size)
{
  backdoor_get_bytes (buffer, size, in_byte, NULL);
}

struct file *
filesys_stub_get_file (void)
{
  int32_t fd = filesys_stub_get_int32 ();
  return fd < 0 ? NULL : (struct file *) (fd + 1);
}

int32_t 
filesys_stub_get_int32 (void)
{
  return backdoor_get_int32 (in_byte, NULL);
}

void 
filesys_stub_match_string (const char *string)
{
  if (backdoor_get_uint32 (in_byte, NULL) != strlen (string))
    panic ("string match failed");
  while (*string != '\0') 
    {
      uint8_t c = *string++;
      if (c != in_byte (NULL))
        panic ("string match failed"); 
    }
}

uint32_t 
filesys_stub_get_uint32 (void)
{
  return backdoor_get_uint32 (in_byte, NULL);
}

